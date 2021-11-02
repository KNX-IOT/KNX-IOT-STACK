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
#include "oc_knx.h"
#include "oc_core_res.h"
#include <stdio.h>
#include "oc_rep.h" // should not be needed

#define TAGS_AS_STRINGS


static void
oc_core_knx_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                        void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_JSON ||
      request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  if (request->accept == APPLICATION_JSON) {

    int length = oc_rep_add_line_to_buffer("{");
    response_length += length;

    length = oc_rep_add_line_to_buffer("\"api\": { \"version\": \"1.0\",");
    response_length += length;

    length = oc_rep_add_line_to_buffer("\"base\": \"/ \"}");
    response_length += length;

    length = oc_rep_add_line_to_buffer("}");
    response_length += length;

    oc_send_json_response(request, OC_STATUS_OK);
    request->response->response_buffer->response_length = response_length;
  } else {

    oc_rep_begin_root_object();

    oc_rep_begin_object(oc_rep_object(root), api);
    oc_rep_set_text_string(api, version, "1.0");
    oc_rep_end_object(oc_rep_object(root), api);

    oc_rep_set_text_string(root, base, "/");
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
  }
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
  int length = oc_rep_add_line_to_buffer("{");
  response_length += length;

  length = oc_rep_add_line_to_buffer("\"api\": { \"version\": \"1.0\",");
  response_length += length;

  length = oc_rep_add_line_to_buffer("\"base\": \"/ \"}");
  response_length += length;

  length = oc_rep_add_line_to_buffer("}");
  response_length += length;
  */

  oc_send_json_response(request, OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_knx_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/.well-known/knx", OC_IF_LL | OC_IF_BASELINE,
    APPLICATION_LINK_FORMAT, OC_DISCOVERABLE, oc_core_knx_get_handler, 0,
    oc_core_knx_post_handler, 0, 0, "");
}

static void
oc_core_knx_reset_post_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
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

  oc_send_json_response(request, OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_knx_reset_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_reset_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/reset",
                               OC_IF_LL | OC_IF_BASELINE, APPLICATION_CBOR,
                               OC_DISCOVERABLE, 0, 0,
                               oc_core_knx_reset_post_handler, 0, 0, "");
}

oc_lsm_state_t
oc_knx_lsm_state(size_t device_index)
{
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR("device not found %d", device_index)
    return LSM_UNLOADED;
  }

  return device->lsm;
}

const char *
oc_core_get_lsm_as_string(oc_lsm_state_t lsm)
{
  // states
  if (lsm == LSM_UNLOADED) {
    return "unloaded";
  }
  if (lsm == LSM_LOADING) {
    return "loading";
  }
  if (lsm == LSM_LOADED) {
    return "loaded";
  }
  // commands
  if (lsm == LSM_UNLOAD) {
    return "unload";
  }
  if (lsm == LSM_STARTLOADING) {
    return "startLoading";
  }
  if (lsm == LSM_lOADCOMPLETE) {
    return "loadComplete";
  }

  return "";
}

bool
oc_core_lsm_check_string(const char *lsm)
{
  int len = strlen(lsm);

  // states
  if (len == 8 && strncmp(lsm, "unloaded", 8) == 0) {
    return true;
  }
  if (len == 7 && strncmp(lsm, "loading", 7) == 0) {
    return true;
  }
  if (len == 6 && strncmp(lsm, "loaded", 6) == 0) {
    return true;
  }

  // commands
  if (len == 6 && strncmp(lsm, "unload", 6) == 0) {
    return true;
  }
  if (len == 12 && strncmp(lsm, "startLoading", 12) == 0) {
    return true;
  }
  if (len == 12 && strncmp(lsm, "loadComplete", 12) == 0) {
    return true;
  }

  return false;
}

oc_lsm_state_t
oc_core_lsm_parse_string(const char *lsm)
{
  int len = strlen(lsm);

  // states
  if (len == 8 && strncmp(lsm, "unloaded", 8) == 0) {
    return LSM_UNLOADED;
  }
  if (len == 7 && strncmp(lsm, "loading", 7) == 0) {
    return LSM_LOADING;
  }
  if (len == 6 && strncmp(lsm, "loaded", 6) == 0) {
    return LSM_LOADED;
  }

  // commands
  if (len == 6 && strncmp(lsm, "unload", 6) == 0) {
    return LSM_UNLOAD;
  }
  if (len == 12 && strncmp(lsm, "startLoading", 12) == 0) {
    return LSM_STARTLOADING;
  }
  if (len == 12 && strncmp(lsm, "loadComplete", 12) == 0) {
    return LSM_lOADCOMPLETE;
  }

  return LSM_UNLOADED;
}


oc_lsm_state_t oc_core_lsm_cmd_to_state(oc_lsm_state_t cmd)
{
  if (cmd == LSM_STARTLOADING) {
    return LSM_LOADING;
  }
  if (cmd == LSM_lOADCOMPLETE) {
    return LSM_LOADED;
  }
  if (cmd == LSM_UNLOAD) {
    return LSM_UNLOADED;
  }

  return LSM_UNLOADED;
}


static void
oc_core_knx_lsm_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  PRINT("oc_core_knx_lsm_get_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
      
  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  oc_lsm_state_t lsm = oc_knx_lsm_state(device_index);

  oc_rep_start_root_object();
  oc_rep_i_set_text_string(root, 3, oc_core_get_lsm_as_string(lsm));
  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_knx_lsm_get_handler - done\n");
}

static void
oc_core_knx_lsm_post_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  oc_rep_t *rep = NULL;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  bool changed = false;
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;
  while (rep != NULL) {
    PRINT("key: (check) %s \n", oc_string(rep->name));
    if (rep->type == OC_REP_STRING) {
#ifdef TAGS_AS_STRINGS
      if (oc_string_len(rep->name) == 3 &&
          memcmp(oc_string(rep->name), "cmd", 3) == 0) {
          oc_lsm_state_t state =
             oc_core_lsm_parse_string(oc_string(rep->value.string));
          device->lsm = oc_core_lsm_cmd_to_state(state);
          changed = true;
          break;
        }
#endif
        if (rep->iname == 2) {
          oc_lsm_state_t state =
            oc_core_lsm_parse_string(oc_string(rep->value.string));
          device->lsm = oc_core_lsm_cmd_to_state(state);
          changed = true;
          break;
        }
    }

    rep = rep->next;
  }

  /* input was set, so create the response*/
  if (changed == true) {
    oc_rep_start_root_object();
    oc_rep_i_set_text_string(root, 3, oc_core_get_lsm_as_string(device->lsm));
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_knx_lsm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_lsm_resource\n");
  // "/a/lsm"
  oc_core_lf_populate_resource(resource_idx, device, "/a/lsm",
                               OC_IF_LL | OC_IF_BASELINE, APPLICATION_CBOR,
                               OC_DISCOVERABLE, oc_core_knx_lsm_get_handler, 0,
                               oc_core_knx_lsm_post_handler, 0, 0, "");
}

static void
oc_core_knx_crc_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
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
oc_core_knx_crc_post_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
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
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/crc",
                               OC_IF_LL, APPLICATION_CBOR, OC_DISCOVERABLE,
                               oc_core_knx_crc_get_handler, 0,
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
  oc_core_lf_populate_resource(resource_idx, device, "/dev/ldevid", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
                               oc_core_knx_ldevid_get_handler, 0, 0, 0, 0, 1,
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
  oc_core_lf_populate_resource(resource_idx, device, "/dev/idevid", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
                               oc_core_knx_idevid_get_handler, 0, 0, 0, 0, 1,
                               ":dpt.a[n]");
}

static void
oc_core_knx_spake_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
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
oc_core_knx_spake_post_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
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
oc_create_knx_spake_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_spake_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/spake",
                               OC_IF_LL, APPLICATION_CBOR, OC_DISCOVERABLE,
                               oc_core_knx_spake_get_handler, 0,
                               oc_core_knx_spake_post_handler, 0, 0, "");
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
  oc_create_knx_spake_resource(OC_KNX_SPAKE, device_index);
}
