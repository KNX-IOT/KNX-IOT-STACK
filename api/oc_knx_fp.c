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
#include "api/oc_knx_fp.h"
#include "oc_core_res.h"
#include <stdio.h>

static void
oc_core_fp_g_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
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
oc_create_fp_g_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_g_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/g", OC_IF_D, APPLICATION_CBOR,
                            OC_DISCOVERABLE, 0, oc_core_fp_g_post_handler, 0, 0,
                            0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_p_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
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
oc_create_fp_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_p_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/p", OC_IF_D, APPLICATION_CBOR,
                            OC_DISCOVERABLE, 0, oc_core_fp_p_post_handler, 0, 0,
                            0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_r_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
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
oc_create_fp_r_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/r", OC_IF_D, APPLICATION_CBOR,
                            OC_DISCOVERABLE, 0, oc_core_fp_r_post_handler, 0, 0,
                            0, 1, "urn:knx:if.c");
}

static void
oc_core_p_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                      void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_JSON) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /*
  int length = clf_add_line_to_buffer("{");
  response_length += length;

  length = clf_add_line_to_buffer("\"api\": { \"version\": \"1.0\",");
  response_length += length;

  length = clf_add_line_to_buffer("\"base\": \"/ \"}");
  response_length += length;

  length = clf_add_line_to_buffer("}");
  response_length += length;
  */

  request->response->response_buffer->content_format = APPLICATION_JSON;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

static void
oc_core_p_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                       void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_JSON) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /*
  int length = clf_add_line_to_buffer("{");
  response_length += length;

  length = clf_add_line_to_buffer("\"api\": { \"version\": \"1.0\",");
  response_length += length;

  length = clf_add_line_to_buffer("\"base\": \"/ \"}");
  response_length += length;

  length = clf_add_line_to_buffer("}");
  response_length += length;
  */

  request->response->response_buffer->content_format = APPLICATION_JSON;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

static void
oc_create_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_p_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/p", OC_IF_LL, APPLICATION_CBOR,
                            OC_DISCOVERABLE, oc_core_p_get_handler, 0,
                            oc_core_p_post_handler, 0, 1, "urn:knx:if.c");
}

/*
void
oc_create_fp_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/fp", OC_IF_LL, APPLICATION_LINK_FORMAT,
                            OC_DISCOVERABLE, oc_core_p_get_handler, 0,
                            oc_core_p_post_handler, 0, 1, "urn:knx:if.c");
}
*/

void
oc_create_knx_fp_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_fp_resources");

  oc_create_fp_g_resource(OC_KNX_FP_G, device_index);
  oc_create_fp_p_resource(OC_KNX_FP_P, device_index);
  oc_create_fp_r_resource(OC_KNX_FP_R, device_index);
  oc_create_p_resource(OC_KNX_P, device_index);
  // oc_create_fp_resource(OC_KNX_FP, device_index);
}
