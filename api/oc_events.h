/*
 // Copyright (c) 2016 Intel Corporation
 // Copyright (c) 2021 Cascoda Ltd.
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
/**
* 
  @brief the various events for the different quues
  @file
*/
#ifndef OC_EVENTS_H
#define OC_EVENTS_H

#include "oc_config.h"
#include "util/oc_process.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  INBOUND_NETWORK_EVENT, /**< inbound network event*/
  UDP_TO_TLS_EVENT,
  INIT_TLS_CONN_EVENT,
  RI_TO_TLS_EVENT,
  INBOUND_RI_EVENT,
  OUTBOUND_NETWORK_EVENT, /**< outbound network event*/
  OUTBOUND_NETWORK_EVENT_ENCRYPTED, /**< outbound network event, payload is encrypted*/
  TLS_READ_DECRYPTED_DATA,
  TLS_WRITE_APPLICATION_DATA,
  INTERFACE_DOWN,  /**< network interface down*/
  INTERFACE_UP,    /**< network interface up */
  TLS_CLOSE_ALL_SESSIONS,
  INBOUND_OSCORE_EVENT,  /**< inbound network event, payload is encrypted with oscore*/
  OUTBOUND_OSCORE_EVENT, /**< outbound network event, payload is encrypted with oscore*/
  OUTBOUND_GROUP_OSCORE_EVENT, /**< outbound multicast network event, payload is encrypted with oscore*/
  __NUM_OC_EVENT_TYPES__
} oc_events_t;

extern oc_process_event_t oc_events[];

#ifdef __cplusplus
}
#endif

#endif /* OC_EVENTS_H */
