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

#ifndef OC_KNX_CLIENT_INTERNAL_H
#define OC_KNX_CLIENT_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif




typedef void (*oc_discover_ia_cb_t)(int ia, oc_endpoint_t *endpoint);

int oc_knx_client_get_endpoint_from_ia(int ia, oc_discover_ia_cb_t my_func);


#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_CLIENT_INTERNAL_H */
