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
#include "api/oc_knx_swu.h"
#include "oc_core_res.h"
#include <stdio.h>

static void
oc_knx_swu_protocol_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  
  oc_send_json_response(request, OC_STATUS_OK);
}

static void
oc_knx_swu_protocol_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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
  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_protocol_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_protocol_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/protocol", OC_IF_LL | OC_IF_BASELINE,
    OC_IF_NONE, OC_DISCOVERABLE, oc_knx_swu_protocol_get_handler,
    oc_knx_swu_protocol_put_handler, 0, 0, 1, "urn:knx:dpt.value1UCount");
}

static void
oc_knx_swu_maxdefer_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

static void
oc_knx_swu_maxdefer_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_maxdefer_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_maxdefer_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/maxdefer", OC_IF_LL | OC_IF_BASELINE,
    OC_IF_NONE, OC_DISCOVERABLE, oc_knx_swu_maxdefer_get_handler,
    oc_knx_swu_maxdefer_put_handler, 0, 0, 1, "urn:knx:dpt.value1UCount");
}

static void
oc_knx_swu_method_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

static void
oc_knx_swu_method_put_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_method_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_method_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/method", OC_IF_LL | OC_IF_BASELINE, OC_IF_NONE,
    OC_DISCOVERABLE, oc_knx_swu_method_get_handler,
    oc_knx_swu_method_put_handler, 0, 0, 1, "urn:knx:dpt.value1UCount");
}

static void
oc_knx_swu_lastupdate_get_handler(oc_request_t *request,
                                  oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_lastupdate_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_lastupdate_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/lastupdate",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_NONE,
                            OC_DISCOVERABLE, oc_knx_swu_lastupdate_get_handler,
                            0, 0, 0, 1, "urn:knx:dpt.value1UCount");
}

static void
oc_knx_swu_result_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_result_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_result_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/result",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_NONE,
                            OC_DISCOVERABLE, oc_knx_swu_result_get_handler, 0,
                            0, 0, 1, "urn:knx:dpt.value1UCount");
}

static void
oc_knx_swu_state_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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
  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_state_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_state_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/state",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_NONE,
                            OC_DISCOVERABLE, oc_knx_swu_state_get_handler, 0, 0,
                            0, 1, "urn:knx:dpt.value1UCount");
}

static void
oc_knx_swu_update_put_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_update_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_update_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/pkgv", OC_IF_LL | OC_IF_BASELINE, OC_IF_NONE,
    OC_DISCOVERABLE, 0, oc_knx_swu_update_put_handler, 0, 0, 1, "dpt.version");
}

static void
oc_knx_swu_pkgv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_pkgv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgv_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/pkgv", OC_IF_LL | OC_IF_BASELINE, OC_IF_NONE,
    OC_DISCOVERABLE, oc_knx_swu_pkgv_get_handler, 0, 0, 0, 1, "dpt.version");
}

static void
oc_knx_swu_pkgcmd_post_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_pkgcmd_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_reset_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/pkgcmd", OC_IF_NONE,
                            OC_IF_NONE, OC_DISCOVERABLE, 0, 0,
                            oc_knx_swu_pkgcmd_post_handler, 0, 0, "");
}

static void
oc_knx_swu_bytes_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_pkgbytes_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgbytes_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/bytes", OC_IF_NONE, OC_IF_NONE, OC_DISCOVERABLE,
    oc_knx_swu_bytes_get_handler, 0, 0, 0, 1, "urn:knx:dpt.value4UCount");
}

static void
oc_knx_swu_pkgqurl_get_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

static void
oc_knx_swu_pkgqurl_put_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

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

  oc_send_json_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_pkgqurl_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgqurl_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/pkgqurl",
                            OC_IF_LL | OC_IF_BASELINE, OC_IF_LL,
                            OC_DISCOVERABLE, oc_knx_swu_pkgqurl_get_handler,
                            oc_knx_swu_pkgqurl_put_handler, 0, 0, 0, "");
}

static void
oc_knx_swu_pkgnames_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;
  
  oc_send_cbor_response(request, OC_STATUS_OK);

}

void
oc_create_knx_swu_pkgnames_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgnames_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/pkgnames", OC_IF_D, OC_IF_D, OC_DISCOVERABLE,
    oc_knx_swu_pkgnames_get_handler, 0, 0, 0, 0, 1, ":dpt.a[n]");
}

static void
oc_knx_swu_pkg_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  //size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_knx_swu_pkg_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkg_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/idevid", OC_IF_D, OC_IF_D, OC_DISCOVERABLE, 0,
    oc_knx_swu_pkg_put_handler, 0, 0, 0, 1, ":dpt.a[n]");
}

static void
oc_core_knx_swu_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int matches = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  for (i = (int)OC_KNX_SWU_PROTOCOL; i < (int)OC_KNX_SWU; i++) {
    oc_resource_t *resource = oc_core_get_resource_by_index(i, device_index);
    if (oc_filter_resource(resource, request, device_index, &response_length,
                           matches)) {
      matches++;
    }
  }

  if (matches > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }
}

void
oc_create_knx_swu_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu", OC_IF_LL, OC_IF_NONE,
                            OC_DISCOVERABLE, oc_core_knx_swu_get_handler, 0, 0,
                            0, 1, "urn:knx:fbswu");
}

void
oc_create_knx_swu_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_swu_resources");

  oc_create_knx_swu_protocol_resource(OC_KNX_SWU_PROTOCOL, device_index);
  oc_create_knx_swu_maxdefer_resource(OC_KNX_SWU_MAXDEFER, device_index);
  oc_create_knx_swu_method_resource(OC_KNX_SWU_METHOD, device_index);
  oc_create_knx_swu_lastupdate_resource(OC_KNX_LASTUPDATE, device_index);
  oc_create_knx_swu_result_resource(OC_KNX_SWU_RESULT, device_index);
  oc_create_knx_swu_state_resource(OC_KNX_SWU_STATE, device_index);
  // /swu/update/{filename} // optional resource not implemented
  oc_create_knx_swu_update_resource(OC_KNX_SWU_UPDATE, device_index);
  oc_create_knx_swu_pkgv_resource(OC_KNX_SWU_PKGV, device_index);
  oc_create_knx_swu_pkgcmd_resource(OC_KNX_SWU_PKGCMD, device_index);
  oc_create_knx_swu_pkgbytes_resource(OC_KNX_SWU_PKGBYTES, device_index);
  oc_create_knx_swu_pkgqurl_resource(OC_KNX_SWU_PKGQURL, device_index);
  oc_create_knx_swu_pkgnames_resource(OC_KNX_SWU_PKGNAMES, device_index);
  oc_create_knx_swu_pkg_resource(OC_KNX_SWU_PKG, device_index);
  oc_create_knx_swu_resource(OC_KNX_SWU, device_index);
}
