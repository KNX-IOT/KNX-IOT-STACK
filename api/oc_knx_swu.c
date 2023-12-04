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
#include "api/oc_knx_helpers.h"
#include "oc_discovery.h"
#include "oc_core_res.h"
#include <stdio.h>

typedef struct oc_swu_t
{
  oc_swu_cb_t cb;
  void *data;
} oc_swu_t;

/* MAX DEFER*/
#define KNX_STORAGE_SWU_MAX_DEFER "swu_knx_max_defer"
static int g_swu_max_defer = 0;

/* UPDATE METHOD*/
#define KNX_STORAGE_SWU_METHOD "swu_knx_method"
static int g_swu_update_method = 0;

/* PACKAGE names*/
/* note will be initialized with "" during creation of the resource*/
#define KNX_STORAGE_PACKAGE_NAMES "swu_knx_package_names"
static oc_string_t g_swu_package_name;

/* last update (time) */
/* note will be initialized with "" during creation of the resource*/
#define KNX_STORAGE_LAST_UPDATE "swu_knx_last_update"
static oc_string_t g_swu_last_update;

/* package bytes */
/* note will be initialized with "" during creation of the resource*/
#define KNX_STORAGE_PACKAGE_BYTES "swu_knx_package_bytes"
static int g_swu_package_bytes = 0;

/* package version (pkgv) */
/* note will be initialized with "" during creation of the resource*/
#define KNX_STORAGE_PACKAGE_VERSION "swu_knx_package_version"
static oc_knx_version_info_t g_swu_package_version;

/* software update state */
/* note will be initialized with "" during creation of the resource*/
#define KNX_STORAGE_UPDATE_STATE "swu_knx_update_state"
static oc_swu_state_t g_swu_state = OC_SWU_STATE_IDLE;

/* package query url /swu/pkgqurl */
/* note will be initialized with "" during creation of the resource*/
#define KNX_STORAGE_QURL "swu_knx_qurl"
static oc_string_t g_swu_qurl;

/* software update result */
/* note will be initialized with "" during creation of the resource*/
#define KNX_STORAGE_UPDATE_RESULT "swu_knx_update_result"
static oc_swu_result_t g_swu_result = OC_SWU_RESULT_INIT;

static oc_swu_t app_swu;

// -----------------------------------------------------------------------------

void
oc_set_swu_cb(oc_swu_cb_t cb, void *data)
{
  app_swu.cb = cb;
  app_swu.data = data;
}

oc_swu_t *
oc_get_swu_cb(void)
{
  return &app_swu;
}

// -----------------------------------------------------------------------------

static void
oc_knx_swu_protocol_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /*
  A list of supported protocols by the KNX IoT device.
  0: Unicast CoAP w/ OSCORE (as defined in RFC 7252) with the additional support
  for Block-wise transfer. CoAP is the default setting. 1: CoAPS (as defined in
  RFC 7252) with the additional support for Block-wise transfer 4: CoAP w/
  OSCORE over TCP (as defined in RFC 8323) 5: CoAP over TLS (as defined in RFC
  8323) 254: Manufacturer specific
  */
  /* only support 0 */

  // Content-Format: "application/cbor"
  // Payload: [ 0 ]
  CborEncoder arrayEncoder;
  cbor_encoder_create_array(&g_encoder, &arrayEncoder, 1);
  cbor_encode_int(&arrayEncoder, (int64_t)0);
  cbor_encoder_close_container(&g_encoder, &arrayEncoder);

  oc_send_cbor_response(request, OC_STATUS_OK);
  return;
}

static void
oc_knx_swu_protocol_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  /* not sure what to do with request data, so we are just parsing it for now*/
  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_knx_swu_protocol_put_handler received : %d\n",
          (int)rep->value.integer);

    oc_send_response_no_format(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_protocol, knx_swu_maxdefer, 0,
                                     "/swu/protocol", OC_IF_SWU | OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_protocol_get_handler,
                                     oc_knx_swu_protocol_put_handler, 0, 0,
                                     "urn:knx:dpt.protocols", OC_SIZE_ZERO());
void
oc_create_knx_swu_protocol_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_protocol_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/protocol",
                            OC_IF_SWU | OC_IF_D, APPLICATION_CBOR,
                            OC_DISCOVERABLE, oc_knx_swu_protocol_get_handler,
                            oc_knx_swu_protocol_put_handler, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.protocols");
}

static void
oc_knx_swu_maxdefer_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  /*
   max defer in seconds
  */
  cbor_encode_int(&g_encoder, (int64_t)g_swu_max_defer);

  oc_send_json_response(request, OC_STATUS_OK);
}

static void
oc_knx_swu_maxdefer_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_knx_swu_maxdefer_put_handler received : %d\n",
          (int)rep->value.integer);
    g_swu_max_defer = (int)rep->value.integer;
    oc_storage_write(KNX_STORAGE_SWU_MAX_DEFER, (uint8_t *)&g_swu_max_defer,
                     sizeof(g_swu_max_defer));
    oc_send_response_no_format(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_maxdefer, knx_swu_method, 0,
                                     "/swu/maxdefer", OC_IF_LI,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_maxdefer_get_handler,
                                     oc_knx_swu_maxdefer_put_handler, 0, 0,
                                     "urn:knx:dpt.timePeriodSec",
                                     OC_SIZE_ZERO());
void
oc_create_knx_swu_maxdefer_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_maxdefer_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/maxdefer", OC_IF_LI,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_knx_swu_maxdefer_get_handler,
                            oc_knx_swu_maxdefer_put_handler, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.timePeriodSec");
}

static void
oc_knx_swu_method_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /*
  0: Pull only
  1: Push only
  2: Both (Initial value).
  */
  /* we are only going to support PUSH */
  cbor_encode_int(&g_encoder, (int64_t)g_swu_update_method);

  oc_send_json_response(request, OC_STATUS_OK);
}

static void
oc_knx_swu_method_put_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_knx_swu_method_put_handler received : %d\n",
          (int)rep->value.integer);
    g_swu_update_method = (int)rep->value.integer;
    oc_storage_write(KNX_STORAGE_SWU_METHOD, (uint8_t *)&g_swu_update_method,
                     sizeof(g_swu_update_method));
    oc_send_response_no_format(request, OC_STATUS_OK);
    return;
  }

  oc_send_json_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_method, knx_lastupdate, 0,
                                     "/swu/method", OC_IF_SWU | OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_method_get_handler,
                                     oc_knx_swu_method_put_handler, 0, 0,
                                     "urn:knx:dpt.transferMethod",
                                     OC_SIZE_ZERO());
void
oc_create_knx_swu_method_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_method_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/method",
                            OC_IF_SWU | OC_IF_D, APPLICATION_CBOR,
                            OC_DISCOVERABLE, oc_knx_swu_method_get_handler,
                            oc_knx_swu_method_put_handler, 0, 0, 0);
  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.transferMethod");
}

static void
oc_knx_swu_lastupdate_get_handler(oc_request_t *request,
                                  oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // last update (time)
  cbor_encode_text_stringz(&g_encoder, oc_string(g_swu_last_update));

  oc_send_json_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_lastupdate, knx_swu_result, 0,
                                     "/swu/lastupdate", OC_IF_D | OC_IF_SWU,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_lastupdate_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.varString8859_1",
                                     OC_SIZE_ZERO());
void
oc_create_knx_swu_lastupdate_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_lastupdate_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/lastupdate",
                            OC_IF_D | OC_IF_SWU, APPLICATION_CBOR,
                            OC_DISCOVERABLE, oc_knx_swu_lastupdate_get_handler,
                            0, 0, 0, 0);
  oc_core_bind_dpt_resource(resource_idx, device,
                            "urn:knx:dpt.varString8859_1");
}

static void
oc_knx_swu_result_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // g_swu_state
  cbor_encode_int(&g_encoder, (int64_t)g_swu_result);

  oc_send_json_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_result, knx_swu_state, 0,
                                     "/swu/result", OC_IF_D | OC_IF_SWU,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_result_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.updateResult",
                                     OC_SIZE_ZERO());

void
oc_create_knx_swu_result_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_result_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/result", OC_IF_D | OC_IF_SWU, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_knx_swu_result_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.updateResult");
}

static void
oc_knx_swu_state_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // g_swu_state
  cbor_encode_int(&g_encoder, (int64_t)g_swu_state);

  oc_send_json_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_state, knx_swu_update, 0,
                                     "/swu/state", OC_IF_D | OC_IF_SWU,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_state_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.dldState", OC_SIZE_ZERO());

void
oc_create_knx_swu_state_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_state_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/state", OC_IF_D | OC_IF_SWU, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_knx_swu_state_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.dldState");
}

static void
oc_knx_swu_update_put_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // note we are not doing anything with the trigger.

  /* not sure what to do with request data, so we are just parsing it for now*/
  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_knx_swu_update_put_handler received : %d\n",
          (int)rep->value.integer);
    oc_send_response_no_format(request, OC_STATUS_OK);
    // oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_update, knx_swu_pkgv, 0,
                                     "/swu/update", OC_IF_D | OC_IF_SWU,
                                     APPLICATION_CBOR, OC_DISCOVERABLE, 0,
                                     oc_knx_swu_update_put_handler, 0, 0,
                                     "urn:knx:dpt.timePeriodSecZ",
                                     OC_SIZE_ZERO());

void
oc_create_knx_swu_update_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_update_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/update", OC_IF_D | OC_IF_SWU, APPLICATION_CBOR,
    OC_DISCOVERABLE, 0, oc_knx_swu_update_put_handler, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.timePeriodSecZ");
}

static void
oc_knx_swu_pkgv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // Payload: [ 1, 2, 3 ]
  CborEncoder arrayEncoder;
  cbor_encoder_create_array(&g_encoder, &arrayEncoder, 3);
  cbor_encode_int(&arrayEncoder, (int64_t)g_swu_package_version.major);
  cbor_encode_int(&arrayEncoder, (int64_t)g_swu_package_version.minor);
  cbor_encode_int(&arrayEncoder, (int64_t)g_swu_package_version.patch);
  cbor_encoder_close_container(&g_encoder, &arrayEncoder);

  oc_send_json_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_pkgv, knx_swu_pkgcmd, 0,
                                     "/swu/pkgv", OC_IF_D | OC_IF_SWU,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_pkgv_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.version", OC_SIZE_ZERO());

void
oc_create_knx_swu_pkgv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgv_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/pkgv", OC_IF_D | OC_IF_SWU, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_knx_swu_pkgv_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.version");
}

static void
oc_knx_swu_a_put_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  int binary_size = 0;
  int block_size = 0;
  int block_offset = 0;
  char *key = 0;
  char *value = 0;
  size_t key_len = 0, value_len;

  oc_content_format_t content_format;
  const uint8_t *payload = NULL;
  size_t len = 0;

  static oc_separate_response_t s_delayed_response_swu;

  oc_swu_t *my_cb = oc_get_swu_cb();
  if (my_cb && my_cb->cb)
    oc_indicate_separate_response(request, &s_delayed_response_swu);
  else
    (void)s_delayed_response_swu;

  PRINT("  oc_knx_swu_a_put_handler : Start\n");

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_OCTET_STREAM) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_init_query_iterator();
  while (oc_iterate_query(request, &key, &key_len, &value, &value_len) > 0) {
    if (strncmp(key, "po", key_len) == 0) {
      block_offset = atoi(value);
    }
    if (strncmp(key, "ps", key_len) == 0) {
      block_size = atoi(value);
    }
    if (strncmp(key, "pkgs", key_len) == 0) {
      binary_size = atoi(value);
    }
  }
  PRINT("binary_size: %d\n", binary_size);
  PRINT("block_size: %d\n", block_size);
  PRINT("block_offset: %d\n", block_offset);
  // if (block_size == 0) {
  //  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
  //  return;
  //}
  size_t device_index = request->resource->device;

  bool berr =
    oc_get_request_payload_raw(request, &payload, &len, &content_format);
  // PRINT("      raw buffer ok: %d len=%d\n", berr, len);

  if (my_cb && my_cb->cb) {
    my_cb->cb(device_index, &s_delayed_response_swu, binary_size, block_offset,
              (uint8_t *)payload, len, my_cb->data);
  } else {
    oc_send_json_response(request, OC_STATUS_OK);
  }

  PRINT("  oc_knx_swu_a_put_handler : End\n");
}

static void
oc_knx_swu_a_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // Triggers a software update query request (PULL on Software Update Server).
  // not implemented
  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_knx_swu_a_post_handler received : %d\n",
          (int)rep->value.integer);

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_pkgcmd, knx_swu_pkgbytes, 0,
                                     "/a/swu", OC_IF_SWU | OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE, 0,
                                     oc_knx_swu_a_put_handler,
                                     oc_knx_swu_a_post_handler, 0,
                                     "urn:knx:dpt.file", OC_SIZE_ZERO());

void
oc_create_knx_swu_a_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_a_resource\n");
  oc_core_populate_resource(resource_idx, device, "/a/swu", OC_IF_SWU | OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE, 0,
                            oc_knx_swu_a_put_handler, oc_knx_swu_a_post_handler,
                            0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.file");
}

static void
oc_knx_swu_bytes_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // g_swu_package_bytes
  cbor_encode_int(&g_encoder, (int64_t)g_swu_package_bytes);

  oc_send_json_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_pkgbytes, knx_swu_pkgqurl, 0,
                                     "/swu/pkgbytes", OC_IF_SWU | OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_bytes_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.value4UCount",
                                     OC_SIZE_ZERO());
void
oc_create_knx_swu_pkgbytes_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgbytes_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/pkgbytes",
                            OC_IF_SWU | OC_IF_D, APPLICATION_CBOR,
                            OC_DISCOVERABLE, oc_knx_swu_bytes_get_handler, 0, 0,
                            0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value4UCount");
}

static void
oc_knx_swu_pkgqurl_get_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  cbor_encode_text_stringz(&g_encoder, oc_string(g_swu_qurl));

  oc_send_json_response(request, OC_STATUS_OK);
}

static void
oc_knx_swu_pkgqurl_put_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_STRING)) {
    PRINT("  oc_knx_swu_pkgqurl_put_handler received : %s\n",
          oc_string_checked(rep->value.string));

    oc_send_response_no_format(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_pkgqurl, knx_swu_pkgnames, 0,
                                     "/swu/pkgqurl", OC_IF_SWU | OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_pkgqurl_get_handler,
                                     oc_knx_swu_pkgqurl_put_handler, 0, 0,
                                     "urn:knx:dpt.url", OC_SIZE_ZERO());

void
oc_create_knx_swu_pkgqurl_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgqurl_resource\n");
  oc_core_populate_resource(resource_idx, device, "/swu/pkgqurl",
                            OC_IF_SWU | OC_IF_D, APPLICATION_CBOR,
                            OC_DISCOVERABLE, oc_knx_swu_pkgqurl_get_handler,
                            oc_knx_swu_pkgqurl_put_handler, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.url");
}

static void
oc_knx_swu_pkgname_get_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  CborEncoder arrayEncoder;
  cbor_encoder_create_array(&g_encoder, &arrayEncoder, 1);
  cbor_encode_text_stringz(&arrayEncoder, oc_string(g_swu_package_name));
  cbor_encoder_close_container(&g_encoder, &arrayEncoder);

  oc_send_cbor_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu_pkgnames, knx_swu, 0,
                                     "/swu/pkgname", OC_IF_SWU | OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_knx_swu_pkgname_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.varString8859_1",
                                     OC_SIZE_ZERO());

void
oc_create_knx_swu_pkgnames_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_pkgnames_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/swu/pkgname", OC_IF_SWU | OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_knx_swu_pkgname_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device,
                            "urn:knx:dpt.varString8859_1");
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

  bool ps_exists = false;
  bool total_exists = false;
  int total = (int)OC_KNX_SWU - (int)OC_KNX_SWU_PROTOCOL;
  int first_entry = (int)OC_KNX_SWU_PROTOCOL; // inclusive
  int last_entry = (int)OC_KNX_SWU;           // exclusive
  // int query_ps = -1;
  int query_pn = -1;
  bool more_request_needed =
    false; // If more requests (pages) are needed to get the full list

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /swu > l = total>;total=22;ps=5
    response_length =
      oc_frame_query_l(oc_string(request->resource->uri), ps_exists, PAGE_SIZE,
                       total_exists, total);
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }
  if (l_exist == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }

  // handle query with page number (pn)
  if (check_if_query_pn_exist(request, &query_pn, NULL)) {
    first_entry += query_pn * PAGE_SIZE;
    if (first_entry >= last_entry) {
      oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
      return;
    }
  }

  if (last_entry > first_entry + PAGE_SIZE) {
    last_entry = first_entry + PAGE_SIZE;
    more_request_needed = true;
  }

  for (i = first_entry; i < last_entry; i++) {
    const oc_resource_t *resource =
      oc_core_get_resource_by_index(i, device_index);
    if (oc_filter_resource(resource, request, device_index, &response_length,
                           &i, i)) {
      matches++;
    }
  }

  if (matches > 0) {
    if (more_request_needed) {
      int next_page_num = query_pn > -1 ? query_pn + 1 : 1;
      response_length += add_next_page_indicator(
        oc_string(request->resource->uri), next_page_num);
    }
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  }
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_swu, sub, 0, "/swu",
                                     OC_IF_SWU | OC_IF_LI,
                                     APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                                     oc_core_knx_swu_get_handler, 0, 0, 0, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:fb.swu");

void
oc_create_knx_swu_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_swu_resource\n");
  //
  oc_core_populate_resource(
    resource_idx, device, "/swu", OC_IF_SWU | OC_IF_LI, APPLICATION_LINK_FORMAT,
    OC_DISCOVERABLE, oc_core_knx_swu_get_handler, 0, 0, 0, 1, "urn:knx:fb.swu");
}

void
oc_create_knx_swu_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_swu_resources");

  if (device_index == 0) {
    OC_DBG("resources for dev 0 created statically");
  } else {
    oc_create_knx_swu_protocol_resource(OC_KNX_SWU_PROTOCOL, device_index);
    oc_create_knx_swu_maxdefer_resource(OC_KNX_SWU_MAXDEFER, device_index);
    oc_create_knx_swu_method_resource(OC_KNX_SWU_METHOD, device_index);
    oc_create_knx_swu_lastupdate_resource(OC_KNX_LASTUPDATE, device_index);
    oc_create_knx_swu_result_resource(OC_KNX_SWU_RESULT, device_index);
    oc_create_knx_swu_state_resource(OC_KNX_SWU_STATE, device_index);
    // /swu/update/{filename} // optional resource not implemented
    oc_create_knx_swu_update_resource(OC_KNX_SWU_UPDATE, device_index);
    oc_create_knx_swu_pkgv_resource(OC_KNX_SWU_PKGV, device_index);
    oc_create_knx_swu_a_resource(OC_KNX_SWU_PKGCMD, device_index);
    oc_create_knx_swu_pkgbytes_resource(OC_KNX_SWU_PKGBYTES, device_index);
    oc_create_knx_swu_pkgqurl_resource(OC_KNX_SWU_PKGQURL, device_index);
    oc_create_knx_swu_pkgnames_resource(OC_KNX_SWU_PKGNAMES, device_index);

    oc_create_knx_swu_resource(OC_KNX_SWU, device_index);
  }
  oc_swu_set_package_name("");
  oc_swu_set_last_update("");
  oc_swu_set_package_version(0, 0, 0);
}

// ----------------------------------------------------------------------------

void
oc_swu_set_package_name(char *name)
{
  oc_free_string(&g_swu_package_name);
  oc_new_string(&g_swu_package_name, name, strlen(name));
}

void
oc_swu_set_last_update(char *time)
{
  oc_free_string(&g_swu_last_update);
  oc_new_string(&g_swu_last_update, time, strlen(time));
}

void
oc_swu_set_package_bytes(int package_bytes)
{
  g_swu_package_bytes = package_bytes;
}

void
oc_swu_set_package_version(int major, int minor, int minor2)
{
  g_swu_package_version.major = major;
  g_swu_package_version.minor = minor;
  g_swu_package_version.patch = minor2;
}

void
oc_swu_set_state(oc_swu_state_t state)
{
  g_swu_state = state;
}

void
oc_swu_set_qurl(char *qurl)
{
  oc_free_string(&g_swu_qurl);
  oc_new_string(&g_swu_qurl, qurl, strlen(qurl));
}

void
oc_swu_set_result(oc_swu_result_t result)
{
  g_swu_result = result;
}