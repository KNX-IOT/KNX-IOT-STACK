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
 *  @brief knx iot router implementation
 *
 * @file
 */
#ifndef OC_KNX_GM_INTERNAL_H
#define OC_KNX_GM_INTERNAL_H

#include <stddef.h>
#include "oc_knx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  @defgroup doc_module_tag_iot_router IOT_ROUTER
  Optional group of iot_router functions.

  Currently implemented:
  - /fp/gm  resource
  - /fp/gm/[entry]
  - /f/netip listing:
    - /p/netip/ttl
    - /p/netip/tol
    - /p/netip/mcast
    - /p/netip/key
    - /p/netip/fra
  - register a generic call back to route all s-mode
  messages
  @{
*/

/**
 * @brief Group Mapping Table Resource (/fp/gm)
 * The payload is an array of objects.
 * Example (JSON):
 * ```
 * [
 *    {
 *        "id": "1",
 *        "ga":[2305, 2401],
 *        "dataType" : 1
 *    },
 *    {
 *        "id": 2,
 *        "ga": [2306],
 *        "dataType": 5,
 *        "s": {
 *          "groupkey": "<key>"
 *          "secSettings": {
 *            "a": true,
 *            "c": true
 *          }
 *       }
 *     }
 * ]
 * ```
 *
 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | id       | 0             |
 * | ga       | 7             |
 * | dataType | 116 (t)       |
 * | s        | 115 (s)       |
 * | groupKey | 107           |
 * | secSettings | 28         |
 * | a        | 97 (a)        |
 * | c        | 99 (c)        |
 *
 * The structure stores the information.
 * The structure will be used as an array.
 * There are function to find
 * - empty index in the array
 * - find the index with a specific id
 * - delete an index, e.g. delete the array entry of data (persistent)
 * - make the entry persistent
 * - free the data
 */
typedef struct oc_group_mapping_table_t
{
  int id;       /**< (0) contents of id*/
  int ga_len;   /**< length of the array of ga identifiers*/
  uint64_t *ga; /**< (7) array of group addresses (unsigned 64 bit integers) */
  uint32_t dataType;    /**< (116) dataType */
  oc_string_t groupKey; /**< (s:107) groupKey */
  bool authentication;  /**< (115:28:97) a authentication applied (default true,
                           if  groupKey exists)*/
  bool confidentiality; /**< (115:28:99) c confidentiality applied (default
                           true, if groupKey exists) */
} oc_group_mapping_table_t;

/**
 * @brief Creation of the knx iot router resources
 *
 * @param device_index index of the device to which the resource are to be
 * created
 */
void oc_create_knx_iot_router_resources(size_t device_index);

void oc_create_iot_router_functional_block(size_t device_index);
void oc_core_f_netip_get_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data);

/**
 * @brief delete all entries of the Group Mapping Table (from persistent)
 * storage
 *
 */
void oc_delete_group_mapping_table();

/**
 * @brief returns the size (amount of total entries) of the fp / gm table
 *
 * @return the allocated amount of entries of the group mapping at table
 */
int oc_core_get_group_mapping_table_size();

/**
 * @brief set an entry in the group mapping table
 *
 * Note: does not write to persistent storage
 * @param device_index index of the device
 * @param index the index in the table, will overwrite if something is there
 * @param entry the group mapping entry
 * @param store the store the entry to persistent storage
 * @return int 0 == successful
 */
int oc_core_set_group_mapping_table(size_t device_index, int index,
                                    oc_group_mapping_table_t entry, bool store);

/**
 * @brief retrieve group mapping entry
 *
 * @param device_index the device index
 * @param index the index in the table
 * @return oc_group_mapping_table_t* the group mapping entry
 */
oc_group_mapping_table_t *oc_get_group_mapping_entry(size_t device_index,
                                                     int index);

/**
 * @brief load all entries of the Group Mapping Table (from persistent) storage
 *
 */
void oc_load_group_mapping_table();

/**
 * @brief retrieve the IPv4 sync latency fraction (fra).
 * @param device_index index of the device
 * @return the fra value
 */
int oc_get_f_netip_fra(size_t device_index);

/**
 * @brief retrieve the IPv4 routing latency tolerance (tol)
 *
 * @param device_index index of the device
 * @return the tol value
 */
int oc_get_f_netip_tol(size_t device_index);

/**
 * @brief retrieve the IPv4 routing backbone key
 *
 * @param device_index index of the device
 * @return the key value
 */
oc_string_t oc_get_f_netip_key(size_t device_index);

/**
 * @brief retrieve the value defines how many routers a multicast message MAY
 * pass until it gets discarded. (ttl)
 *
 * @param device_index index of the device
 * @return the ttl value
 */
int oc_get_f_netip_ttl(size_t device_index);
/**
 * @brief retrieve the current IPv4 routing multicast address.(mcast)
 *
 * @param device_index index of the device
 * @return the mcast value
 */
uint32_t oc_get_f_netip_mcast(size_t device_index);

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