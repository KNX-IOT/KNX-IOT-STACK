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

#ifndef OC_KNX_INTERNAL_H
#define OC_KNX_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LSM state machine values
 *
 */
typedef enum {
  LSM_UNLOADED = 0,   ///< state is unloaded, e.g. ready for loading
  LSM_LOADED,         ///< state is LOADED, e.g. normal operation
  LSM_lOADCOMPLETE,   ///< command loading complete, state will be LOADED
  LSM_STARTLOADING,   ///< command loading started, state will be LOADING
  LSM_LOADING,        ///< state loading.
  LSM_UNLOAD          ///< command unload: state will be UNLOADED
} oc_lsm_state_t;


/**
@brief Creation of the KNX device resources.

@param device index of the device to which the resource is to be created
*/
void oc_create_knx_resources(size_t device);

/**
@brief function to retrieve the loading state

@param device index of the device to which the resource is to be created
 * @return
 *  - the loading state
 *  - note unloaded is returned if the device is incorrect.
*/
oc_lsm_state_t oc_knx_lsm_state(size_t device);


#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_INTERNAL_H */
