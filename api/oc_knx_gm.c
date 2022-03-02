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

#include "oc_api.h"
#include "api/oc_knx_gm.h"
#include "oc_discovery.h"
#include "oc_core_res.h"
#include <stdio.h>

#ifdef OC_GM_TABLE

static void
oc_core_fp_gm_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_fp_gm_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /* example entry: </fp/gm/1>;ct=60 (cbor)*/
  for (i = 0; i < GAMT_MAX_ENTRIES; i++) {
    if (i > 0) {
      length = oc_rep_add_line_to_buffer(",\n");
      response_length += length;
    }
    if (oc_string_len(g_groups[i].dpt) == 0) {
      // index not in use
      break;
    }

    length = oc_rep_add_line_to_buffer("<fp/gm/");
    response_length += length;
    char string[10];
    sprintf((char *)&string, "%d", i + 1);
    length = oc_rep_add_line_to_buffer(string);
    response_length += length;

    length = oc_rep_add_line_to_buffer(";ct=60");
    response_length += length;
  }

  if (response_length > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_gm_get_handler - end\n");
}

static void
oc_core_fp_gm_post_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  request->response->response_buffer->content_format = APPLICATION_CBOR;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_fp_gm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/fp/gm", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_gm_get_handler, oc_core_fp_gm_post_handler, 0, 0, 0, 1,
    "urn:knx:if.c");
}

static void
oc_core_fp_gm_x_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_gm_x_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  if (value >= GAMT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  // if ( oc_string_len(&g_groups[value - 1].dpt) == 0) {
  // it is empty
  //  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  //  return;
  //}

  oc_rep_begin_root_object();
  // ga- 7
  oc_rep_i_set_int(root, 7, g_groups[value - 1].ga);
  // dpt- 116
  oc_rep_i_set_text_string(root, 116, oc_string(g_groups[value - 1].dpt));
  // note add also classic.

  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);
  return;
}

static void
oc_core_fp_gm_x_del_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_gm_x_del_handler\n");

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  if (value >= GAMT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_free_string(&g_groups[value - 1].dpt);
  oc_new_string(&g_groups[value - 1].dpt, "", 0);

  PRINT("oc_core_fp_gm_x_del_handler - end\n");

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_gm_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/gm/*", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_gm_x_get_handler, 0, 0,
                            oc_core_fp_gm_x_del_handler, 0, 1, "urn:knx:if.c");
}


#endif /* OC_GM_TABLE */

// -----------------------------------------------------------------------------

void
oc_create_knx_gm_resources(size_t device_index)
{
  (void)device_index;
#ifdef OC_GM_TABLE
    OC_DBG("oc_create_knx_gm_resources");

  oc_create_fp_gm_resource(OC_KNX_FP_GM, device_index);
  oc_create_fp_gm_x_resource(OC_KNX_FP_GM_X, device_index);

#endif /* OC_GM_TABLE */
}


// -----------------------------------------------------------------------------

static oc_gateway_t app_gateway = { NULL, NULL };

int 
oc_set_gateway_cb(oc_gateway_s_mode_cb_t cb, void *data)
{
  app_gateway.cb = cb;
  app_gateway.data = data;

  return 0;
}

oc_gateway_t *
oc_get_gateway_cb(void)
{
  return &app_gateway;
}