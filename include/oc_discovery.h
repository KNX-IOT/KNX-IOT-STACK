/*
// Copyright (c) 2016 Intel Corporation
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
  @brief CoAP Discovery.
  @file
*/
#ifndef OC_DISCOVERY_H
#define OC_DISCOVERY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief create a resource that is discoverable.
 *
 * @param resource_idx the resource index
 * @param device the device to which the resource belongs
 */
void oc_create_discovery_resource(int resource_idx, size_t device);

/**
 * @brief filter resource if it needs to be included in the response of a
 * link-format response
 *
 * @param resource the resource to be included
 * @param request  the request, with all query parameters
 * @param device_index the device index on the request is being made
 * @param response_length the current response length
 * @param matches if there are already resources added to the response
 * @param truncate 1 = do urn truncation; remove the urn part of the rt value
 * @return true resource added (as entry) to the response
 * @return false resource not added to the response
 */
bool oc_filter_resource(oc_resource_t *resource, oc_request_t *request,
                        size_t device_index, size_t *response_length,
                        int matches, int truncate);

/**
 * @brief add the resource to the response in application link format
 *
 * @param resource the resource
 * @param request  the request
 * @param device_index the device index
 * @param response_length the response length (to be increased)
 * @param matches current matches
 * @param truncate 1 = do urn truncation; remove the urn part of the rt value
 * @return true
 * @return false
 */
bool oc_add_resource_to_wk(oc_resource_t *resource, oc_request_t *request,
                           size_t device_index, size_t *response_length,
                           int matches, int truncate);

#ifdef __cplusplus
}
#endif

#endif /* OC_DISCOVERY_H */
