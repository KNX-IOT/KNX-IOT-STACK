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

#ifndef OC_KNX_FP_INTERNAL_H
#define OC_KNX_FP_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
@brief Creation of the KNX feature point resources.

@param device index of the device to which the resource are to be created
*/
void oc_create_knx_fp_resources(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_FP_INTERNAL_H */
