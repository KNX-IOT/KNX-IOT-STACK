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
  OC_DBG(" message allocated %p", *message);
  oc_network_event_handler_mutex_unlock();
  if (message) {
#if defined(OC_DYNAMIC_ALLOCATION) && !defined(OC_INOUT_BUFFER_SIZE)
    message->data = malloc(OC_PDU_SIZE);
    OC_DBG(" (((((((( message data %p size: %d", message->data, OC_PDU_SIZE);
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
    message->endpoint.group_id = 0;

    OC_DBG("allocating message ref_count %d", message->ref_count);
    OC_DBG("message data: %p", message->data);
#ifdef OC_OSCORE
    message->encrypted = 0;
#endif /* OC_OSCORE */
#if !defined(OC_DYNAMIC_ALLOCATION) || defined(OC_INOUT_BUFFER_SIZE)
    OC_DBG("buffer: Allocated TX/RX buffer; num free: %d",
           oc_memb_numfree(pool));
#endif /* !OC_DYNAMIC_ALLOCATION || OC_INOUT_BUFFER_SIZE */
  }
#if !defined(OC_DYNAMIC_ALLOCATION) || defined(OC_INOUT_BUFFER_SIZE)
  else {
    OC_WRN("buffer: No free TX/RX buffers!");
  }
#endif /* !OC_DYNAMIC_ALLOCATION || OC_INOUT_BUFFER_SIZE */
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
  OC_DBG("%d", message->ref_count);
}

void
oc_message_unref(oc_message_t *message)
{
  if (message) {
    message->ref_count--;
    OC_DBG("refcount: %d", message->ref_count);
    if (message->ref_count <= 0) {
      PRINT("oc_message_unref: deallocating\n");
#if defined(OC_DYNAMIC_ALLOCATION) && !defined(OC_INOUT_BUFFER_SIZE)
      if (message->data != NULL) {
        OC_DBG(" )))))))) Free message data %p", message->data);
        free(message->data);
      }
#endif /* OC_DYNAMIC_ALLOCATION && !OC_INOUT_BUFFER_SIZE */
      struct oc_memb *pool = message->pool;
      if (pool != NULL) {
        OC_DBG(" FFFFFFFFFFF  Free message %p from pool %p size %d", message,
               pool, pool->size);
        oc_memb_free(pool, message);
      }
#if !defined(OC_DYNAMIC_ALLOCATION) || defined(OC_INOUT_BUFFER_SIZE)
      OC_DBG("buffer: freed TX/RX buffer; num free: %d", oc_memb_numfree(pool));
#endif /* !OC_DYNAMIC_ALLOCATION || OC_INOUT_BUFFER_SIZE */
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
  oc_endpoint_print(&message->endpoint);
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
