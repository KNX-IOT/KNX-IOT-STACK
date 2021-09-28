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

#include "oc_api.h"

#include "oc_core_res.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool
is_device_in_mode(int device_index, oc_device_mode_t mode)
{
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_DBG("ERROR in retrieving the device");
    return false;
  }
  if (device->device_mode == mode) {
    return true;
  }

  return false;
}

bool
oc_is_device_mode_in_programming(int device_index)
{
  return is_device_in_mode(device_index, OC_PROGRAMMING_MODE);
}

bool
oc_is_device_mode_in_normal(int device_index)
{
  return is_device_in_mode(device_index, OC_NORMAL_OPERATION);
}

int
oc_device_mode_set_mode(int device_index, oc_device_mode_t mode)
{
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_DBG("ERROR in retrieving the device");
    return -1;
  }

  if (mode != OC_PROGRAMMING_MODE && device->individual_address == 0) {
    OC_DBG("oc_device_mode_set_mode: individual adress = 0, which means that "
           "it is not set");
    return -1;
  }

  device->device_mode = mode;
  return 0;
}

int
oc_device_mode_display(int device_index)
{
  if (oc_is_device_mode_in_programming(device_index)) {
    PRINT(" Device is in programming mode\n");
  }
  if (oc_is_device_mode_in_normal(device_index)) {
    PRINT(" Device is in normal operation mode\n");
  }

  return 0;
}
