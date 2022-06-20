/*
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
/**
  @brief security: oscore replay
  @file

  Description of the replay window implemented algorithm.

 The KNX servers keep a list endpoints that they have received a
'synchronised' message from. Upon boot, this list endpoints is empty, so servers
will respond to requests from all new client endpoints with 4.01 UNAUTHORISED
message containing an Echo option. The echo option is OSCORE-encrypted, and its
actual value is the local time of the server. Upon receiving such a response,
the client retransmits the request and includes the Echo value that the server
sent. This verifies that:

    a) the client is reachable at the source IP address, preventing attackers
from attempting to bypass the deduplication code by changing the source IP
address of packets b) the request is fresh - the server drops request where the
timestamp contained in the Echo option is older than a given threshold,
configurable within engine.c

This is all transparent to the higher layers - the 4.01 UNAUTHORISED does not
reach the client callback. The only observable side-effect is that the first
request sent to a 'new' server will have a slightly longer latency: twice the
round-trip time, as opposed to just once.

*/


#ifndef OC_OSCORE_REPLAY_H
#define OC_OSCORE_REPLAY_H

#ifdef OC_OSCORE
#define OC_MAX_RX_SEQUENCE_NUMBERS 30

/**
 * @brief Add an endpoint to the table of sequence numbers.
 *
 * The newly created endpoint will be stored with sequence number 0.
 * This function saves a copy of endpoint - the pointer need not be valid
 * after this function returns.
 *
 * @param endpoint The endpoint to be saved
 * @return int 0 on success, 1 if the table of endpoints is full.
 */
int oc_oscore_replay_add_endpoint(const oc_endpoint_t *endpoint);

/**
 * @brief Delete an endpoint from the table of sequence numbers.
 *
 * @param endpoint The endpoint to be deleted. Uses oc_endpoint_compare_address
 * for the comparison
 * @return int 0 upon successful deletion, 1 if the endpoint was not found.
 */
int oc_oscore_replay_delete_endpoint(const oc_endpoint_t *endpoint);

/**
 * @brief Get the sequence number for an endpoint.
 *
 * @param endpoint Endpoint to search for. Uses oc_endpoint_compare_address.
 * @param sequence_number Pointer to sequence number variable to be filled in.
 * @return int 0 on success, 1 if the endpoint was not found.
 */
int oc_oscore_replay_get_sequence_number(const oc_endpoint_t *endpoint,
                                         uint16_t *sequence_number);

/**
 * @brief Update an existing endpoint with a new sequence number.
 *
 * @param endpoint Endpoint to search for. Uses oc_endpoint_compare_address
 * @param sequence_number The new value of the sequence number
 * @return int 0 on success, 1 if the endpoint was not found.
 */
int oc_oscore_replay_update_sequence_number(const oc_endpoint_t *endpoint,
                                            uint16_t sequence_number);
#endif
#endif