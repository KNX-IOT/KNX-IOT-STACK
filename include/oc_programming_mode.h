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

/**
 * @brief the device modes
 * the device needs to be in programming mode to set:
 * - internal address (ia)
 * - installation id (iid)
 * - hostname (hname)
 */
typedef enum oc_device_mode_t {

  OC_PROGRAMMING_MODE = 0, /**< device is in programming mode */
  OC_NORMAL_OPERATION      /**< device is in normal operation */
} oc_device_mode_t;

/**
 * @brief checks if the device is in programming mode
 *
 * @param device_index the device index
 * @return true = in programming mode
 * @return false = not in programming mode
 */
bool oc_is_device_mode_in_programming(int device_index);

/**
 * @brief is the device in normal mode (e.g. operational)
 *
 * @param device_index the devcie index
 * @return true = in normal (operational) mode
 * @return false = not in normal mode
 */
bool oc_is_device_mode_in_normal(int device_index);

/**
 * @brief set the device in a specific mode
 *
 * @param device_index the device index
 * @param mode the requested mode
 * @return int 0 = success
 */
int oc_device_mode_set_mode(int device_index, oc_device_mode_t mode);

/**
 * @brief prints the mode to standard out
 *
 * @param device_index the device index
 * @return int 0 == success
 */
int oc_device_mode_display(int device_index);

#ifdef __cplusplus
}
#endif

#endif /* OC_DEVICE_MODE_H */
