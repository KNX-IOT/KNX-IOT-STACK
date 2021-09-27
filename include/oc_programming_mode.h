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
 * @file
 * Device status
 *

 */
#ifndef OC_DEVICE_MODE_H
#define OC_DEVICE_MODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum oc_device_mode_t {
	
  OC_PROGRAMMING_MODE = 0,   ///< device is in programming mode
  OC_NORMAL_OPERATION        ///< device is in normal operation
} oc_device_mode_t;


bool oc_is_device_mode_in_programming(int device_index);

bool oc_is_device_mode_in_normal(int device_index);

int oc_device_mode_set_mode(int device_index, oc_device_mode_t mode);

int oc_device_mode_display(int device_index);

#ifdef __cplusplus
}
#endif

#endif /* OC_DEVICE_MODE_H */
