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
  @brief knx /dev resource implementation
  @file
*/
#ifndef OC_KNX_DEV_INTERNAL_H
#define OC_KNX_DEV_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KNX_STORAGE_IA "dev_knx_ia"
#define KNX_STORAGE_IID "dev_knx_iid"
#define KNX_STORAGE_FID "dev_knx_fid"

/**
 * @brief Creation of the KNX device resources.
 * e.g. the dev resources:
 *  - sn (serial number)
 *  - hwv (hardware version)
 *  - fwv (firmware version)
 *  - hwt (hardware type)
 *  - model (device model)
 *  - sa (sub address)
 *  - da (device address)
 *  - ipv6 (ipv6 address)
 *  - hname (host name)
 *  - ia (internal address)
 *  - iid (installation identifier)
 *  - port (port address)
 *
 * @param device index of the device to which the resource is to be created
 */
void oc_create_knx_device_resources(size_t device);

/**
@brief read the contents from disk during start up
for the resources implemented / dev / *

@param device index of the device to which the data is to be read
*/
void oc_knx_device_storage_read(size_t device);

/**
 * @brief clear the persistent storage
 * reset behavior according to the supplied erase code
 * - reset = 2 (Factory Reset) :
 *   - internal address (ia)
 *   - host name (hname)
 *   - Installation ID (iid)
 *   - programming mode (pm)
 *   - device address (da)
 *   - sub address (sa)
 *   - group object table
 *   - recipient object table
 *   - publisher object table
 * - reset = 3 (reset ia) :
 *   - internal address (ia)
 * - reset = 7 (Factory Reset without IA):
 *   - group object table
 *   - recipient object table
 *   - publisher object table
 *
 * @param device_index The device index
 * @param reset_mode the KNX reset mode
 */
void oc_knx_device_storage_reset(size_t device_index, int reset_mode);

/**
 * @brief function checks if the device is in programming mode
 *
 * @param device_index the device index
 * @return true in programming mode
 * @return false not in programming mode
 */
bool oc_knx_device_in_programming_mode(size_t device_index);

/**
 * @brief function set the programming mode of the device to true or false
 *
 * @param device_index the device index
 * @param programming_mode true to set the device in programming mode, false
 * otherwise
 */
void oc_knx_device_set_programming_mode(size_t device_index,
                                        bool programming_mode);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_DEV_INTERNAL_H */
