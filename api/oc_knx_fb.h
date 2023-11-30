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
  @brief knx /f resource implementation
  @file

  This module implements the /f and /f/x resource
  The /f resource list all functional blocks.
  The functional blocks will have urls defined as
  `<functionalblocknumber>` (instance 0) or when there are more instances
  as`<functionalblocknumber>_instance`

*/
#ifndef OC_KNX_FB_INTERNAL_H
#define OC_KNX_FB_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief add functional blocks to the response
 *
 * adds the list of functional blocks for /f or ./well-known/core to the
 * response
 *
 * @param request the request
 * @param device_index the device index
 * @param response_length the current response length
 * @param matches number of matches (so far)
 * @param skipped number of entries already skipped
 * @param first_entry first entry to be included
 * @param last_entry last entry to be included (exclusive)
 * @return true
 * @return false
 */
bool oc_add_function_blocks_to_response(oc_request_t *request,
                                        size_t device_index,
                                        size_t *response_length, int *matches,
                                        int *skipped, int first_entry,
                                        int last_entry);

/**
 *@brief Creation of the KNX function block resources.
 * - /fb
 * - /fb/X
 *@param device index of the device to which the resource is to be created
 */
void oc_create_knx_fb_resources(size_t device);

/**
 *@brief count functional blocks in a device
 * @param device_index the device index
 */
int oc_count_functional_blocks(size_t device_index);

/**
 * @brief check if functional blocks should be added to the response
 * @param request the request
 * @return true
 * @return false
 */
bool oc_filter_functional_blocks(oc_request_t *request);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_FB_INTERNAL_H */
