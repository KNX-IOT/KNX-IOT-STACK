/*
// Copyright (c) 2019 Intel Corporation
// Copyright (c) 2022 Cascoda Ltd.
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

#ifndef OC_MAIN_H
#define OC_MAIN_H

#include "oc_api.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief The factory presets info
 *
 */
typedef struct oc_factory_presets_t
{
  oc_factory_presets_cb_t cb; /**< the callback */
  void *data;                 /**< the callback user data */
} oc_factory_presets_t;

/**
 * @brief retrieve the factory reset info, e.g. the callback and callback data
 * 
 * @return oc_factory_presets_t* the preset data
 */
oc_factory_presets_t *oc_get_factory_presets_cb(void);

/**
 * @brief The reset info
 *
 */
typedef struct oc_reset_t
{
  oc_reset_cb_t cb;/**< the callback */
  void *data;/**< the callback user data */
} oc_reset_t;

/**
 * @brief retrieve the reset info, e.g. the callback and callback data
 * 
 * @return oc_reset_t* the reset data
 */
oc_reset_t *oc_get_reset_cb(void);

/**
 * @brief The restart info
 *
 */
typedef struct oc_restart_t
{
  oc_restart_cb_t cb;/**< the callback */
  void *data;/**< the callback user data */
} oc_restart_t;

/**
 * @brief retrieve the restart info, e.g. the callback and callback data
 * 
 * @return oc_restart_t* the restart data
 */
oc_restart_t *oc_get_restart_cb(void);

/**
 * @brief The hostname info
 *
 */
typedef struct oc_hostname_t
{
  oc_hostname_cb_t cb;/**< the callback */
  void *data; /**< the callback user data */
} oc_hostname_t;

/**
 * @brief retrieve the hostname info, e.g. the callback and callback data
 * 
 * @return oc_hostname_t* the hostname info
 */
oc_hostname_t *oc_get_hostname_cb(void);

/**
 * @brief is main initialized
 * 
 * @return true 
 * @return false 
 */
bool oc_main_initialized(void);

/**
 * Set acceptance of new commands(GET/PUT/POST/DELETE) for logical device
 *
 * The device drops/accepts new commands when the drop is set to true/false.
 *
 * @note If OC_SECURITY is set, this call is used to drop all new incoming
 *       commands during closing TLS sessions (CLOSE_ALL_TLS_SESSIONS).
 *
 * @param[in] device index of the logical device
 * @param[in] drop set whether all new commands will be accepted/dropped
 */
void oc_set_drop_commands(size_t device, bool drop);

/**
 * Get status of dropping of logical device.
 *
 * @param[in] device the index of the logical device
 *
 * @return true if the device dropping new commands
 */
bool oc_drop_command(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_MAIN_H */
