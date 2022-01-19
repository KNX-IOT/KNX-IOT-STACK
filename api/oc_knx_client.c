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
#include "api/oc_knx_client.h"
#include "api/oc_knx_fp.h"

#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>


typedef struct ia_userdata
{
  int ia;
  oc_discover_ia_cb_t cb_func;
} ia_userdata;


static oc_discovery_flags_t
discovery_ia_cb(const char *payload, int len, oc_endpoint_t *endpoint,
             void *user_data)
{
  //(void)anchor;
  (void)payload;
  (void)len;

  PRINT("discovery_ia_cb\n");
  print_ep(endpoint);

  ia_userdata *cb_data = (ia_userdata *)user_data;
  if (cb_data && cb_data->cb_func) {
    PRINT("discovery_ia_cb: calling function\n");
    cb_data->cb_func(cb_data->ia, endpoint);
  }
  if (cb_data) {
    free(user_data);
  }

  return OC_STOP_DISCOVERY;
}


int
oc_knx_client_get_endpoint_from_ia(int ia, oc_discover_ia_cb_t my_func)
{
  char query[20];
  snprintf(query, 20, "if=urn:knx:ia.%d", ia);
  
  ia_userdata * cb_data = (ia_userdata *)malloc(sizeof(ia_userdata));
  cb_data->ia = ia;
  cb_data->cb_func = my_func;

  oc_do_wk_discovery_all(query, 2, discovery_ia_cb, cb_data);
  oc_do_wk_discovery_all(query, 3, discovery_ia_cb, cb_data);
  oc_do_wk_discovery_all(query, 5, discovery_ia_cb, cb_data);
  return 0;
}