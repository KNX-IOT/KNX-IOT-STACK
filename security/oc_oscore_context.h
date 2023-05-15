/*
// Copyright (c) 2020 Intel Corporation
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

#ifndef OC_OSCORE_CONTEXT_H
#define OC_OSCORE_CONTEXT_H

#include "messaging/coap/oscore_constants.h"
#include "oc_helpers.h"
#include "oc_uuid.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Replay window type definition
 *
 */
typedef struct oc_rwin_t
{
  /**
   * @brief Sender Sequence Number
   */
  uint64_t ssn;
  /**
   * @brief Sender Address, usually the IPv6 source address of the sending
   * device
   */
  uint8_t sender_address[16];
  /**
   * @brief  Destination Address, usually an S-mode multicast address
   */
  uint8_t destination_address[16];
} oc_rwin_t;

/**
 * @brief the oscore context information
 *
 * This is the data for the encryption/decryption
 * the data is created from an auth/at entry.
 */
typedef struct oc_oscore_context_t
{
  struct oc_oscore_context_t
    *next; /**< pointer to the next, NULL if there is not any */
  /* Provisioned parameters */
  int auth_at_index; /**< index of the auth AT table +1, so index = 0 is invalid
                      */
  uint8_t
    token_id[OSCORE_IDCTX_LEN]; /**< Note: the serial number of the device */
  uint8_t master_secret[OSCORE_IDCTX_LEN]; /**< OSCORE master secret [bytes ]*/
  size_t device;                           /**< device index */
  uint8_t sendid[OSCORE_CTXID_LEN];        /**< SID [bytes] */
  uint8_t sendid_len;                      /** length of SID */
  uint8_t recvid[OSCORE_CTXID_LEN];        /**< RID [bytes] */
  uint8_t recvid_len;                      /**< length of RID */
  uint64_t ssn;                            /**< sender sequence number */
  uint8_t idctx[OSCORE_IDCTX_LEN];         /**< OSCORE context */
  uint8_t idctx_len;                       /**< length of OSCORE context */
  oc_string_t desc;                        /**< description */
  /* Derived parameters */
  /* 128-bit keys */
  uint8_t sendkey[OSCORE_KEY_LEN]; /**< derived sender key */
  uint8_t recvkey[OSCORE_KEY_LEN]; /**< derived recipient key */
  /* Common IV */
  uint8_t commoniv[OSCORE_COMMON_IV_LEN];
  /* Replay Window */
  // TODO make the replay window configurable from CMake
  oc_rwin_t rwin[OSCORE_REPLAY_WINDOW_SIZE];
  uint8_t rwin_idx;
} oc_oscore_context_t;

/**
 * @brief creates an OSCORE data
 *
 * @param id the OSCORE identifier
 * @param id_len the length of the OSCORE identifier
 * @param id_ctx the OSCORE context identifier
 * @param id_ctx_len the length of the OSCORE context identifier
 * @param type  the type of context
 *
 * @param secret the OSCORE master secret
 * @param secret_len the length of the OSCORE master secret
 * @param salt the salt to be used
 * @param salt_len the length of the salt
 * @param param the parameters
 * @param param_len the length of the parameters
 *
 * @return true parameters derived (installed, e.g. can be used for
 * encryption/decryption)
 * @return false parameters NOT derived (NOT installed)
 */
int oc_oscore_context_derive_param(const uint8_t *id, uint8_t id_len,
                                   uint8_t *id_ctx, uint8_t id_ctx_len,
                                   const char *type, uint8_t *secret,
                                   uint8_t secret_len, uint8_t *salt,
                                   uint8_t salt_len, uint8_t *param,
                                   uint8_t param_len);

void oc_oscore_free_context(oc_oscore_context_t *ctx);

/**
 * @brief free all OSCORE contexts
 *
 */
void oc_oscore_free_all_contexts();

/**
 * @brief creates an OSCORE context (e.g. the internal structure for
 encoding/decoding)
 *
 * Note: OSCORE context is also a field.
 *
 * @param device the device index
 *
 * @param senderid the SID
 * @param senderid_size the length of SID
 * @param recipientid the RID
 * @param recipientid_size the length of RID
 * @param ssn  the sender sequence number
 * @param desc  the description

 * @param mastersecret the OSCORE master secret
 * @param mastersecret_size the length of the OSCORE master secret
 * @param token_id the token
 * @param token_id_size the length of the token_id
 * @param auth_at_index index in the auth at table -1.
 * @param from_storage initialize ssn from storage
 *
 * @return true parameters derived (installed, e.g. can be used for
 * encryption/decryption)
 * @return false parameters NOT derived (NOT installed)
 */
oc_oscore_context_t *oc_oscore_add_context(
  size_t device, const char *senderid, int senderid_size,
  const char *recipientid, int recipientid_size, uint64_t ssn, const char *desc,
  const char *mastersecret, int mastersecret_size, const char *token_id,
  int token_id_size, int auth_at_index, bool from_storage);

oc_oscore_context_t *oc_oscore_find_context_by_serial_number(
  size_t device, char *serial_number);

oc_oscore_context_t *oc_oscore_find_context_by_group_address(
  size_t device, uint32_t group_address);

oc_oscore_context_t *oc_oscore_find_context_by_kid(oc_oscore_context_t *ctx,
                                                   size_t device, uint8_t *kid,
                                                   uint8_t kid_len);

oc_oscore_context_t *oc_oscore_find_context_by_token_mid(
  size_t device, uint8_t *token, uint8_t token_len, uint16_t mid,
  uint8_t **request_piv, uint8_t *request_piv_len, bool tcp);

oc_oscore_context_t *oc_oscore_find_context_by_oscore_id(size_t device,
                                                         char *oscore_id,
                                                         size_t oscore_id_len);

oc_oscore_context_t *oc_oscore_find_context_by_rid(size_t device, char *rid,
                                                   size_t rid_len);

#ifdef __cplusplus
}
#endif

#endif /* OC_OSCORE_CONTEXT_H */
