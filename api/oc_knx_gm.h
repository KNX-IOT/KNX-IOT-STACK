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
  @file
*/
#ifndef OC_KNX_GM_INTERNAL_H
#define OC_KNX_GM_INTERNAL_H

#include <stddef.h>
#include "oc_knx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creation of the gm resources
 *
 * @param device_index index of the device to which the resource are to be
 * created
 */
void oc_create_knx_gm_resources(size_t device_index);

/**
  @defgroup doc_module_tag_gateway Gateway
  Optional group of KNX-IOT to Classic gateway functions.

  Currently implemented: register a generic call back to route all s-mode
  messages
  @{
*/

/**
 * Callback invoked for all s-mode communication
 * e.g. to be used to create a KNX-IOT to CLASSIC gateway
 */
typedef void (*oc_gateway_s_mode_cb_t)(
  size_t device_index, char *sender_ip_address,
  oc_group_object_notification_t *s_mode_message, void *data);

/**
 * @brief The gateway info
 *
 */
typedef struct oc_gateway_t
{
  oc_gateway_s_mode_cb_t cb; /**< the callback */
  void *data;                /**< the callback user data */
} oc_gateway_t;

/**
 * @brief retrieve the gateway info, e.g. the callback and callback data
 *
 * @return oc_gateway_t* the s-mode gateway data
 */
oc_gateway_t *oc_get_gateway_cb(void);

/**
 * Initialize the gateway callbacks.
 *
 * This function is typically called as part of an KNX-IOT to Classic gateway
 *
 * @param[in] oc_gateway_s_mode_cb_t callback function invoked for each received
 * s-mode message.
 * @param[in] data context pointer that is passed to the oc_gateway_s_mode_cb_t
 *
 * @return
 *   - `0` on success
 *   - `-1` on failure
 *
 * @see init
 * @see oc_gateway_s_mode_cb_t
 */
int oc_set_gateway_cb(oc_gateway_s_mode_cb_t oc_gateway_s_mode_cb_t,
                      void *data);

/** @} */ // end of doc_module_tag_gateway

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_GM_INTERNAL_H */