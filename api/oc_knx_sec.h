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
 *
 */
typedef enum {
  OC_PROFILE_UNKNOWN = 0, /**< unknown profile */
  OC_PROFILE_COAP_OSCORE, /**< "coap_oscore" */
  OC_PROFILE_COAP_DTLS    /**< 2coap_dtls" */
} oc_at_profile_t;

oc_at_profile_t oc_string_to_at_profile(oc_string_t str);
char *oc_at_profile_to_string(oc_at_profile_t at_profile);

/**
 * @brief Oscore Credential Configuration
 *
 *{
 * "access_token": "OC5BLLhkAG ...",
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
 *  https://www.iana.org/assignments/cwt/cwt.xhtml#confirmation-methods
 *  https://datatracker.ietf.org/doc/html/rfc8392#section-4
 *
 * | Name | Key | Value Type                       |
 * |------|-----|----------------------------------|
 * | access_token (id) | 0   | text string  (max 32 byte)   |
 * | iss  | 1   | text string                      |
 * | sub  | 2   | text string                      |
 * | aud  | 3   | text string                      |
 * | exp  | 4   | integer or floating-point number |
 * | nbf  | 5   | integer or floating-point number |
 * | iat  | 6   | integer or floating-point number |
 * | cti  | 7   | byte string                      |
 * | cnf  | 8   | map                              |
 * | scope | 9  | byte string                      |
 *
 * example of https://www.rfc-editor.org/rfc/rfc8747.html
 *{
 * /iss/ 1 : "coaps://server.example.com",
 * /aud/ 3 : "coaps://client.example.org",
 * /exp/ 4 : 1879067471,
 * /cnf/ 8 :{
 *  /COSE_Key/ 1 :{
 *     /kty/ 1 : /EC2/ 2,
 *     /crv/ -1 : /P-256/ 1,
 *     /x/ -2 : h'd7cc072de2205bdc1537a543d53c60a6acb62eccd890c7fa27c9
 *                e354089bbe13',
 *     /y/ -3 : h'f95e1d4b851a2cc80fff87d8e23f22afb725d535e515d020731e
 *                79a3b4e47120'
 *    }
 *  }
 *}
 *
 * | Name     | CBOR Key | Value Type | Usage                  |
 * |----------|----------|------------|------------------------|
 * | cnf      | TBD (8)  | map        | token response         |
 * | cnf      | TBD (8)  | map        | introspection response |
 *
 *     Note, maps are not stored.
 */
typedef struct oc_auth_at_t
{
  oc_string_t id;                //!< (0) token id
  oc_interface_mask_t interface; //!< (9) the interfaces
  oc_at_profile_t profile;       //!< (19) "coap_oscore" or "coap_dtls"
  oc_string_t dnsname;           //!< dtls 2:x sub::dnsname
  oc_string_t kty;               //!< dtls 8:x cnf:kty
  oc_string_t kid;               //!< dtls 8:2 cnf:kid
  oc_string_t osc_id;            //!< oscore cnf::osc::kid
  oc_string_t osc_ms;            //!< oscore cnf::osc:ms 4
  oc_string_t osc_alg;           //!< oscore cnf::osc:alg

  int *ga;    ///< array of integers, for the group objects in the interface
  int ga_len; //< length of the array of ga identifiers
} oc_auth_at_t;

/**
 * Oscore profile
 *
 * https://datatracker.ietf.org/doc/html/draft-ietf-ace-oscore-profile-19#section-3.2.1
 *
 * | name      | CBOR label | CBOR type | description                      |
 * | ----------| -----------| ----------| ---------------------------------|
 * | id        | 0    | byte string | OSCORE Input Material Identifier     |
 * | version   | 1    | unsigned integer | OSCORE   Version                |
 * | ms        | 2    | byte string  | OSCORE Master Secret value          |
 * | hkdf      | 3    | text string / integer | OSCORE HKDF value          |
 * | alg       | 4    | text string / integer | OSCORE AEAD Algorithm value |
 * | salt      | 5    | byte string | an input to  OSCORE Master Salt value|
 * | contextId | 6    | byte string | OSCORE ID Context value              |
 *
 * {
 *   "alg" : "AES-CCM-16-64-128",
 *   "id" : b64'AQ=='
 *   "ms" : b64'+a+Dg2jjU+eIiOFCa9lObw'
 * }
 *
 *     Note, maps are not stored.
 */
typedef struct oc_oscore_profile_t
{
  oc_string_t id; // kid???
  int version;
  oc_string_t ms;
  oc_string_t hkdf;
  oc_string_t alg;
  oc_string_t salt;
  oc_string_t contextId;
} oc_oscore_profile_t;

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

bool oc_knx_sec_check_acl(oc_method_t method, oc_resource_t *resource,
                      oc_endpoint_t *endpoint);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_SEC_INTERNAL_H */
