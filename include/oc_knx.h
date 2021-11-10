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

#ifndef OC_KNX_INTERNAL_H
#define OC_KNX_INTERNAL_H

#include <stddef.h>

#include "oc_api.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
 * @brief Pase Resource Object
 *
 *  { "rnd": x}
 *  { "pa": x}
 *  { "pb": x}
 *  { "ca": x}
 *
 *  {
 *    "value": {
 *      "sia": int,
 *      "s": {},
 *      "st": "string",
 *      "ga": int
 *    }
 *  }
 *
 * Key translation
 * | Json Key | Integer Value |  type       |
 * | -------- | ------------- |-------------|
 * | salt     | 5             | byte string |
 * | pa       | 10            | byte string |
 * | pb       | 11            | byte string |
 * | pbkdf2   | 12            | map         |
 * | cb       | 13            | byte string |
 * | ca       | 14            | byte string |
 * | rnd      | 15            | byte string |
 * | it       | 16            | unsigned    |
 */
typedef struct oc_pase_t
{
  oc_string_t salt;
  oc_string_t pa;
  oc_string_t pb;
  oc_string_t ca;
  oc_string_t cb;
  oc_string_t pbkdf2_pa;
  oc_string_t pbkdf2_pb;
  oc_string_t rnd;
  int it;
} oc_pase_t;


/**
 * @brief Group Object Notification
 *
 *  { 5: { 6: "st value" , 7: "ga value", 1: "value" } }
 *
 *  {
 *    "value": {
 *      "sia": int,
 *      "s": {},
 *      "st": "string",
 *      "ga": int
 *    }
 *  }
 *
 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | value    | 1             |
 * | sia      | 4             |
 * | s        | 5             |
 * | st       | 6             |
 * | ga       | 7             |
 */
typedef struct oc_group_object_notification_t
{
  oc_string_t value;
  int sia;
  oc_string_t s;
  oc_string_t st;
  int ga;
} oc_group_object_notification_t;

/**
 * @brief LSM state machine values
 *
 */
typedef enum {
  LSM_UNLOADED = 0, ///< state is unloaded, e.g. ready for loading
  LSM_LOADED,       ///< state is LOADED, e.g. normal operation
  LSM_lOADCOMPLETE, ///< cmd loading complete, state will be LOADED
  LSM_STARTLOADING, ///< cmd loading started, state will be LOADING
  LSM_LOADING,      ///< state loading.
  LSM_UNLOAD        ///< cmd unload: state will be UNLOADED
} oc_lsm_state_t;

/**
@brief check if the lsm state is loaded

@param device index of the device to which the resource is to be created
*/
oc_lsm_state_t oc_knx_lsm_state(size_t device);

bool oc_core_lsm_check_string(const char *lsm);

oc_lsm_state_t oc_core_lsm_parse_string(const char *lsm);

const char *oc_core_get_lsm_as_string(oc_lsm_state_t lsm);

/**
@brief Creation of the KNX device resources.

@param device index of the device to which the resource is to be created
*/
void oc_create_knx_resources(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_INTERNAL_H */
