/*
// Copyright (c) 2016 Intel Corporation
// Copyright (c) 2022 Cascoda Ltd.
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

#include "messaging/coap/engine.h"
#include "oc_signal_event_loop.h"
#include "port/oc_network_events_mutex.h"
#include "util/oc_memb.h"
#include "messaging/coap/coap.h"
#include "api/oc_replay.h"
#include <stdint.h>
#include <stdio.h>
#ifdef OC_DYNAMIC_ALLOCATION
#include <stdlib.h>
#endif /* OC_DYNAMIC_ALLOCATION */

#ifdef OC_OSCORE
#include "security/oc_tls.h"
#include "security/oc_oscore.h"
#endif /* OC_OSCORE */
#include "messaging/coap/oscore.h"

#include "oc_buffer.h"
#include "oc_config.h"
#include "oc_events.h"

OC_PROCESS(message_buffer_handler, "OC Message Buffer Handler");
#ifdef OC_INOUT_BUFFER_POOL
OC_MEMB_STATIC(oc_incoming_buffers, oc_message_t, OC_INOUT_BUFFER_POOL);
OC_MEMB_STATIC(oc_outgoing_buffers, oc_message_t, OC_INOUT_BUFFER_POOL);
#else  /* OC_INOUT_BUFFER_POOL */
OC_MEMB(oc_incoming_buffers, oc_message_t, OC_MAX_NUM_CONCURRENT_REQUESTS);
OC_MEMB(oc_outgoing_buffers, oc_message_t, OC_MAX_NUM_CONCURRENT_REQUESTS);
#endif /* !OC_INOUT_BUFFER_POOL */

static oc_message_t *
allocate_message(struct oc_memb *pool)
{
  oc_network_event_handler_mutex_lock();
  oc_message_t *message = (oc_message_t *)oc_memb_alloc(pool);
  oc_network_event_handler_mutex_unlock();
  if (message) {
#if defined(OC_DYNAMIC_ALLOCATION) && !defined(OC_INOUT_BUFFER_SIZE)
    message->data = malloc(OC_PDU_SIZE);
    if (!message->data) {
      OC_ERR("Out of memory, cannot allocate message");
      oc_memb_free(pool, message);
      return NULL;
    }
#endif /* OC_DYNAMIC_ALLOCATION && !OC_INOUT_BUFFER_SIZE */
    message->pool = pool;
    message->length = 0;
    message->next = 0;
    message->ref_count = 1;
    message->endpoint.interface_index = -1;
    message->endpoint.device = 0;
    message->endpoint.group_address = 0;
    message->soft_ref_cb = NULL;

#ifdef OC_OSCORE
    message->encrypted = 0;
#endif /* OC_OSCORE */
#if !defined(OC_DYNAMIC_ALLOCATION) || defined(OC_INOUT_BUFFER_SIZE)
    OC_DBG("buffer: Allocated TX/RX buffer; num free: %d",
           oc_memb_numfree(pool));
#endif /* !OC_DYNAMIC_ALLOCATION || OC_INOUT_BUFFER_SIZE */
  } else {
    // no unused buffers, so go through buffers with soft references and
    // free one. said buffer can no longer be used for e.g. retransmitting
    // requests when challenged with an Echo option. however, freeing up
    // one of these means that it can no longer be used for its original
    // purpose
    for (int i = 0; i < pool->num; ++i) {
      int offset = pool->size * i;
      message = (oc_message_t *)((uint8_t *)pool->mem + offset);

      if (message->ref_count == 1 && message->soft_ref_cb != NULL) {
        // OC_WRN("Freeing echo retransmission candidate %p");
        message->soft_ref_cb(message);
        // we know that was the last reference, so now we can allocate
        // a new message successfully
        return allocate_message(pool);
      }
    }

    OC_WRN("buffer: No free TX/RX buffers!");
    message = NULL;
  }
  return message;
}

oc_message_t *
oc_allocate_message_from_pool(struct oc_memb *pool)
{
  if (pool) {
    return allocate_message(pool);
  }
  return NULL;
}

void
oc_set_buffers_avail_cb(oc_memb_buffers_avail_callback_t cb)
{
  oc_memb_set_buffers_avail_cb(&oc_incoming_buffers, cb);
}

oc_message_t *
oc_allocate_message(void)
{
  return allocate_message(&oc_incoming_buffers);
}

oc_message_t *
oc_internal_allocate_outgoing_message(void)
{
  return allocate_message(&oc_outgoing_buffers);
}

void
oc_message_add_ref(oc_message_t *message)
{
  if (message) {
    message->ref_count++;
  }
}

void
oc_message_unref(oc_message_t *message)
{
  if (message) {
    message->ref_count--;
    if (message->ref_count <= 0) {
#if defined(OC_DYNAMIC_ALLOCATION) && !defined(OC_INOUT_BUFFER_SIZE)
      if (message->data != NULL) {
        free(message->data);
      }
#endif /* OC_DYNAMIC_ALLOCATION && !OC_INOUT_BUFFER_SIZE */
      struct oc_memb *pool = message->pool;
      if (pool != NULL) {
        oc_memb_free(pool, message);
      }
    }
  }
}

void
oc_recv_message(oc_message_t *message)
{
  if (oc_process_post(&message_buffer_handler, oc_events[INBOUND_NETWORK_EVENT],
                      message) == OC_PROCESS_ERR_FULL) {
    oc_message_unref(message);
  }
}

void
oc_send_message(oc_message_t *message)
{
  // we only want to cache OSCORE-secured requests, as these frames are the
  // only ones that will be challenged with an Echo option. however, at this
  // point we only have the encoded CoAP bytes, so we parse just the header
  // and token.
  uint8_t version = (COAP_HEADER_VERSION_MASK & message->data[0]) >>
                    COAP_HEADER_VERSION_POSITION;
  uint8_t type =
    (COAP_HEADER_TYPE_MASK & message->data[0]) >> COAP_HEADER_TYPE_POSITION;
  uint8_t code = message->data[1];
  uint8_t token_len = (COAP_HEADER_TOKEN_LEN_MASK & message->data[0]) >>
                      COAP_HEADER_TOKEN_LEN_POSITION;
  uint8_t *token = message->data + COAP_HEADER_LEN;

  if (version == 1 && type == 1 && (code >> 5 == 0) &&
      message->endpoint.flags & SECURED) {
    oc_replay_message_track(message, token_len, token);
  }

  if (oc_process_post(&message_buffer_handler,
                      oc_events[OUTBOUND_NETWORK_EVENT],
                      message) == OC_PROCESS_ERR_FULL) {
    OC_ERR("oc_send_message  ref_count decrease due to FULL\n");
    message->ref_count--;
  }

  _oc_signal_event_loop();
}

#ifdef OC_SECURITY
void
oc_close_all_tls_sessions_for_device(size_t device)
{
  oc_process_post(&message_buffer_handler, oc_events[TLS_CLOSE_ALL_SESSIONS],
                  (oc_process_data_t)device);
}

void
oc_close_all_tls_sessions(void)
{
  oc_process_poll(&(oc_tls_handler));
  _oc_signal_event_loop();
}
#endif /* OC_SECURITY */

OC_PROCESS_THREAD(message_buffer_handler, ev, data)
{
  OC_PROCESS_BEGIN();
  OC_DBG("Started buffer handler process");
  while (1) {
    OC_PROCESS_YIELD();

    if (ev == oc_events[INBOUND_NETWORK_EVENT]) {

#ifdef OC_OSCORE
      if (oscore_is_oscore_message((oc_message_t *)data) == 0) {
        OC_DBG_OSCORE("Inbound network event: oscore request");
        oc_process_post(&oc_oscore_handler, oc_events[INBOUND_OSCORE_EVENT],
                        data);
      } else
#endif /* OC_OSCORE */
      {
        OC_DBG_OSCORE("Inbound network event: decrypted request");
        oc_process_post(&coap_engine, oc_events[INBOUND_RI_EVENT], data);
      }

    } else if (ev == oc_events[OUTBOUND_NETWORK_EVENT]) {
      oc_message_t *message = (oc_message_t *)data;
      /* handle OSCORE first*/
#if OC_OSCORE
      if ((message->endpoint.flags & MULTICAST) &&
          (message->endpoint.flags & OSCORE) &&
          ((message->endpoint.flags & OSCORE_ENCRYPTED) == 0)) {
        OC_DBG_OSCORE(
          "Outbound secure multicast request: forwarding to OSCORE");
        oc_process_post(&oc_oscore_handler,
                        oc_events[OUTBOUND_GROUP_OSCORE_EVENT], data);
      } else if ((message->endpoint.flags & OSCORE) &&
                 ((message->endpoint.flags & OSCORE_ENCRYPTED) == 0)) {
        OC_DBG_OSCORE("Outbound network event: forwarding to OSCORE");
        oc_process_post(&oc_oscore_handler, oc_events[OUTBOUND_OSCORE_EVENT],
                        data);
      } else
#endif /* !OC_OSCORE */
        if (message->endpoint.flags & DISCOVERY) {
          OC_DBG("Outbound network event: multicast request");
          oc_endpoint_print(&message->endpoint);
          oc_send_discovery_request(message);
          oc_message_unref(message);
        } else {
          OC_DBG("Outbound network event: unicast message");
          oc_message_t *message = (oc_message_t *)data;
          oc_send_buffer(message);
          oc_message_unref(message);
        }
    } else if (ev == oc_events[OUTBOUND_NETWORK_EVENT_ENCRYPTED]) {
      OC_DBG("Outbound network event:OUTBOUND_NETWORK_EVENT_ENCRYPTED");
      oc_message_t *message = (oc_message_t *)data;
      oc_send_buffer(message);
      oc_message_unref(message);
    }
  }
  OC_PROCESS_END();
}

oc_message_t *
oc_get_incoming_message_with_ptr(uint8_t *data)
{
  struct oc_memb *pool = &oc_incoming_buffers;
  for (size_t i = 0; i < pool->num; ++i) {
    // unused block, should not contain data of a valid message
    if (pool->count[i] <= 0)
      continue;

    int offset = i * (int)pool->size;
    oc_message_t *msg = (oc_message_t *)((char *)pool->mem + offset);

    if (msg->data <= data && data < msg->data + msg->length) {
      // data lies within msg, so we return it
      return msg;
    }
  }
  return NULL;
}

int
oc_buffer_num_free_incoming()
{
  return oc_memb_numfree(&oc_incoming_buffers);
}

int
oc_buffer_num_free_outgoing()
{
  return oc_memb_numfree(&oc_outgoing_buffers);
}