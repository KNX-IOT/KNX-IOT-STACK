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
  @brief knx /p resource implementation
  @file

  The properties are implemented as non discoverable resources.
  The same API for data points can be used to create properties.
  The only difference is that the discoverable field is set on "not discoverable"
  All these none discoverable resources are listed under /p
*/
#ifndef OC_KNX_P_INTERNAL_H
#define OC_KNX_P_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@brief Creation of the KNX /p resource.
 * - /p
 * 
 *@param device index of the device to which the resource is to be created
 */
void oc_create_knx_p_resources(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_FB_INTERNAL_H */
