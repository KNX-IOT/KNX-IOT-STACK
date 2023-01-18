/*
// Copyright (c) 2021-2022 Cascoda Ltd
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
  @brief software update
  @file
*/

#ifndef OC_KNX_SWU_INTERNAL_H
#define OC_KNX_SWU_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The software update states
 *
 */
typedef enum {
  OC_SWU_STATE_IDLE = 0,    /**< state is idle */
  OC_SWU_STATE_DOWNLOADING, /**< state is downloading */
  OC_SWU_STATE_DOWNLOADED   /**< state is downloaded */
} oc_swu_state_t;

/**
 * @brief The software result states
 *
 */
typedef enum {
  OC_SWU_RESULT_INIT =
    0, /**< 0 Initial value. Once the updating process is initiated (Download
          /Update), this Resource MUST be reset to Initial value. */
  OC_SWU_RESULT_SUCCESS,   /**< 1 Software updated successfully.*/
  OC_SWU_RESULT_ERR_FLASH, /**< 2 Not enough flash memory for the new software
                              package.*/
  OC_SWU_RESULT_ERR_RAM,   /**< 3 Out of RAM during downloading process*/
  OC_SWU_RESULT_ERR_CONN,  /**< 4 Connection lost during downloading process.*/
  OC_SWU_RESULT_ERR_ICF,   /**< 5 Integrity check failure for new downloaded
                              package.*/
  OC_SWU_RESULT_ERR_UPT,   /**< 6 Unsupported package type.*/
  OC_SWU_RESULT_ERR_URL,   /**< 7 Invalid URL.*/
  OC_SWU_RESULT_ERR_SUF,   /**< 8 Software update failed.*/
  OC_SWU_RESULT_ERR_UP,    /**< 9 Unsupported protocol. */
} oc_swu_result_t;

/**
 * @brief Creation of the KNX software update resources.
 *
 * @param device index of the device to which the resources are to be created
 */
void oc_create_knx_swu_resources(size_t device);

/**
 * @brief set the current firmware package name
 *
 * @param name the name of the firmware package
 */
void oc_swu_set_package_name(char *name);

/**
 * @brief set the current last update time
 *
 * @param time the update time in IETF RFC 3339
 */
void oc_swu_set_last_update(char *time);

/**
 * @brief set the current amount of the bytes written
 *
 * @param package_bytes the amount of bytes written
 */
void oc_swu_set_package_bytes(int package_bytes);

/**
 * @brief Sets the current package version
 *
 * @param major the major number e.g. 1 of [1, 2, 3]
 * @param minor the minor number e.g. 2 of [1, 2, 3]
 * @param minor2 the minor2 number e.g. 3 of [1, 2, 3]
 */
void oc_swu_set_package_version(int major, int minor, int minor2);

/**
 * @brief sets the current download state
 *
 * @param state the download state
 */
void oc_swu_set_state(oc_swu_state_t state);

/**
 * @brief sets the url to be queried for downloading
 *
 * @param qurl the url
 */
void oc_swu_set_qurl(char *qurl);

/**
 * @brief sets the result of the download procedure
 *
 * @param result the result, including possible errors
 */
void oc_swu_set_result(oc_swu_result_t result);

/**
 * Callback invoked by the stack to set the software
 *
 * @param[in] device the device index
 * @param[in] binary_size the full size of the binary
 * @param[in] block_offset the offset (in the file)
 * @param[in] block_data the block data
 * @param[in] block_len the size of the block_data
 * @param[in] data the user supplied data
 *
 */
typedef void (*oc_swu_cb_t)(size_t device, size_t binary_size,
                            size_t block_offset, uint8_t *block_data,
                            size_t block_len, void *data);

/**
 * Set the software update callback.
 *
 * The host name callback is called by the stack when the software update is
 * performed
 *
 * @note oc_set_hostname_cb() must be called before oc_main_init().
 *
 * @param[in] cb oc_swu_cb_t function pointer to be called
 * @param[in] data context pointer that is passed to the oc_restart_cb_t
 *                 the pointer must be a valid pointer till after oc_main_init()
 *                 call completes.
 */
void oc_set_swu_cb(oc_swu_cb_t cb, void *data);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_SWU_INTERNAL_H */
