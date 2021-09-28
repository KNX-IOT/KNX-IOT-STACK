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
#include "api/oc_knx.h"
#include "oc_core_res.h"
#include <stdio.h>

static void
oc_core_knx_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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
oc_core_knx_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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

void
oc_create_knx_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_LL,
                            OC_DISCOVERABLE, oc_core_knx_get_handler, 0,
                            oc_core_knx_post_handler, 0, 0, "");
}


static void
oc_core_knx_reset_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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

void
oc_create_knx_reset_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_reset_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/reset",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_LL,
                            OC_DISCOVERABLE, 0, 0,
                            oc_core_knx_reset_post_handler, 0, 0, "");
}

static void
oc_core_knx_lsm_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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
oc_core_knx_lsm_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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

void
oc_create_knx_lsm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_lsm_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/lsm",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_LL,
                            OC_DISCOVERABLE, oc_core_knx_lsm_get_handler, 0,
                            oc_core_knx_lsm_post_handler, 0, 0, "");
}

static void
oc_core_knx_crc_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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
oc_core_knx_crc_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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

void
oc_create_knx_crc_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_crc_lsm_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/crc",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_LL,
                            OC_DISCOVERABLE, oc_core_knx_crc_get_handler, 0,
                            oc_core_knx_crc_post_handler, 0, 0, "");
}


static void
oc_core_knx_ldevid_get_handler(oc_request_t *request,
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
oc_create_knx_ldevid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_iid_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/ldevid", OC_IF_D, OC_IF_D,
                            OC_DISCOVERABLE, oc_core_knx_ldevid_get_handler,
                            0, 0, 0, 0, 1,
                            ":dpt.a[n]");
}

static void
oc_core_knx_idevid_get_handler(oc_request_t *request,
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
oc_create_knx_idevid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_iid_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/idevid", OC_IF_D, OC_IF_D, OC_DISCOVERABLE,
    oc_core_knx_idevid_get_handler, 0, 0, 0, 0, 1, ":dpt.a[n]");
}

void
oc_create_knx_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_resources");
  
  oc_create_knx_reset_resource(OC_KNX, device_index);
  oc_create_knx_resource(OC_KNX_RESET, device_index);
  oc_create_knx_lsm_resource(OC_KNX_LSM, device_index);
  oc_create_knx_crc_resource(OC_KNX_CRC, device_index);
  oc_create_knx_ldevid_resource(OC_KNX_LDEVID, device_index);
  oc_create_knx_idevid_resource(OC_KNX_IDEVID, device_index);
}
