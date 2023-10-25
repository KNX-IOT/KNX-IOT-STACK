/*
// Copyright (c) 2023 Cascoda Ltd
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

#include "oc_knx_sub.h"
#include "oc_helpers.h"
#include "oc_ri.h"
#include "oc_core_res.h"
#include "oc_api.h"

static void
oc_core_sub_delete_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  //   OC_DBG("oc_core_sub_delete_handler\n");
  (void)iface_mask;
  (void)data;
  oc_send_linkformat_response(request, OC_STATUS_DELETED, 0);
  return;
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(sub, knx_a_sen, 0, "/sub", OC_IF_P,
                                     APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                                     0, 0, 0, oc_core_sub_delete_handler, NULL,
                                     OC_SIZE_ZERO());

void
oc_create_sub_resource(int resource_idx, size_t device)
{
  //   OC_DBG("oc_create_sub_resource\n");
  oc_core_populate_resource(resource_idx, device, "/sub", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE, 0, 0, 0,
                            oc_core_sub_delete_handler, 1);
}