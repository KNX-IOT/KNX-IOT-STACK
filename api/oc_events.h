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

#ifndef OC_EVENTS_H
#define OC_EVENTS_H

#include "oc_config.h"
#include "util/oc_process.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  INBOUND_NETWORK_EVENT,
  UDP_TO_TLS_EVENT,
  INIT_TLS_CONN_EVENT,
  RI_TO_TLS_EVENT,
  INBOUND_RI_EVENT,
  OUTBOUND_NETWORK_EVENT,
  OUTBOUND_NETWORK_EVENT_ENCRYPTED,
  TLS_READ_DECRYPTED_DATA,
  TLS_WRITE_APPLICATION_DATA,
  INTERFACE_DOWN,
  INTERFACE_UP,
  TLS_CLOSE_ALL_SESSIONS,
#ifdef OC_OSCORE
  INBOUND_OSCORE_EVENT,
  OUTBOUND_OSCORE_EVENT,
  OUTBOUND_GROUP_OSCORE_EVENT,
#endif /* OC_OSCORE */
  __NUM_OC_EVENT_TYPES__
} oc_events_t;

extern oc_process_event_t oc_events[];

#ifdef __cplusplus
}
#endif

#endif /* OC_EVENTS_H */
