/*
// Copyright (c) 2023 Cascoda Ltd
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <stdbool.h>

#include "oc_replay.h"

#include "port/oc_clock.h"
#include "oc_knx_sec.h"
#include "oc_config.h"
#include "messaging/coap/constants.h"
#include "oc_api.h"

#ifndef OC_MAX_REPLAY_RECORDS
#define OC_MAX_REPLAY_RECORDS (20)
#endif

#ifndef OC_MAX_MESSAGE_RECORDS
#define OC_MAX_MESSAGE_RECORDS (2)
#endif

#ifndef OC_REPLAY_RECORD_TIMEOUT
#define OC_REPLAY_RECORD_TIMEOUT (5)
#endif

static struct oc_replay_record
{
  uint64_t rx_ssn;        /// most recent received SSN of client
  oc_string_t rx_kid;     /// byte string holding the KID of the client
  oc_string_t rx_kid_ctx; /// byte string holding the KID context of the client.
                          /// can be null
  oc_clock_time_t time;   /// time of last received packet
  uint32_t window; /// bitfield indicating received SSNs through bit position
  bool in_use;     /// whether this structure is in use & has valid data
} replay_records[OC_MAX_REPLAY_RECORDS] = { 0 };

static struct oc_cached_message_record
{
  uint16_t token_len;
  uint8_t token[COAP_TOKEN_LEN];
  struct oc_message_s *message;
} message_records[OC_MAX_MESSAGE_RECORDS] = { 0 };

// make record available for reuse
static void
free_record(struct oc_replay_record *rec)
{
  // bounds check
  if (replay_records <= rec && rec < replay_records + OC_MAX_REPLAY_RECORDS) {
    rec->rx_ssn = 0;
    rec->window = 0;
    oc_free_string(&rec->rx_kid);
    oc_free_string(&rec->rx_kid_ctx);
    rec->time = 0;
    rec->in_use = false;
  }
}

// get the first available record
static struct oc_replay_record *
get_empty_record()
{
  for (size_t i = 0; i < OC_MAX_REPLAY_RECORDS; ++i) {
    if (!replay_records[i].in_use)
      return replay_records + i;
  }

  struct oc_replay_record *oldest_rec = replay_records;

  for (size_t i = 1; i < OC_MAX_REPLAY_RECORDS; ++i) {
    if (replay_records[i].time < oldest_rec->time)
      oldest_rec = replay_records + i;
  }
  free_record(oldest_rec);
  return oldest_rec;
}

// find record with KID and CTX
static struct oc_replay_record *
get_record(oc_string_t rx_kid, oc_string_t rx_kid_ctx)
{
  if (oc_byte_string_len(rx_kid) == 0)
    return NULL;

  for (size_t i = 0; i < OC_MAX_REPLAY_RECORDS; ++i) {
    struct oc_replay_record *rec = replay_records + i;

    if (rec->in_use) {
      bool rx_kid_match = oc_byte_string_cmp(rx_kid, rec->rx_kid) == 0;
      bool null_contexts = oc_byte_string_len(rx_kid_ctx) == 0 &&
                           oc_byte_string_len(rec->rx_kid_ctx) == 0;
      bool contexts_match =
        oc_byte_string_cmp(rx_kid_ctx, rec->rx_kid_ctx) == 0;

      if (rx_kid_match && (null_contexts || contexts_match))
        return rec;
    }
  }
  return NULL;
}

// return true if SSN of device identified by KID & KID_CTX is within replay
// window
//    if it is, update SSN within replay record
// return false if no entry found, or if SSN is outside replay window
bool
oc_replay_check_client(uint64_t rx_ssn, oc_string_t rx_kid,
                       oc_string_t rx_kid_ctx)
{
  /*
  With CoAP over UDP, you cannot guarantee messages are received in order.
  what if you happen to receive SSN 32 followed by non-replayed SSNs 28,
  29, 30, 31... do you drop all these?

  We can use the default anti-replay algorithm specified by OSCORE, which
  uses a sliding window in order to track every received SSN within a
  given range.

  This can be implemented very efficiently using a bitfield, where the
  position of bits indicate the SSN being considered, and the value
  of the bit indicates whether the packet has been received before

  The entire bitfield is left shifted whenever the recorded SSN increases,
  thus 'sliding' the window in a very efficient manner

  Here's an example of the algorithm in operation, with a reduced bitfield
  for readability:

  ssn = 8
  bitfield = 0b1100'0011

  rx 6, 8 - 6 = 2, check bit 2, accept & set bit 2

  ssn = 8
  bitfield = 0b1100'0111

  rx 7 again, thrown out because bit 8 - 7 = 1 is set
  rx 2 again, thrown out because 8 - 2 = 6 and bit 6 is set
  rx 8, 8 - 8 = 0, check bit 0 & reject

  rx 9, 8 - 9 = -1,  change ssn, left shift bitfield by 1, set bit 0

  ssn = 9
  bitfield = 0b1000'1111

  */

  struct oc_replay_record *rec = get_record(rx_kid, rx_kid_ctx);
  if (rec == NULL)
    return false;

  // received message matched existing record, so this record is useful &
  // should be kept around - thus we update the time here
  rec->time = oc_clock_time();

  int64_t ssn_diff = rec->rx_ssn - rx_ssn;

  if (ssn_diff >= 0) {
    // ensure it is not too old
    if (ssn_diff > sizeof(rec->window) * 8)
      return false;

    // received SSN is within the window - see if it has been received before
    if (rec->window & (1 << ssn_diff)) {
      // received before, so this is a replay
      return false;
    } else {
      // not received before, so remember that this SSN has been seen before
      rec->window |= 1 << ssn_diff;
      return true;
    }
  } else {
    uint64_t rplwdo = oc_oscore_get_rplwdo();
    if (-ssn_diff <= rplwdo) {
      // slide the window and accept the packet
      rec->rx_ssn = rx_ssn;
      // ssn_diff is negative in this side of the if
      rec->window = rec->window << (-ssn_diff);
      // set bit 1, indicating ssn rec->rx_ssn has been received
      rec->window |= 1;
      return true;
    } else {
      return false;
    }
  }
}

// update replay record if match found
// otherwise, create new replay record
void
oc_replay_add_client(uint64_t rx_ssn, oc_string_t rx_kid,
                     oc_string_t rx_kid_ctx)
{
  struct oc_replay_record *rec = get_record(rx_kid, rx_kid_ctx);

  if (!rec) {
    rec = get_empty_record();
    oc_byte_string_copy(&rec->rx_kid, rx_kid);
    oc_byte_string_copy(&rec->rx_kid_ctx, rx_kid_ctx);
    rec->in_use = true;
  }

  rec->rx_ssn = rx_ssn;
  rec->window = 1;
  rec->time = oc_clock_time();
}

void
oc_replay_free_client(oc_string_t rx_kid)
{
  for (size_t i = 0; i < OC_MAX_REPLAY_RECORDS; ++i) {
    struct oc_replay_record *rec = replay_records + i;
    if (oc_byte_string_cmp(rx_kid, rec->rx_kid) == 0) {
      free_record(rec);
    }
  }
}

struct oc_message_s *
oc_replay_find_msg_by_token(uint16_t token_len, uint8_t *token)
{
  for (int i = 0; i < OC_MAX_MESSAGE_RECORDS; ++i) {
    if (message_records[i].message == NULL)
      continue;

    if (message_records[i].token_len == token_len) {
      if (memcmp(token, message_records[i].token, token_len) == 0)
        return message_records[i].message;
    }
  }
  return NULL;
}

static struct oc_cached_message_record *
find_record_by_msg(struct oc_message_s *msg)
{
  if (msg == NULL)
    return NULL;

  for (int i = 0; i < OC_MAX_MESSAGE_RECORDS; ++i)
    if (message_records[i].message = msg)
      return message_records + i;
  return NULL;
}

static struct oc_cached_message_record *
find_empty_msg_record()
{
  for (int i = 0; i < OC_MAX_MESSAGE_RECORDS; ++i)
    if (message_records[i].message == NULL)
      return message_records + i;
  return NULL;
}

static oc_event_callback_retval_t
oc_replay_free_msg_handler(void *msg)
{
  struct oc_cached_message_record *rec = find_record_by_msg(msg);
  if (msg) {
    // OC_DBG("Freeing tracked message %p with token:", msg);
    // OC_LOGbytes(rec->token, rec->token_len);
    rec->token_len = 0;
    rec->message = NULL;
    oc_message_unref(msg);
  }
  return OC_EVENT_DONE;
}

void
oc_replay_message_unref(struct oc_message_s *msg)
{
  oc_replay_free_msg_handler(msg);
  // OC_DBG("Removing callback...");
  oc_remove_delayed_callback(msg, oc_replay_free_msg_handler);
}

void
oc_replay_message_track(struct oc_message_s *msg, uint16_t token_len,
                        uint8_t *token)
{
  struct oc_cached_message_record *rec = find_empty_msg_record();
  if (rec == NULL)
    return;
  // OC_DBG("Tracking message %p with token:", msg);
  // OC_LOGbytes(token, token_len);
  oc_message_add_ref(msg);
  msg->soft_ref_cb = oc_replay_message_unref;

  rec->token_len = token_len;
  memcpy(rec->token, token, token_len);
  rec->message = msg;

  oc_set_delayed_callback(msg, oc_replay_free_msg_handler,
                          OC_REPLAY_RECORD_TIMEOUT);
}
