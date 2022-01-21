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
 *  { "pbkdf2" : { "salt" : "xxxx", "it" : 5}}}
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
 *
 * note no storage needed for map
 */
typedef struct oc_pase_t
{
  oc_string_t salt; ///< salt
  oc_string_t pa;   ///< pa
  oc_string_t pb;   ///< pb
  oc_string_t ca;   ///< ca
  oc_string_t cb;   ///< cb
  oc_string_t rnd;  ///< rnd
  int it;
} oc_pase_t;

/**
 * @brief Group Object Notification
 * Can be used for receiving messages or sending messages.
 *
 *  generic structures:
 *  { 5: { 6: "st value" , 7: "ga value", 1: "value" } }
 *
 *  { 4: "sia", 5: { 6: "st", 7: "ga", 1: "value" } }
 *
 * Key translation
 * | Json Key | Integer Value | type   |
 * | -------- | ------------- |--------|
 * | value    | 1             | object |
 * | sia      | 4             | int    |
 * | s        | 5             | object |
 * | st       | 6             | string |
 * | ga       | 7             | int    |
 */
typedef struct oc_group_object_notification_t
{
  oc_string_t value; ///< generic value received.
  int sia;           ///< (source id) sender individual address
  oc_string_t st;    ///< Service type code (write=w, read=r, response=rp)
  int ga;            ///< group address
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
 * @brief check if the lsm state is loaded
 *
 * @param device index of the device to which the resource is to be created
 */
oc_lsm_state_t oc_knx_lsm_state(size_t device);

/**
 * @brief checks if the load state machine (lsm) string contains a valid value
 *
 * @param lsm The lsm as string
 * @return true correct value
 * @return false incorrect value
 */
bool oc_core_lsm_check_string(const char *lsm);

/**
 * @brief convert the load state machine (lsm) string to lsm state
 *
 * @param lsm The state as string
 * @return oc_lsm_state_t The state as struct
 */
oc_lsm_state_t oc_core_lsm_parse_string(const char *lsm);

/**
 * @brief convert the load state machine (lsm) state to string
 *
 * @param lsm the state
 * @return const char* The state as string
 */
const char *oc_core_get_lsm_as_string(oc_lsm_state_t lsm);

/**
 * @brief sets the ldevid
 *
 * @param ldevid the ldevid certificate
 * @param len the length of the certificate
 */
void oc_knx_set_idevid(const char *idevid, int len);

/**
 * @brief sets the idevid
 *
 * @param idevid the idevid certificate
 * @param len the length of the certificate
 */
void oc_knx_set_ldevid(char *idevid, int len);

/**
 * @brief sets the fingerprint value (of the loaded materials)
 *
 * @param fingerprint The fingerprint value
 */
void oc_knx_set_fingerprint(uint64_t fingerprint);

/**
 * @brief increase the finger print value
 *
 */
void oc_knx_increase_fingerprint();

/**
 * @brief load the fingerprint value from storage
 *
 */
void oc_knx_load_fingerprint();

/**
 * @brief dump the fingerprint value to storage
 *
 */
void oc_knx_dump_fingerprint();

/**
 * @brief sets the oscore sequence number
 *
 * @param osn the oscore sequence number
 */
void oc_knx_set_osn(uint64_t osn);

/**
 * @brief load the state of the device from persistent storage
 * load data for:
 * - load state machine (lsm)
 *
 * @param device_index the device index to load the data for
 */
void oc_knx_load_state(size_t device_index);

/**
 * @brief Creation of the KNX device resources.
 *
 * creates and handles the following resources:
 * - /a/lsm
 * - /.knx
 * - /.well-known/knx
 * - /.well-known/knx/osn
 * - /.well-known/knx/f (fingerprint)
 * - /.well-known/knx/ldevid
 * - /.well-known/knx/idevid
 * - /.well-known/knx/spake
 *
 * @param device index of the device to which the resource is to be created
 *
 */
void oc_create_knx_resources(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_INTERNAL_H */
