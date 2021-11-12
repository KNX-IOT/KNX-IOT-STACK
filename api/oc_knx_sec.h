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
 * @brief The software update states
 *
 */
typedef enum {
  OC_PROFILE_UNKNOWN = 0,    /**< unknown profile */
  OC_PROFILE_COAP_OSCORE,    /**< coap_oscore */
  OC_PROFILE_COAP_DTLS       /**< coap_dtls */
} oc_cc_profile_t;

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
 * Key translation
 * | Json Key          | Integer Value | type    |
 * | ----------------- | ------------- |---------|
 * | access_token (id) | 0             | object  |
 * | profile           | 38            | string  |
 * | scope             | 9             | object  |
 * | cnf               | 8             | map     |
 * | osc               | 4             | map     |
 * | alg               | 4             | int     |
 * | id                | 0             | string  |
 * | ms                | x             | int     |
 * | kid               | 2             | string  |
 * | nbf  (optional)   | 5             | integer |
 * | sub               | 2             | string  |
 * | ms  (bytestring)  | 2             | string  |
 */
typedef struct oc_oscore_cc_t
{
  oc_string_t at;
  oc_cc_profile_t profile;
  int *scope;
  int sia;
  oc_string_t alg;
  oc_string_t ms;
} oc_oscore_cc_t;

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

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_SEC_INTERNAL_H */
