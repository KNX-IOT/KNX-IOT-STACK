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

// ---------------------------Variables --------------------------------------

oc_group_object_notification_t g_received_notification;

oc_pase_t g_pase;

oc_string_t g_idevid;
oc_string_t g_ldevid;

// ----------------------------------------------------------------------------

static void
oc_core_knx_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                        void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_JSON &&
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

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

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

oc_lsm_state_t
oc_core_lsm_cmd_to_state(oc_lsm_state_t cmd)
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

// ----------------------------------------------------------------------------

static void
oc_core_knx_knx_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  PRINT("oc_core_knx_knx_get_handler\n");

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

  // g_received_notification

  oc_rep_begin_root_object();
  oc_rep_i_set_key(&root_map, 1);
  CborEncoder value_map;
  cbor_encoder_create_map(&root_map, &value_map, CborIndefiniteLength);
  // sia
  oc_rep_i_set_int(value, 4, g_received_notification.sia);
  // ga
  oc_rep_i_set_int(value, 7, g_received_notification.ga);
  // st M Service type code(write = w, read = r, response = rp) Enum : w, r, rp
  oc_rep_i_set_text_string(value, 6, oc_string(g_received_notification.st));
  // missing s (5) a map

  cbor_encoder_close_container_checked(&root_map, &value_map);

  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_knx_knx_get_handler - done\n");
}

static void
oc_core_knx_knx_post_handler(oc_request_t *request,
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

  // bool changed = false;
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;

      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {
        case OC_REP_STRING: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 2 &&
              memcmp(oc_string(object->name), "st", 2) == 0) {
            oc_free_string(&g_received_notification.st);
            oc_new_string(&g_received_notification.st,
                          oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
#endif
          if (object->iname == 6) {
            oc_free_string(&g_received_notification.st);
            oc_new_string(&g_received_notification.st,
                          oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;

        case OC_REP_INT: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 3 &&
              memcmp(oc_string(object->name), "sia", 3) == 0) {
            g_received_notification.sia = object->value.integer;
          }
          if (oc_string_len(object->name) == 2 &&
              memcmp(oc_string(object->name), "ga", 2) == 0) {
            g_received_notification.ga = object->value.integer;
          }
#endif
          // sia
          if (object->iname == 4) {
            g_received_notification.sia = object->value.integer;
          }
          // ga
          if (object->iname == 7) {
            g_received_notification.ga = object->value.integer;
          }
        } break;
        case OC_REP_NIL:
          break;
        default:
          break;
        }
        object = object->next;
      }
    }
    case OC_REP_NIL:
      break;
    default:
      break;
    }
    rep = rep->next;
  };

  /* input was set, so create the response*/
  // if (changed == true) {
  // oc_rep_start_root_object();
  // oc_rep_i_set_text_string(root, 3, oc_core_get_lsm_as_string(device->lsm));
  // oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_CHANGED);
  return;
  //}

  // oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_knx_knx_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_knx_resource\n");
  // "/a/lsm"
  oc_core_lf_populate_resource(
    resource_idx, device, "/.knx", OC_IF_LL | OC_IF_BASELINE, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_knx_knx_get_handler, 0,
    oc_core_knx_knx_post_handler, 0, 1, "urn:knx:g.s");
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

static void
oc_core_knx_ldevid_get_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  PRINT("oc_core_knx_ldevid_get_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_PKCS7_CMC_REQUEST) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  response_length = oc_string_len(g_ldevid);
  oc_rep_encode_raw((const uint8_t *)oc_string(g_ldevid),
                    (size_t)response_length);

  request->response->response_buffer->content_format =
    APPLICATION_PKCS7_CMC_RESPONSE;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;

  PRINT("oc_core_knx_ldevid_get_handler- done\n");
}

/* optional resource */
void
oc_create_knx_ldevid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_ldevid_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/ldevid",
                               OC_IF_D, APPLICATION_PKCS7_CMC_REQUEST,
                               OC_DISCOVERABLE, oc_core_knx_ldevid_get_handler,
                               0, 0, 0, 0, 1, ":dpt.a[n]");
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_idevid_get_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  PRINT("oc_core_knx_idevid_get_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_PKCS7_CMC_REQUEST) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  response_length = oc_string_len(g_idevid);
  oc_rep_encode_raw((const uint8_t *)oc_string(g_idevid),
                    (size_t)response_length);

  request->response->response_buffer->content_format =
    APPLICATION_PKCS7_CMC_RESPONSE;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;

  PRINT("oc_core_knx_idevid_get_handler- done\n");
}

void
oc_create_knx_idevid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_idevid_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/idevid",
                               OC_IF_D, APPLICATION_PKCS7_CMC_REQUEST,
                               OC_DISCOVERABLE, oc_core_knx_idevid_get_handler,
                               0, 0, 0, 0, 1, ":dpt.a[n]");
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_spake_post_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_knx_spake_post_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  if (request->content_format != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  int valid_request = 0;
  // check input
  // note: no check if there are multiple byte strings in the request payload
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_BYTE_STRING: {
      // ca == 14
      if (rep->iname == 14) {
        valid_request = 14;
      }
      // pa == 10
      if (rep->iname == 10) {
        valid_request = 10;
      }
      // rnd == 15
      if (rep->iname == 15) {
        valid_request = 15;
      }
    } break;
    case OC_REP_OBJECT: {
      // pbkdf2 == 12
      // not sure if we need this
      if (rep->iname == 12) {
        valid_request = 12;
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    }
    rep = rep->next;
  }

  if (valid_request == 0) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
  }
  rep = request->request_payload;
  // handle input
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_BYTE_STRING: {
      // ca == 14
      if (rep->iname == 14) {
        oc_free_string(&g_pase.ca);
        oc_new_string(&g_pase.ca, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
      // pa == 10
      if (rep->iname == 10) {
        oc_free_string(&g_pase.pa);
        oc_new_string(&g_pase.pa, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
      // rnd == 15
      if (rep->iname == 15) {
        oc_free_string(&g_pase.rnd);
        oc_new_string(&g_pase.rnd, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
        // TODO: compute pB
        oc_free_string(&g_pase.pb);
        oc_new_string(&g_pase.pb, "pb-computed", strlen("pb-computed"));

        // TODO: compute cB
        oc_free_string(&g_pase.cb);
        oc_new_string(&g_pase.cb, "cb-computed", strlen("cb-computed"));
      }
    } break;
    case OC_REP_OBJECT: {
      // pbkdf2 == 12
      if (rep->iname == 12) {
        oc_rep_t *object = rep->value.object;
        while (object != NULL) {
          switch (object->type) {
          case OC_REP_BYTE_STRING: {
            // salt
            if (object->iname == 5) {
              oc_free_string(&g_pase.salt);
              oc_new_string(&g_pase.salt, oc_string(object->value.string),
                            oc_string_len(object->value.string));
            }
          } break;
          case OC_REP_INT: {
            // it
            if (object->iname == 16) {
              g_pase.it = object->value.integer;
            }
          } break;
          case OC_REP_NIL:
            break;
          default:
            break;
          }
          object = object->next;
        }
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    }
    rep = rep->next;
  }

  PRINT("oc_core_knx_spake_post_handler valid_request: %d\n", valid_request);
  // on ca
  if (valid_request == 14) {
    // return changed, no payload
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }
  // on pa
  if (valid_request == 10) {
    // return changed, frame pb (11) & cb (13)
    // TODO: probably we need to calculate them...

    oc_rep_begin_root_object();
    // pb (11)
    oc_rep_i_set_text_string(root, 11, oc_string(g_pase.pb));
    // cb (13)
    oc_rep_i_set_text_string(root, 13, oc_string(g_pase.cb));
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }
  // on rnd
  if (valid_request == 15) {
    // return changed, frame rnd (15) & pbkdf2 (12 containing (16, 5))
    // TODO: probably we need to calculate them...

    oc_rep_begin_root_object();
    // rnd (15)
    oc_rep_i_set_text_string(root, 15, oc_string(g_pase.rnd));
    // pbkdf2
    oc_rep_i_set_key(&root_map, 12);
    oc_rep_begin_object(&root_map, pbkdf2);
    // it 16
    oc_rep_i_set_int(pbkdf2, 16, g_pase.it);
    // salt 5
    oc_rep_i_set_text_string(pbkdf2, 5, oc_string(g_pase.salt));
    oc_rep_end_object(&root_map, pbkdf2);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_knx_spake_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_spake_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/spake",
                               OC_IF_NONE, APPLICATION_CBOR, OC_DISCOVERABLE, 0,
                               0, oc_core_knx_spake_post_handler, 0, 0, "");
}

// ----------------------------------------------------------------------------

void
oc_knx_set_idevid(const char *idevid, int len)
{
  oc_free_string(&g_idevid);
  oc_new_string(&g_idevid, idevid, len);
}

void
oc_knx_set_ldevid(char *idevid, int len)
{
  oc_free_string(&g_ldevid);
  oc_new_string(&g_ldevid, idevid, len);
}

void
oc_create_knx_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_resources");

  oc_create_knx_lsm_resource(OC_KNX_LSM, device_index);
  oc_create_knx_knx_resource(OC_KNX_DOT_KNX, device_index);

  oc_create_knx_reset_resource(OC_KNX, device_index);
  oc_create_knx_resource(OC_KNX_RESET, device_index);
  oc_create_knx_crc_resource(OC_KNX_CRC, device_index);
  oc_create_knx_ldevid_resource(OC_KNX_LDEVID, device_index);
  oc_create_knx_idevid_resource(OC_KNX_IDEVID, device_index);
  oc_create_knx_spake_resource(OC_KNX_SPAKE, device_index);
}
