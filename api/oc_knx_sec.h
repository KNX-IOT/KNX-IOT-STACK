/*
// Copyright (c) 2021 Cascoda Ltd
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

#ifndef OC_KNX_SEC_INTERNAL_H
#define OC_KNX_SEC_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The token profiles
 * see section 3.5.4.2 Access Token Resource Object
 */
typedef enum {
  OC_PROFILE_UNKNOWN = 0, /**< unknown profile */
  OC_PROFILE_COAP_DTLS,   /**< "coap_dtls" */
  OC_PROFILE_COAP_OSCORE  /**< "coap_oscore" */
} oc_at_profile_t;

oc_at_profile_t oc_string_to_at_profile(oc_string_t str);
char *oc_at_profile_to_string(oc_at_profile_t at_profile);

/**
 * @brief Access Token (at) Information
 *
 * example of (json) payload:
 *{
 * "id": "OC5BLLhkAG ...",
 * "profile": "coap_oscore",
 * "scope": ["if.g.s.<ga>"],
 * "cnf": {
 * "osc": {
 * "alg": "AES-CCM-16-64-128",
 * "id": "<kid>",
 * "ms": "f9af8s.6bd94e6f"
 * }}}
 *
 * scope : "coap_oscore" [OSCORE] or "coap_dtls"
 *
 *  | name      | CBOR key | CBOR type | mandatory  |
 *  |-----------|----------|-----------|------------|
 *  | id        | 0        | string    | yes        |
 *  | profile   | 38       | unsigned  | yes        |
 *  | scope     | 9        | string    | yes        |
 *  | cnf       | 8        | map       | yes        |
 *  | osc       | 4        | map       | oscore     |
 *  | kid       | 2        | string    | optional   |
 *  | nbf       | 5        | integer   | optional   |
 *  | sub       | 2        | string    | conditional |
 *
 *
 * Specific Oscore values
 *
 * https://datatracker.ietf.org/doc/html/draft-ietf-ace-oscore-profile-19#section-3.2.1
 *
 * | name      | CBOR label | CBOR type | description   |default value |
 * | ----------| -----------| ----------|---------------|--------------|
 * | id  | 0    | byte string | OSCORE Input Material Identifier |  - |
 * | version | 1    | unsigned integer | OSCORE   Version   | 1          |
 * | ms | 2    | byte string  | OSCORE Master Secret value (shall be PSK) | - |
 * | hkdf | 3    | text string / integer | HKDF value | HKDF SHA-256  |
 * | alg | 4  | text string / integer | AEAD Algorithm | AES-CCM-16-64-128 (10)|
 * | salt | 5 | byte string | Master Salt | Default SHALL be an empty byte
 *string | | contextId | 6    | byte string | OSCORE ID Context value | omit  |
 *
 * {
 *   "alg" : "AES-CCM-16-64-128",
 *   "id" : b64'AQ=='
 *   "ms" : b64'+a+Dg2jjU+eIiOFCa9lObw'
 * }
 *
 *     Note: maps are not stored.
 */
typedef struct oc_auth_at_t
{
  oc_string_t id;                /**< (0) token id*/
  oc_interface_mask_t interface; /**< (9) the interfaces*/
  oc_at_profile_t profile;       /**< (19) "coap_oscore" or "coap_dtls"*/
  oc_string_t sub;               /**< dtls 2 sub*/
  oc_string_t kid;               /**< dtls 8:2 cnf:kid*/
  oc_string_t osc_id;            /**< oscore cnf::osc::kid*/
  oc_string_t osc_ms;            /**< oscore cnf::osc:ms 4*/
  oc_string_t osc_alg;           /**< oscore cnf::osc:alg*/

  int *ga;    ///< array of integers, for the group objects in the interface
  int ga_len; //< length of the array of ga identifiers
} oc_auth_at_t;

/**
 * @brief set an entry in the auth/at table
 *
 * @param index index in the table, will overwrite if something is there
 * @param entry the auth/at entry
 * @return int 0 == successfull
 */
int oc_core_set_at_table(int index, oc_auth_at_t entry);

/**
 * @brief set shared (SPAKE) key
 * TBD if this remains
 *
 * @param shared_key the master key after SPAKE2 handshake
 * @param shared_key_size the key size
 */
void oc_oscore_set_auth(uint8_t *shared_key, int shared_key_size);

/**
 * @brief delete the /auth/at table
 * will be used in reset of the device
 */
void oc_delete_at_table();

/**
 * @brief retrieve the replay window
 *
 * @return uint64_t the replay window
 */
uint64_t oc_oscore_get_rplwdo();

/**
 * @brief retrieve the oscore sequence number delay value
 *
 * @return uint64_t the osn delay value
 */
uint64_t oc_oscore_get_osndelay();

/**
 * @brief Creation of the KNX security resources.
 *
 *
 * creates the following resources:
 * - /f/oscore
 * - /p/oscore/rplwdo
 * - /p/oscore/osndelay
 * - /auth
 * optional:
 * - a/sen
 *
 * @param device index of the device to which the resources are to be created
 */
void oc_create_knx_sec_resources(size_t device);

/**
 * @brief initialize OSCORE for the device
 *
 * @param device_index The device index
 */
void oc_init_oscore(size_t device_index);

bool oc_knx_contains_interface(oc_interface_mask_t at_interface,
                               oc_interface_mask_t resource_interface);

/**
 * @brief is the method allowed according to the interface mask
 *
 * @param iface_mask the interface mask
 * @param method the method to be checked
 * @return true method allowed
 * @return false method not allowed
 */
bool oc_if_method_allowed_according_to_mask(oc_interface_mask_t iface_mask,
                                            oc_method_t method);

/**
 * @brief check access control based on acl and resource interfaces
 *
 * @param method invocation method for this call
 * @param resource the resource being called
 * @param endpoint the used endpoint
 * @return true has access
 * @return false does not have access
 */
bool oc_knx_sec_check_acl(oc_method_t method, oc_resource_t *resource,
                          oc_endpoint_t *endpoint);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_SEC_INTERNAL_H */
