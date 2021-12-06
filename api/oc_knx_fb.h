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
 * @return true
 * @return false
 */
bool oc_add_function_blocks_to_response(oc_request_t *request,
                                        size_t device_index,
                                        size_t *response_length, int matches);

/**
 *@brief Creation of the KNX function block resources.
 * - /fb
 * - /fb/X
 *@param device index of the device to which the resource is to be created
 */
void oc_create_knx_fb_resources(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_FB_INTERNAL_H */
