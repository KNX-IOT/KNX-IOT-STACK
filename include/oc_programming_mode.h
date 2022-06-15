/*
// Copyright (c) 2021-2022 Cascoda Ltd
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
  @brief programming mode code
  @file
 */
#ifndef OC_DEVICE_MODE_H
#define OC_DEVICE_MODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief checks if the device is in programming mode
 *
 * @param device_index the device index
 * @return true = in programming mode
 * @return false = not in programming mode
 */
bool oc_is_device_mode_in_programming(size_t device_index);

#ifdef __cplusplus
}
#endif

#endif /* OC_DEVICE_MODE_H */
