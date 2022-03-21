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
  @file
*/
#ifndef OC_KNX_DEV_INTERNAL_H
#define OC_KNX_DEV_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
@brief Creation of the KNX device resources.
e.g. the / dev / * resources

@param device index of the device to which the resource is to be created
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
 *  reset = 2 (reset all) :
 * 
 * - internal address (ia)
 * - host name (hname)
 * - Installation ID (iid)
 * - programming mode (pm)
 * - group object table
 * - recipient object table
 * - publisher object table
 *
 *  reset = 7 (reset tables):
 *
 * - group object table
 * - recipient object table
 * - publisher object table
 *
 * @param device_index The device index
 * @param reset_mode the KNX reset mode
 */
void oc_knx_device_storage_reset(size_t device_index, int reset_mode);

/**
 * @brief function checks if the device is in programming mode
 * the following resources can be changed when in programming mode
 * - ia
 * - hostname
 * - idd
 *
 * @param device_index the device index
 * @return true in programming mode
 * @return false not in programming mode
 */
bool oc_knx_device_in_programming_mode(size_t device_index);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_DEV_INTERNAL_H */
