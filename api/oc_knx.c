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
#include "oc_knx_fp.h"
#include "oc_knx_dev.h"
#include "oc_core_res.h"
#include <stdio.h>
#include "oc_rep.h" // should not be needed

#define TAGS_AS_STRINGS

#define LSM_STORE "LSM_STORE"
// ---------------------------Variables --------------------------------------

oc_group_object_notification_t g_received_notification;

uint64_t g_crc = 0;
uint64_t g_osn = 0;

oc_pase_t g_pase;

oc_string_t g_idevid;
oc_string_t g_ldevid;

// ----------------------------------------------------------------------------

#define RESTART_DEVICE 2
#define RESET_DEVICE 1

static int
convert_cmd(char *cmd)
{
  if (strncmp(cmd, "reset", strlen("reset")) == 0) {
    return RESET_DEVICE;
  }
  if (strncmp(cmd, "restart", strlen("restart")) == 0) {
    return RESTART_DEVICE;
  }

  OC_DBG("convert_cmd command not recognized: %s", cmd);
  return 0;
}

int
restart_device()
{
  PRINT("restart device\n");

  // do a reboot...
  return 0;
}

int
reset_device(int value)
{
  PRINT("reset device: %d\n", value);

  oc_knx_device_storage_reset(0);

  return 0;
}

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

    oc_rep_start_root_object();

    oc_rep_set_text_string(root, base, "/");

    oc_rep_set_key(&root_map, "api");
    oc_rep_begin_object(&root_map, api);
    oc_rep_set_text_string(api, version, "1.0");
    oc_rep_end_object(&root_map, api);

    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
  }
}

/*
  value = 1   Unsigned
  cmd = 2  Text string
  status = 3  Unsigned
  code = "code" Unsigned
  time = "time" Unsigned
*/
static void
oc_core_knx_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  int value = -1;
  int cmd = -1;
  // int time;
  // int code;

  PRINT("oc_core_knx_post_handler\n");

  char buffer[200];
  memset(buffer, 200, 1);
  oc_rep_to_json(request->request_payload, (char *)&buffer, 200, true);
  PRINT("%s", buffer);

  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_INT: {
      if (rep->iname == 1) {
        {
          value = rep->value.integer;
        }
      }
    } break;
    case OC_REP_STRING: {
      if (rep->iname == 2) {
        {
          // the command
          cmd = convert_cmd(oc_string(rep->value.string));
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

  PRINT("  cmd   : %d\n", cmd);
  PRINT("  value : %d\n", value);

  bool error = true;

  if (cmd == RESTART_DEVICE) {
    restart_device();
    error = false;
  } else if (cmd == RESET_DEVICE) {
    reset_device(value);
    error = false;
  }

  // Before executing the reset function,
  // the KNX IoT device MUST return a response with CoAP response 690 code 2.04
  // CHANGED and with payload containing Error Code and Process Time in seconds
  // as defined 691 for the Response to a Master Reset Request for KNX Classic
  // devices, see [10].
  if (error == false) {

    // oc_rep_begin_root_object ();
    oc_rep_start_root_object();

    // note need to figure out how to fill in the correct response values
    oc_rep_set_int(root, code, 5);
    oc_rep_set_int(root, time, 2);
    oc_rep_end_root_object();

    PRINT("oc_core_knx_post_handler %d - end\n",
          oc_rep_get_encoded_payload_size());
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }

  PRINT(" invalid command\n");
  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
  return;
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

oc_lsm_state_t
oc_knx_lsm_state(size_t device_index)
{
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR("device not found %d", device_index);
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

    oc_storage_write(LSM_STORE, (uint8_t *)&device->lsm, sizeof(device->lsm));

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
  // { 4: "sia", 5: { 6: "st", 7: "ga", 1: "value" } }

  oc_rep_begin_root_object();
  // sia
  oc_rep_i_set_int(root, 4, g_received_notification.sia);

  oc_rep_i_set_key(&root_map, 5);
  CborEncoder value_map;
  cbor_encoder_create_map(&root_map, &value_map, CborIndefiniteLength);

  // ga
  oc_rep_i_set_int(value, 7, g_received_notification.ga);
  // st M Service type code(write = w, read = r, response = rp) Enum : w, r, rp
  oc_rep_i_set_text_string(value, 6, oc_string(g_received_notification.st));
  // missing value

  cbor_encoder_close_container_checked(&root_map, &value_map);

  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_knx_knx_get_handler - done\n");
}

void
oc_reset_g_received_notification()
{
  g_received_notification.sia = -1;
  g_received_notification.ga = -1;
  // g_received_notification.value =

  oc_free_string(&g_received_notification.st);
  oc_new_string(&g_received_notification.st, "", strlen(""));
}

void
oc_core_smode_rp(oc_request_t *request )
{
}


/*
 {sia: 5678, es: {st: write, ga: 1, value: 100 }}
*/
static void
oc_core_knx_knx_post_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  oc_rep_t *rep = NULL;

  PRINT("KNX KNX Post Handler");
  char buffer[200];
  memset(buffer, 200, 1);
  oc_rep_to_json(request->request_payload, (char *)&buffer, 200, true);
  PRINT("Decoded Payload: %s\n", buffer);

  PRINT("Full Payload Size: %d\n", (int)request->_payload_len);

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
  oc_reset_g_received_notification();

  /* loop over the request document to parse all the data */
  rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_INT: {
      // sia
      if (rep->iname == 4) {
        g_received_notification.sia = rep->value.integer;
      }
    } break;
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
  }

  bool do_write = false;
  bool do_read = false;
  // handle the request
  // loop over the group addresses of the /fp/r
  PRINT(" .knx : sia   %d\n", g_received_notification.sia);
  PRINT(" .knx : ga    %d\n", g_received_notification.ga);
  PRINT(" .knx : st    %s\n", oc_string(g_received_notification.st));
  if (strcmp(oc_string(g_received_notification.st), "w") == 0) {
    do_write = true;
  } else if (strcmp(oc_string(g_received_notification.st), "r") == 0) {
    do_read = true;
  } else {
    PRINT(" .knx : st : no reading/writing: ignoring request\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  int index = oc_core_find_group_object_table_index(g_received_notification.ga);
  PRINT(" .knx : index %d\n", index);
  if (index == -1) {
    // if nothing is found (initially) then return a bad request.
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  while (index != -1) {
    oc_string_t myurl = oc_core_find_group_object_table_url_from_index(index);
    PRINT(" .knx : url  %s\n", oc_string(myurl));
    if (oc_string_len(myurl) > 0) {
      // get the resource to do the fake post on
      oc_resource_t *my_resource = oc_ri_get_app_resource_by_uri(
        oc_string(myurl), oc_string_len(myurl), device_index);

      if (my_resource != NULL) {
        if (do_write) {
           // write the value to the resource
           my_resource->post_handler.cb(request, iface_mask, data);
        }
        if (do_read) {
            // do the actual read from the resource and 
            // send the reply
            oc_do_s_mode(oc_string(my_resource->uri), "rp");
         }
      }
    }
    // get the next index in the table to get the url from.
    // this stops when the returned index == -1
    int new_index = oc_core_find_next_group_object_table_index(
      g_received_notification.ga, index);
    index = new_index;
  }

  // don't send anything back on a multi cast message
  if (request->origin && (request->origin->flags & MULTICAST)) {
    PRINT(" .knx : Multicast - not sending response\n");
    oc_send_cbor_response(request, OC_IGNORE);
    return;
  }

  PRINT(" .knx : Unicast - sending response\n");
  // send the response
  oc_send_cbor_response(request, OC_STATUS_OK);
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
  PRINT("oc_core_knx_crc_get_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  cbor_encode_uint(&g_encoder, g_crc);

  PRINT("oc_core_knx_crc_get_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_knx_crc_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_crc_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/crc",
                               OC_IF_LL, APPLICATION_CBOR, OC_DISCOVERABLE,
                               oc_core_knx_crc_get_handler, 0, 0, 0, 0, "");
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_osn_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_knx_osn_get_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  cbor_encode_uint(&g_encoder, g_osn);

  PRINT("oc_core_knx_osn_get_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_knx_osn_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_osn_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/.well-known/knx/osn",
                               OC_IF_LL, APPLICATION_CBOR, OC_DISCOVERABLE,
                               oc_core_knx_osn_get_handler, 0, 0, 0, 0, "");
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
oc_knx_set_crc(uint64_t crc)
{
  g_crc = crc;
}

void
oc_knx_set_osn(uint64_t osn)
{
  g_osn = osn;
}

// ----------------------------------------------------------------------------

bool
oc_is_s_mode_request(oc_request_t *request)
{
  if (request == NULL) {
    return false;
  }

  // TODO: add code to check if the request is part of the same setup
  // e.g. check idd of the device vs the data in the request

  PRINT("  oc_is_s_mode_request %.*s\n", (int)request->uri_path_len,
        request->uri_path);
  if (strncmp(".knx", request->uri_path, request->uri_path_len) == 0) {
    return true;
  }
  return false;
}

oc_rep_t *
oc_s_mode_get_value(oc_request_t *request)
{

  /* loop over the request document to parse all the data */
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;

      object = rep->value.object;
      while (object != NULL) {
        // search for "value" (1)
        if (object->iname == 1) {
          return object;
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
  }
  return NULL;
}

static void
oc_issue_s_mode(int sia_value, int group_address, char *rp, uint8_t *value_data,
                int value_size)
{
  int scope = 5;
  //(void)sia_value; /* variable not used */

  PRINT("  oc_issue_s_mode : scope %d\n", scope);

  oc_make_ipv6_endpoint(mcast, IPV6 | DISCOVERY | MULTICAST, 5683, 0xff, scope,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0xfd);

  if (oc_init_post("/.knx", &mcast, NULL, NULL, LOW_QOS, NULL)) {

    /*
    { 4: <sia>, 5: { 6: <st>, 7: <ga>, 1: <value> } }
    */

    PRINT("before subtract and encode: %d\n",
          oc_rep_get_encoded_payload_size());
    // oc_rep_new(response_buffer.buffer, response_buffer.buffer_size);
    // oc_rep_subtract_length(oc_rep_get_encoded_payload_size());
    PRINT("after subtract, before encode: %d\n",
          oc_rep_get_encoded_payload_size());

    oc_rep_begin_root_object();

    oc_rep_i_set_int(root, 4, sia_value);

    oc_rep_i_set_key(&root_map, 5);
    CborEncoder value_map;
    cbor_encoder_create_map(&root_map, &value_map, CborIndefiniteLength);

    // ga
    oc_rep_i_set_int(value, 7, group_address);
    // st M Service type code(write = w, read = r, response = rp) Enum : w, r,
    // rp
    oc_rep_i_set_text_string(value, 6, rp);

    // set the "value" key
    oc_rep_i_set_key(&value_map, 5);
    // copy the data, this is already in cbor from the fake response of the
    // resource GET function
    oc_rep_encode_raw_encoder(&value_map, value_data, value_size);

    cbor_encoder_close_container_checked(&root_map, &value_map);

    oc_rep_end_root_object();

    PRINT("S-MODE Payload Size: %d\n", oc_rep_get_encoded_payload_size());

    if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  Sent POST request\n");
    } else {
      PRINT("  Could not send POST request\n");
    }
  }
}


void
oc_do_s_mode_internal(char *resource_url, char *rp, int x)
{
  (void)x;

  if (resource_url == NULL) {
    return;
  }

  oc_resource_t *my_resource =
    oc_ri_get_app_resource_by_uri(resource_url, strlen(resource_url), 0);
  if (my_resource == NULL) {
    PRINT(" oc_do_s_mode : error no URL found %s\n", resource_url);
    return;
  }

  // get the sender ia
  size_t device_index = 0;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  int sia_value = device->ia;

  uint8_t *buffer = malloc(100);
  if (!buffer) {
    OC_WRN("oc_do_s_mode: out of memory allocating buffer");
  } //! buffer

  oc_request_t request = { 0 };
  oc_response_t response = { 0 };
  response.separate_response = 0;
  oc_response_buffer_t response_buffer;
  // if (!response_buf && resource) {
  //  OC_DBG("coap_notify_observers: Issue GET request to resource %s\n\n",
  //         oc_string(resource->uri));
  response_buffer.buffer = buffer;
  response_buffer.buffer_size = 100;

  // same initialization as oc_ri.c
  response_buffer.code = 0;
  response_buffer.response_length = 0;
  response_buffer.content_format = 0;

  response.separate_response = NULL;
  response.response_buffer = &response_buffer;

  request.response = &response;
  request.request_payload = NULL;
  request.query = NULL;
  request.query_len = 0;
  request.resource = NULL;
  request.origin = NULL;
  request._payload = NULL;
  request._payload_len = 0;

  request.content_format = APPLICATION_CBOR;
  request.accept = APPLICATION_CBOR;
  request.uri_path = resource_url;
  request.uri_path_len = strlen(resource_url);

  oc_rep_new(response_buffer.buffer, response_buffer.buffer_size);

  // get the value...oc_request_t request_obj;
  oc_interface_mask_t iface_mask = OC_IF_NONE;
  // void *data;
  my_resource->get_handler.cb(&request, iface_mask, NULL);

  // get the data
  // int value_size = request.response->response_buffer->buffer_size;
  int value_size = oc_rep_get_encoded_payload_size();
  uint8_t *value_data = request.response->response_buffer->buffer;

  // Cache value data, as it gets overwritten in oc_issue_do_s_mode
  uint8_t buf[100];
  assert(value_size < 100);
  memcpy(buf, value_data, value_size);

  if (value_size == 0) {
    PRINT(" . ERROR: value size == 0");
    return;
  }

  int group_address = 0;

  // loop over all group addresses and issue the s-mode command
  int index = oc_core_find_group_object_table_url(resource_url);
  while (index != -1) {
    PRINT("  index : %d\n", index);

    int ga_len = oc_core_find_group_object_table_number_group_entries(index);
    for (int j = 0; j < ga_len; j++) {
      group_address = oc_core_find_group_object_table_group_entry(index, j);
      PRINT("   ga : %d\n", group_address);
      // issue the s-mode command
      oc_issue_s_mode(sia_value, group_address, rp, buf, value_size);
    }
    index = oc_core_find_next_group_object_table_url(resource_url, index);
  }

  // free buffer
  // free(buffer);
}



void
oc_do_s_mode(char *resource_url, char *rp)
{

  if (resource_url == NULL) {
    return;
  }

  oc_resource_t *my_resource =
    oc_ri_get_app_resource_by_uri(resource_url, strlen(resource_url), 0);
  if (my_resource == NULL) {
    PRINT(" oc_do_s_mode : error no URL found %s\n", resource_url);
    return;
  }

  // get the sender ia
  size_t device_index = 0;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  int sia_value = device->ia;

  uint8_t *buffer = malloc(100);
  if (!buffer) {
    OC_WRN("oc_do_s_mode: out of memory allocating buffer");
  } //! buffer

  oc_request_t request = { 0 };
  oc_response_t response = { 0 };
  response.separate_response = 0;
  oc_response_buffer_t response_buffer;
  // if (!response_buf && resource) {
  //  OC_DBG("coap_notify_observers: Issue GET request to resource %s\n\n",
  //         oc_string(resource->uri));
  response_buffer.buffer = buffer;
  response_buffer.buffer_size = 100;

  // same initialization as oc_ri.c
  response_buffer.code = 0;
  response_buffer.response_length = 0;
  response_buffer.content_format = 0;

  response.separate_response = NULL;
  response.response_buffer = &response_buffer;

  request.response = &response;
  request.request_payload = NULL;
  request.query = NULL;
  request.query_len = 0;
  request.resource = NULL;
  request.origin = NULL;
  request._payload = NULL;
  request._payload_len = 0;

  request.content_format = APPLICATION_CBOR;
  request.accept = APPLICATION_CBOR;
  request.uri_path = resource_url;
  request.uri_path_len = strlen(resource_url);

  oc_rep_new(response_buffer.buffer, response_buffer.buffer_size);

  // get the value...oc_request_t request_obj;
  oc_interface_mask_t iface_mask = OC_IF_NONE;
  // void *data;
  my_resource->get_handler.cb(&request, iface_mask, NULL);

  // get the data
  // int value_size = request.response->response_buffer->buffer_size;
  int value_size = oc_rep_get_encoded_payload_size();
  uint8_t *value_data = request.response->response_buffer->buffer;

  // Cache value data, as it gets overwritten in oc_issue_do_s_mode
  uint8_t buf[100];
  assert(value_size < 100);
  memcpy(buf, value_data, value_size);

  if (value_size == 0) {
    PRINT(" . ERROR: value size == 0");
    return;
  }

  int group_address = 0;

  // loop over all group addresses and issue the s-mode command
  int index = oc_core_find_group_object_table_url(resource_url);
  while (index != -1) {
    PRINT("  index : %d\n", index);

    int ga_len = oc_core_find_group_object_table_number_group_entries(index);
    for (int j = 0; j < ga_len; j++) {
      group_address = oc_core_find_group_object_table_group_entry(index, j);
      PRINT("   ga : %d\n", group_address);
      // issue the s-mode command
      oc_issue_s_mode(sia_value, group_address, rp, buf, value_size);
    }
    index = oc_core_find_next_group_object_table_url(resource_url, index);
  }

  // free buffer
  // free(buffer);
}



// ----------------------------------------------------------------------------

void
oc_knx_load_state(size_t device_index)
{
  int temp_size;

  oc_lsm_state_t lsm;
  PRINT("oc_knx_load_state: Loading Device Config from Persistent storage\n");

  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR(" could not get device %d\n", device_index);
  }

  temp_size = oc_storage_read(LSM_STORE, (uint8_t *)&lsm, sizeof(lsm));
  if (temp_size > 0) {
    device->lsm = lsm;
    PRINT("  load state (storage) %ld [%s]\n", (long)lsm,
          oc_core_get_lsm_as_string(lsm));
  }
}

void
oc_create_knx_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_resources");

  oc_create_knx_lsm_resource(OC_KNX_LSM, device_index);
  oc_create_knx_knx_resource(OC_KNX_DOT_KNX, device_index);
  oc_create_knx_crc_resource(OC_KNX_CRC, device_index);
  oc_create_knx_osn_resource(OC_KNX_OSN, device_index);
  oc_create_knx_ldevid_resource(OC_KNX_LDEVID, device_index);
  oc_create_knx_idevid_resource(OC_KNX_IDEVID, device_index);
  oc_create_knx_spake_resource(OC_KNX_SPAKE, device_index);
  oc_create_knx_resource(OC_KNX, device_index);
}
