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
/**
  @brief implementation of /.well-known/knx and /k resources
  @file
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
 * Example Json:
 * ```
 *  { "rnd": x}
 *  { "pa": x}
 *  { "pb": x}
 *  { "ca": x}
 *  { "pbkdf2" : { "salt" : "xxxx", "it" : 5}}}
 * ```
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
  oc_string_t id;   /**< recipient id */
  uint8_t salt[32]; /**< salt */
  uint8_t pa[65];   /**< pa */
  uint8_t pb[65];   /**< pb */
  uint8_t ca[32];   /**< ca */
  uint8_t cb[32];   /**< cb */
  uint8_t rnd[32];  /**< rnd */
  int it;
} oc_pase_t;

/**
 * @brief Group Object Notification (s-mode messages)
 * Can be used for receiving messages or sending messages.
 *
 *  generic structures:
 * ```
 *  { 5: { 6: "st value" , 7: "ga value", 1: "value" } }
 *
 *  { 4: "sia", 5: { 6: "st", 7: "ga", 1: "value" } }
 * ```
 *
 * Key translation
 * | Json Key | Integer Value | type     |
 * | -------- | ------------- |----------|
 * | value    | 1             | object   |
 * | sia      | 4             | uint32_t |
 * | s        | 5             | object   |
 * | st       | 6             | string   |
 * | ga       | 7             | uint32_t |
 */
typedef struct oc_group_object_notification_t
{
  oc_string_t value; /**< generic value received. */
  uint32_t sia;      /**< (source id) sender individual address */
  oc_string_t st;    /**< Service type code (write=w, read=r, response=rp) */
  uint32_t ga;       /**< group address */
} oc_group_object_notification_t;

/**
 * @brief LSM state machine values
 *
 */
typedef enum oc_lsm_state {
  LSM_S_UNLOADED = 0,      /**< (0) state is unloaded, e.g. ready for loading */
  LSM_S_LOADED = 1,        /**< (1) state is LOADED, e.g. normal operation */
  LSM_S_LOADING = 2,       /**< (2) state loading. */
  LSM_S_UNLOADING = 4,     /**< (4) state unloading loading. */
  LSM_S_LOADCOMPLETING = 5 /**< (5) cmd unload: state will be UNLOADED */
} oc_lsm_state_t;

/**
 * @brief LSM event values
 *
 */
typedef enum {
  LSM_E_NOP = 0, /**< (0) No operation */
  LSM_E_STARTLOADING =
    1, /**< (1) Request to start the loading of the loadable part */
  LSM_E_LOADCOMPLETE = 2, /**< (2) cmd loading complete, state will be LOADED */
  LSM_E_UNLOAD = 4        /**< (4) cmd unload: state will be UNLOADED */
} oc_lsm_event_t;

/**
 * @brief retrieve the current lsm state
 *
 * @param device_index index of the device to which the resource is to be
 * created
 * @return the lsm state
 */
oc_lsm_state_t oc_a_lsm_state(size_t device_index);

/**
 * @brief retrieve the current lsm state
 *
 * @param device_index index of the device to which the resource is to be
 * created
 * @param new_state the new lsm_state
 * @return 0 == success
 */
int oc_a_lsm_set_state(size_t device_index, oc_lsm_event_t new_state);

/**
 * @brief convert the load state machine (lsm) event to string
 *
 * @param lsm_e the event
 * @return const char* The state as string
 */
const char *oc_core_get_lsm_event_as_string(oc_lsm_event_t lsm_e);

/**
 * @brief convert the load state machine (lsm) state to string
 *
 * @param lsm_s the state
 * @return const char* The state as string
 */
const char *oc_core_get_lsm_state_as_string(oc_lsm_state_t lsm_s);

/**
 * Callback invoked by the stack to inform the change of the lsm
 *
 * @param[in] device the device index
 * @param[out] lsm_state the new state of the lsm
 * @param[in] data the user supplied data
 *
 */
typedef void (*oc_lsm_change_cb_t)(size_t device, oc_lsm_state_t lsm_state,
                                   void *data);

/**
 * Set the load state machine change callback.
 *
 * The callback is called by the stack when lsm is changed
 *
 * @note oc_set_hostname_cb() must be called before oc_main_init().
 *
 * @param[in] cb oc_hostname_cb_t function pointer to be called
 * @param[in] data context pointer that is passed to the oc_restart_cb_t
 *                 the pointer must be a valid pointer till after oc_main_init()
 *                 call completes.
 */
void oc_set_lsm_change_cb(oc_lsm_change_cb_t cb, void *data);

bool oc_s_mode_notification_to_json(
  char *buffer, size_t buffer_size,
  oc_group_object_notification_t notification);

/**
 * @brief checks if the device is in "run-time" mode
 * run-time is:
 * - ia initialized (e.g. larger than 0)
 * - iid initialized (e.g. larger than 0)
 * - load state machine (lsm) == loaded
 *
 * @param device_index The device index.
 * @return true in runtime
 * @return false not in run time
 */
bool oc_is_device_in_runtime(size_t device_index);

/**
 * @brief sets the idevid
 *
 * @param idevid the idevid certificate
 * @param length the length of the certificate
 */
void oc_knx_set_idevid(const char *idevid, int length);

/**
 * @brief sets the ldevid
 *
 * @param ldevid the ldevid certificate
 * @param len the length of the certificate
 */
void oc_knx_set_ldevid(char *ldevid, int len);

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
 * @brief load the state of the device from persistent storage
 * load data for:
 * - load state machine (lsm)
 *
 * @param device_index the device index to load the data for
 */
void oc_knx_load_state(size_t device_index);

/**
 * @brief reset the device
 * the reset value according to the specification:
 * - reset = 2 (Factory Reset) :
 *   - individual address (ia)
 *   - host name (hname)
 *   - Installation ID (iid)
 *   - programming mode (pm)
 *   - device address (da)
 *   - sub address (sa)
 *   - group object table
 *   - recipient object table
 *   - publisher object table
 * - reset = 3 (reset ia) :
 *   - individual address (ia)
 * - reset = 7 (Factory Reset without IA):
 *   - group object table
 *   - recipient object table
 *   - publisher object table
 *
 * @see oc_knx_device_storage_reset
 * @param device_index the device index
 * @param reset_value the reset value
 * @return int 0== success
 */
int oc_reset_device(size_t device_index, int reset_value);

/**
 * @brief Creation of the KNX device resources.
 *
 * creates and handles the following resources:
 * - /a/lsm
 * - /k
 * - /.well-known/knx
 * - /.well-known/knx/osn
 * - /.well-known/knx/f (fingerprint)
 * - /.well-known/knx/ldevid (optional)
 * - /.well-known/knx/idevid (optional)
 * - /.well-known/knx/spake
 *
 * @param device index of the device to which the resource is to be created
 *
 */
void oc_create_knx_resources(size_t device);

/**
 * @brief runtime switch to ignore s-mode messages send by ourselves.
 * The /k resource handles all incoming s-mode messages.
 * This switch can be used to ignore s-mode messages that are send by ourselves.
 *
 * note for the sniffer, all messages must be handled/reported.
 * note for a Classic to IoT gateway the self send messages must be ignored
 *
 * @param ignore true: ignore, false handle message
 *
 */
int oc_knx_knx_ignore_smessage_from_self(bool ignore);

#ifdef OC_SPAKE
/**
 * @brief Initialise the RNG used for SPAKE2+ and global data structures
 *
 */
void oc_initialise_spake_data(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_INTERNAL_H */
