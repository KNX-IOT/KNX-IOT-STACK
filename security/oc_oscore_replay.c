/*
// Copyright (c) 2022-2023 Cascoda Ltd
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

#include "oc_endpoint.h"
#include "oc_oscore_replay.h"

#ifdef OC_OSCORE

static struct
{
  oc_endpoint_t endpoint;
  uint16_t sequence_number;
  bool in_use;
} sn_table[OC_MAX_RX_SEQUENCE_NUMBERS] = { 0 };

int
oc_oscore_replay_add_endpoint(const oc_endpoint_t *endpoint)
{
  for (int i = 0; i < OC_MAX_RX_SEQUENCE_NUMBERS; i++) {
    if (!sn_table[i].in_use) {
      sn_table[i].endpoint = *endpoint;
      // ensure no potentially invalid pointers are saved
      sn_table[i].endpoint.next = NULL;
      sn_table[i].sequence_number = 0;
      sn_table[i].in_use = true;
      return 0;
    }
  }
  return 1;
}

int
oc_oscore_replay_delete_endpoint(const oc_endpoint_t *endpoint)
{
  for (int i = 0; i < OC_MAX_RX_SEQUENCE_NUMBERS; i++) {
    if (sn_table[i].in_use) {
      if (oc_endpoint_compare_address(endpoint, &sn_table[i].endpoint) == 0) {
        sn_table[i].in_use = 0;
        return 0;
      }
    }
  }
  return 1;
}

int
oc_oscore_replay_get_sequence_number(const oc_endpoint_t *endpoint,
                                     uint16_t *sequence_number)
{
  for (int i = 0; i < OC_MAX_RX_SEQUENCE_NUMBERS; i++) {
    if (sn_table[i].in_use) {
      if (oc_endpoint_compare_address(endpoint, &sn_table[i].endpoint) == 0) {
        *sequence_number = sn_table[i].sequence_number;
        return 0;
      }
    }
  }
  return 1;
}

int
oc_oscore_replay_update_sequence_number(const oc_endpoint_t *endpoint,
                                        uint16_t sequence_number)
{
  for (int i = 0; i < OC_MAX_RX_SEQUENCE_NUMBERS; i++) {
    if (sn_table[i].in_use) {
      if (oc_endpoint_compare_address(endpoint, &sn_table[i].endpoint) == 0) {
        sn_table[i].sequence_number = sequence_number;
        return 0;
      }
    }
  }
  return 1;
}

#endif