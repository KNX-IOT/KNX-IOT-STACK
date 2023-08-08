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
#ifndef OC_REPLAY_H
#define OC_REPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "oc_helpers.h"

/**
 * @brief Add a synchronised client
 *
 * If a client with the same KID & KID_CTX already exists, it will be
 * updated and marked as in sync
 *
 * @param rx_ssn Sender Sequence Number of newly received OSCORE request
 * @param rx_kid Key Identifier of received request
 * @param rx_kid_ctx Key ID Context of received request
 */
void oc_replay_add_client(uint64_t rx_ssn, oc_string_t rx_kid,
                          oc_string_t rx_kid_ctx);

/**
 * @brief Check if a client is synchronised
 *
 * If the client is synchronised, this function also updates its entry with the
 * new SSN. Thus, the replay window is updated 'in the background', through the
 * natural usage of this function
 *
 * @param rx_ssn Sender Sequence Number of newly received OSCORE request
 * @param rx_kid Key Identifier of received request
 * @param rx_kid_ctx Key ID Context of received request
 * @return true Client is synchronised, you may accept the frame with the given
 * SSN
 * @return false Client is not synchronised, you must challenge the frame
 */
bool oc_replay_check_client(uint64_t rx_ssn, oc_string_t rx_kid,
                            oc_string_t rx_kid_ctx);

/**
 * @brief Free all clients with a given KID. Should be used whenever the
 * corresponding access token is deleted
 *
 * @param rx_kid the KID
 */
void oc_replay_free_client(oc_string_t rx_kid);

#ifdef __cplusplus
}
#endif

#endif // OC_REPLAY_H