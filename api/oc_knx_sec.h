/*
// Copyright (c) 2021-2023 Cascoda Ltd
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
  @brief knx application level security
  @file
*/

#ifndef OC_KNX_SEC_INTERNAL_H
#define OC_KNX_SEC_INTERNAL_H

#include <stddef.h>

#include "oc_ri.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The token profiles
 * see section 3.5.4.2 Access Token Resource Object
 */
typedef enum {
  OC_PROFILE_UNKNOWN = 0,     /**< unknown profile */
  OC_PROFILE_COAP_DTLS = 1,   /**< "coap_dtls" */
  OC_PROFILE_COAP_OSCORE = 2, /**< "coap_oscore" */
  OC_PROFILE_COAP_TLS =
    254, /**< coap_tls" [OSCORE] for [X.509] certificates with TLS */
  OC_PROFILE_COAP_PASE = 255 /**< "coap_pase" [OSCORE] with PASE credentials */
} oc_at_profile_t;

/**
 * @brief string to access token profile
 *
 * @param str input string
 * @return oc_at_profile_t the token profile
 */
oc_at_profile_t oc_string_to_at_profile(oc_string_t str);

/**
 * @brief access token profile to string
 *
 * @param at_profile the access token profile
 * @return char* the string denoting the at access token profile
 */
char *oc_at_profile_to_string(oc_at_profile_t at_profile);

/**
 * @brief Access Token (at) Information
 * payload for a unicast message
 * Example(JSON):
 * ```
 *{
 * "id": "OC5BLLhkAG ...",
 * "profile": "coap_oscore",
 * "aud" : "<iid>.<ia>"
 * "scope": ["if.sec", "if.b"],
 * "cnf": {
 * "osc": {
 * "alg": "AES-CCM-16-64-128", (decimal 10)
 * "id": "<kid>/<sid>",
 * "rid": "SID for response",
 * "ms": "f9af8s.6bd94e6f"
 * }}}
 * ```
 * second example of (JSON) payload for a group address:
 * ```
 *{
 * "id": "OC5BLLhkAG ...",
 * "profile": "coap_oscore",
 * "scope": [0, 1, 2],
 * "cnf": {
 * "osc": {
 * "alg": "AES-CCM-16-64-128",
 * "id": "<kid>/<sid>",
 * "ms": "f9af8s.6bd94e6f"
 * }}}
 * ```
 * scope : "coap_oscore" [OSCORE] or "coap_dtls"
 *
 *  | name      | CBOR key | CBOR type  | mandatory  |
 *  |-----------|----------|------------|------------|
 *  | id        | 0        | string     | yes        |
 *  | profile   | 38       | unsigned   | yes        |
 *  | scope     | 9  | string/int array | yes        |
 *  | cnf       | 8        | map        | yes        |
 *  | osc       | 4        | map        | oscore     |
 *  | kid       | 2        | string     | optional   |
 *  | nbf       | 5        | integer    | optional   |
 *  | sub       | 2        | string     | conditional |
 *
 *
 * Specific oscore values (ACE):
 *
 * https://datatracker.ietf.org/doc/html/draft-ietf-ace-oscore-profile-19#section-3.2.1
 *
 * | name      | CBOR label | CBOR type | description   |default value |
 * | ----------| -----------| ----------|---------------|--------------|
 * | id        | 0      | string      | full ctx identifier |  - |
 * | ms        | 18:4:2 | byte string | Master Secret value (shall be PSK) | - |
 * | version   | 18:4:1 | uint        | OSCORE Version | 1          |
 * | hkdf      | 18:4:3 | integer     | HKDF value | HKDF SHA-256  (-10) |
 * | alg       | 18:4:4 | integer     | AEAD Algorithm | AES-CCM-16-64-128 (10)|
 * | salt      | 18:4:5 | byte string | Master Salt    | Default empty byte
 *string | | contextId | 18:4:6 | byte string | OSCORE ID Context value | omit |
 * | osc_id    | 18:4:0 | byte string | OSCORE SID     | -  |
 *
 * Example payload:
 * ```
 * {
 *   "alg" : "AES-CCM-16-64-128",
 *   "id" : b64'AQ=='
 *   "ms" : b64'+a+Dg2jjU+eIiOFCa9lObw'
 * }
 * ```
 * Note: maps are not stored.
 */
typedef struct oc_auth_at_t
{
  oc_string_t id;            /**< (0) auth/at/{id}, encoding: HEX */
  oc_interface_mask_t scope; /**< (9) the scope (interfaces) */
  oc_at_profile_t
    profile; /**< (38) "coap_oscore" or "coap_dtls", only oscore implemented*/
  oc_string_t aud;         /**< not used anymore, references  */
  oc_string_t sub;         /**< (2) DTLS (not used) 2 sub */
  oc_string_t kid;         /**< (8:2) DTLS (not used)  cnf:kid*/
  oc_string_t osc_version; /**< (18:4:1) OSCORE cnf:osc:version (optional) */
  oc_string_t osc_ms;      /**< (18:4:2) OSCORE cnf:osc:ms (byte string) */
  uint8_t osc_hkdf;        /**< (18:4:3) OSCORE cnf:osc:hkdf (optional-not used)
                              (decimal value)*/
  uint8_t osc_alg;         /**< (18:4:4) OSCORE cnf:osc:alg (optional- not used)
                              default: decimal value 10*/
  oc_string_t
    osc_salt; /**< (18:4:5) OSCORE cnf:osc:salt (optional) empty string */
  oc_string_t osc_contextid;
  /**< (18:4:6) OSCORE cnf:osc:contextid used as "kid_context" (byte string, 6
   * bytes) */
  oc_string_t osc_id; /**< (18:4:0) OSCORE cnf:osc:id  (used as SID & KID) (byte
                         string), max 7 bytes */
  oc_string_t
    osc_rid;   /**< (18:4:7) OSCORE cnf:osc:rid (recipient ID) (byte string) */
  int nbf;     /**< token not valid before (optional) */
  int ga_len;  /**< length of the group addresses (ga) in the scope */
  int64_t *ga; /**< (scope) array of group addresses, for the group objects in
                  the scope, int64_t for framing arrays */
} oc_auth_at_t;

/**
 * @brief returns the size (amount of total entries) of the auth/at table
 *
 * @return the allocated amount of entries of the auth/at table
 */
int oc_core_get_at_table_size();

/**
 * @brief set an entry in the auth/at table
 *
 * Note: does not write to persistent storage
 * @param device_index index of the device
 * @param index the index in the table, will overwrite if something is there
 * @param entry the auth/at entry
 * @param store the store the entry to persistent storage
 * @return int 0 == successful
 */
int oc_core_set_at_table(size_t device_index, int index, oc_auth_at_t entry,
                         bool store);

/**
 * @brief find the entry with context_id as id
 *
 * @param device_index The device index
 * @param context_id the context id to search for
 * @return int -1 : no entry with that name
 * @return int >=0 : index of found entry
 */
int oc_core_find_at_entry_with_context_id(size_t device_index,
                                          char *context_id);

/**
 * @brief Find an entry with a given OSCORE ID
 *
 * @param device_index The device index
 * @param osc_id the oscore ID to search for
 * @param osc_id_len length of the context
 * @return int -1 : no entry with that oscore id
 * @return int >= index of found entry
 */
int oc_core_find_at_entry_with_osc_id(size_t device_index, uint8_t *osc_id,
                                      size_t osc_id_len);

/**
 * @brief find empty slot
 *
 * @param device_index The device index
 * @return int -1 : no space left
 * @return int >=0 : index to place entry
 */
int oc_core_find_at_entry_empty_slot(size_t device_index);

/**
 * @brief set shared (SPAKE) key to the auth at table, on the Management Client
 * side
 *
 * @param client_senderid the client_senderid of the device that has been
 * negotiated with spake2plus. This will become the Receiver ID within the
 * OSCORE context. This value is an ASCII-encoded string representing the
 * hexadecimal serial number
 * @param client_senderid_size the size of the serial number
 * @param clientrecipient_id the clientrecipient_id (delivered during the
 * handshake). This will become the Sender ID. This value is in HEX
 * @param clientrecipient_id_size the size of the clientrecipient_id
 * @param shared_key the master key after SPAKE2 handshake
 * @param shared_key_size the key size
 */
void oc_oscore_set_auth_mac(char *client_senderid, int client_senderid_size,
                            char *clientrecipient_id,
                            int clientrecipient_id_size, uint8_t *shared_key,
                            int shared_key_size);

/**
 * @brief set shared (SPAKE) key to the auth at table, on the Device side
 *
 * @param client_senderid the client_senderid of the device that has been
 * negotiated with spake2plus. This will become the Sender ID within the OSCORE
 * context. This value is an ASCII-encoded string representing the hexadecimal
 * serial number
 * @param client_senderid_size the size of the serial number
 * @param clientrecipient_id the clientrecipient_id (delivered during the
 * handshake). This will become the Receiver ID. This value is in HEX
 * @param clientrecipient_id_size the size of the clientrecipient_id
 * @param shared_key the master key after SPAKE2 handshake
 * @param shared_key_size the key size
 */
void oc_oscore_set_auth_device(char *client_senderid, int client_senderid_size,
                               char *clientrecipient_id,
                               int clientrecipient_id_size, uint8_t *shared_key,
                               int shared_key_size);

/**
 * @brief retrieve auth/at entry
 *
 * @param device_index the device index
 * @param index the index in the table
 * @return oc_auth_at_t* the auth at entry
 */
oc_auth_at_t *oc_get_auth_at_entry(size_t device_index, int index);

/**
 * @brief print the auth/at entry
 *
 * @param device_index the device index
 * @param index the index in the table to be printed
 */
void oc_print_auth_at_entry(size_t device_index, int index);

/**
 * @brief delete the /auth/at table
 * will be used in reset of the device
 *
 * @param device_index the device index
 */
void oc_delete_at_table(size_t device_index);

/**
 * @brief reset the /auth/at table
 * will be used in reset of the device
 *erase_code:
 * - 2 : reset all entries (using oc_delete_at_table())
 * - 7 : reset all entries without scope = "if.sec"
 * @param device_index the device index
 * @param erase_code the erase code
 */
void oc_reset_at_table(size_t device_index, int erase_code);

/**
 * @brief delete the /auth/at table entry
 *
 * @param device_index the device index
 * @param index the index in the table
 * return 0 == success
 */
int oc_at_delete_entry(size_t device_index, int index);

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
 * - /auth/o
 * - /auth/o/rplwdo
 * - /auth/o/osndelay
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
 * Note: does not read the context from storage
 * @param device_index The device index
 */
void oc_init_oscore(size_t device_index);

/**
 * @brief initialize OSCORE for the device
 *
 * @param device_index The device index
 * @param from_storage contents read from storage
 */
void oc_init_oscore_from_storage(size_t device_index, bool from_storage);

/**
 * @brief function to check if the at_interface is listed in the resource
 * interfaces
 *
 * @param at_interface interface to be checked
 * @param resource_interface list of interfaces.
 * @return true one of the at_interface listed in resource_interface list
 * @return false none of the at_interfaces listed in resource_interface list
 */
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
bool oc_knx_sec_check_acl(oc_method_t method, const oc_resource_t *resource,
                          oc_endpoint_t *endpoint);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_SEC_INTERNAL_H */
