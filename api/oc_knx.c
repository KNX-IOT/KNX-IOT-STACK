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
#include "oc_core_res.h"
#include "oc_knx.h"
#include "oc_knx_client.h"
#include "oc_knx_dev.h"
#include "oc_knx_fp.h"
#include "oc_knx_gm.h"
#include "oc_knx_sec.h"
#include "oc_main.h"
#include "oc_rep.h"
#include "oc_base64.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "port/dns-sd.h"

#ifdef OC_SPAKE
#include "security/oc_spake2plus.h"
#endif

#define TAGS_AS_STRINGS

#define LSM_STORE "LSM_STORE"
#define FINGERPRINT_STORE "dev_knx_fingerprint"
#define OSN_STORE "dev_knx_osn"

// ---------------------------Variables --------------------------------------

static oc_group_object_notification_t g_received_notification;

static uint64_t g_fingerprint = 0;
static uint64_t g_osn = 0;

static oc_pase_t g_pase;

static oc_string_t g_idevid;
static oc_string_t g_ldevid;

static bool g_ignore_smessage_from_self = false;

static int valid_request = 0;

// ----------------------------------------------------------------------------

enum SpakeKeys {
  SPAKE_ID = 0,
  SPAKE_SALT = 5,
  SPAKE_PW = 8, // For device handover, not implemented yet
  SPAKE_PA_SHARE_P = 10,
  SPAKE_PB_SHARE_V = 11,
  SPAKE_PBKDF2 = 12,
  SPAKE_CB_CONFIRM_V = 13,
  SPAKE_CA_CONFIRM_P = 14,
  SPAKE_RND = 15,
  SPAKE_IT = 16,
};

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
restart_device(size_t device_index)
{
  PRINT("restart device\n");

  // reset the programming mode
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR("device not found %d", (int)device_index);
  } else {
    device->pm = false;
  }

  // clear run time errors
  // ??

  // switch off safe state
  // ??

  oc_restart_t *my_restart = oc_get_restart_cb();
  if (my_restart && my_restart->cb) {
    // do a restart on application level
    my_restart->cb(device_index, my_restart->data);
  }

  return 0;
}

int
oc_reset_device(size_t device_index, int value)
{
  PRINT("reset device: %d\n", value);

  oc_knx_device_storage_reset(device_index, value);

  return 0;
}

// ----------------------------------------------------------------------------

/*
  payload example:
  {
    "api": {
      "version" : "1.0.0",
      "base": "/"
    }
  }
*/
static void
oc_core_knx_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                        void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_JSON &&
      request->accept != APPLICATION_CBOR && request->accept != CONTENT_NONE) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  if (request->accept == APPLICATION_JSON) {

    int length = oc_rep_add_line_to_buffer("{");
    response_length += length;

    length = oc_rep_add_line_to_buffer("\"api\": { \"version\": \"1.0.0\",");
    response_length += length;

    length = oc_rep_add_line_to_buffer("\"base\": \"/ \"}");
    response_length += length;

    length = oc_rep_add_line_to_buffer("}");
    response_length += length;

    oc_send_json_response(request, OC_STATUS_OK);
    request->response->response_buffer->response_length = response_length;
  } else {

    oc_rep_begin_root_object();
    oc_rep_set_object(root, api);
    oc_rep_set_text_string(api, version, "1.0.0");
    oc_rep_set_text_string(api, base, "/");
    oc_rep_close_object(root, api);
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
static size_t cached_device_index;
static int cached_value;

static oc_event_callback_retval_t
delayed_reset(void *context)
{
  oc_reset_device(cached_device_index, cached_value);
  return OC_EVENT_DONE;
}

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
          value = (int)rep->value.integer;
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
  PRINT("  value : %d\n", (int)value);

  bool error = true;
  size_t device_index = request->resource->device;

  if (cmd == RESTART_DEVICE) {
    restart_device(device_index);
    error = false;
  } else if (cmd == RESET_DEVICE) {
    // oc_reset_device(device_index, value);
    cached_device_index = device_index;
    cached_value = value;
    oc_set_delayed_callback_ms(NULL, delayed_reset, 100);
    error = false;
  }

  // Before executing the reset function,
  // the KNX IoT device MUST return a response with CoAP response 690 code 2.04
  // CHANGED and with payload containing Error Code and Process Time in seconds
  // as defined 691 for the Response to a Master Reset Request for KNX Classic
  // devices, see [10].
  if (error == true) {
    PRINT(" invalid command\n");
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  } else if (cmd == RESET_DEVICE) {
    oc_rep_begin_root_object();

    // TODO note need to figure out how to fill in the correct response values
    oc_rep_set_int(root, code, 0);
    oc_rep_set_int(root, time, 2);
    oc_rep_end_root_object();

    PRINT("oc_core_knx_post_handler %d - end\n",
          oc_rep_get_encoded_payload_size());
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx, knx_fp_g, 0, "/.well-known/knx",
                                     OC_IF_LI | OC_IF_SEC,
                                     APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                                     oc_core_knx_get_handler, 0,
                                     oc_core_knx_post_handler, 0, NULL,
                                     OC_SIZE_ZERO());

void
oc_create_knx_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx",
                            OC_IF_LI | OC_IF_SEC, APPLICATION_LINK_FORMAT,
                            OC_DISCOVERABLE, oc_core_knx_get_handler, 0,
                            oc_core_knx_post_handler, 0, 0);
}

// ----------------------------------------------------------------------------

oc_lsm_state_t
oc_a_lsm_state(size_t device_index)
{
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR("device not found %d", (int)device_index);
    return LSM_S_UNLOADED;
  }

  return device->lsm_s;
}

/*
 * function will store the new state
 */
int
oc_a_lsm_set_state(size_t device_index, oc_lsm_event_t new_state)
{
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR("device not found %d", (int)device_index);
    return -1;
  }
  device->lsm_s = new_state;

  oc_storage_write(LSM_STORE, (uint8_t *)&device->lsm_s, sizeof(device->lsm_s));

  return 0;
}

const char *
oc_core_get_lsm_state_as_string(oc_lsm_state_t lsm)
{
  // states
  if (lsm == LSM_S_UNLOADED) {
    return "unloaded";
  }
  if (lsm == LSM_S_LOADED) {
    return "loaded";
  }
  if (lsm == LSM_S_LOADING) {
    return "loading";
  }
  if (lsm == LSM_S_UNLOADING) {
    return "unloading";
  }
  if (lsm == LSM_S_LOADCOMPLETING) {
    return "load completing";
  }

  return "";
}

const char *
oc_core_get_lsm_event_as_string(oc_lsm_event_t lsm)
{
  // commands
  if (lsm == LSM_E_NOP) {
    return "nop";
  }
  if (lsm == LSM_E_STARTLOADING) {
    return "startLoading";
  }
  if (lsm == LSM_E_LOADCOMPLETE) {
    return "loadComplete";
  }
  if (lsm == LSM_E_UNLOAD) {
    return "unload";
  }

  return "";
}

/*
 * function will store the new state
 */
bool
oc_lsm_event_to_state(oc_lsm_event_t lsm_e, size_t device_index)
{
  if (lsm_e == LSM_E_NOP) {
    // do nothing
    return true;
  }
  if (lsm_e == LSM_E_STARTLOADING) {
    oc_a_lsm_set_state(device_index, LSM_S_LOADING);
    return true;
  }
  if (lsm_e == LSM_E_LOADCOMPLETE) {
    oc_a_lsm_set_state(device_index, LSM_S_LOADED);
    return true;
  }
  if (lsm_e == LSM_E_UNLOAD) {
    // do a reset
    oc_delete_group_rp_table();
    oc_delete_group_object_table();
    oc_a_lsm_set_state(device_index, LSM_S_UNLOADED);
    return true;
  }
  return false;
}

static void
oc_core_a_lsm_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;

  PRINT("oc_core_a_lsm_get_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  oc_lsm_state_t lsm = oc_a_lsm_state(device_index);

  oc_rep_begin_root_object();
  oc_rep_i_set_int(root, 3, lsm);
  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_a_lsm_get_handler - done\n");
}

static void
oc_core_a_lsm_post_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  oc_rep_t *rep = NULL;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  bool changed = false;
  int event = LSM_E_NOP;
  /* loop over the request document */
  rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 2) {
        event = (int)rep->value.integer;
        changed = true;
        break;
      }
    }
    rep = rep->next;
  } /* while */

  PRINT("  load event %d [%s]\n", event,
        oc_core_get_lsm_event_as_string((oc_lsm_event_t)event));
  // check the input and change the state
  changed = oc_lsm_event_to_state((oc_lsm_event_t)event, device_index);

  /* input was set, so create the response*/
  if (changed == true) {
    oc_loadstate_t *lsm_cb = oc_get_lsm_change_cb();

    if (lsm_cb->cb != NULL) {
      lsm_cb->cb(device_index, oc_a_lsm_state(device_index), lsm_cb->data);
    }

    if (oc_is_device_in_runtime(device_index)) {
      oc_register_group_multicasts();
      oc_init_datapoints_at_initialization();
      oc_device_info_t *device = oc_core_get_device_info(device_index);
      if (device) {
        knx_publish_service(oc_string(device->serialnumber), device->iid,
                            device->ia, device->pm);
      }
    }

    oc_rep_new(request->response->response_buffer->buffer,
               (int)request->response->response_buffer->buffer_size);
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 3, (int)oc_a_lsm_state(device_index));
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(a_lsm, knx_spake, 0, "/a/lsm", OC_IF_C,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_a_lsm_get_handler, 0,
                                     oc_core_a_lsm_post_handler, 0, NULL,
                                     OC_SIZE_ZERO());

void
oc_create_a_lsm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_a_lsm_resource\n");
  // "/a/lsm"
  oc_core_populate_resource(
    resource_idx, device, "/a/lsm", OC_IF_C, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_a_lsm_get_handler, 0, oc_core_a_lsm_post_handler, 0, 0);
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_k_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;

  PRINT("oc_core_knx_k_get_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
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
  // st M Service type code(write = w, read = r, response = rp) Enum : w, r, a
  // (rp)
  oc_rep_i_set_text_string(value, 6, oc_string(g_received_notification.st));
  // missing value

  cbor_encoder_close_container_checked(&root_map, &value_map);

  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_knx_k_get_handler - done\n");
}

// ----------------------------------------------------------------------------

bool
oc_s_mode_notification_to_json(char *buffer, size_t buffer_size,
                               oc_group_object_notification_t notification)
{
  // { 5: { 6: <st>, 7: <ga>, 1: <value> } }
  // { "s": { "st": <st>,  "ga": <ga>, "value": <value> } }
  int size = snprintf(
    buffer, buffer_size,
    "{\"sia\": %d, \"s\":{\"st\": \"%s\", \"ga\":%d, \"value\": %s } }",
    notification.sia, oc_string_checked(notification.st), notification.ga,
    oc_string_checked(notification.value));
  if ((int)size > buffer_size) {
    return false;
  }
  return true;
}

void
oc_reset_g_received_notification()
{
  g_received_notification.sia = 0;
  g_received_notification.ga = 0;
  // g_received_notification.value =

  oc_free_string(&g_received_notification.st);
  oc_new_string(&g_received_notification.st, "", strlen(""));
}

/*
 {sia: 5678, es: {st: write, ga: 1, value: 100 }}
*/
static void
oc_core_knx_k_post_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  oc_rep_t *rep = NULL;
  oc_rep_t *rep_value = NULL;
  char ip_address[100];

  PRINT("KNX K POST Handler\n");
  PRINT("Decoded Payload:\n");
  oc_print_rep_as_json(request->request_payload, true);

  PRINT("Full Payload Size: %d\n", (int)request->_payload_len);
  OC_LOGbytes_OSCORE(request->_payload, (int)request->_payload_len);

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR &&
      request->accept != APPLICATION_OSCORE &&
      request->accept != CONTENT_NONE) {
    request->response->response_buffer->code = oc_status_code(OC_IGNORE);
    return;
  }

  if (g_ignore_smessage_from_self) {
    // check if incoming message is from myself.
    // if so, then return with bad request
    // Note: The same device can have multiple IP addresses,
    // so all endpoints for this device need to be compared against.
    oc_endpoint_t *origin = request->origin;
    if (origin != NULL) {
      PRINT("k post : origin of message:");
      PRINTipaddr(*origin);
      PRINT("\n");
    }

    oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(0);
    oc_endpoint_t *ep_i = NULL;

    for (ep_i = my_ep; ep_i != NULL; ep_i = ep_i->next) {
      PRINTipaddr(*ep_i);
      PRINT("\n");

      if (oc_endpoint_compare_address(origin, ep_i) == 0) {
        if (origin->addr.ipv6.port == ep_i->addr.ipv6.port) {
          request->response->response_buffer->code = oc_status_code(OC_IGNORE);
          PRINT(" same address and port: not handling message");
          return;
        }
      }
    }
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_IGNORE);
    return;
  }

  oc_reset_g_received_notification();

  // get sender ip address
  SNPRINTFipaddr(ip_address, 100 - 1, *request->origin);

  /* loop over the request document to parse all the data */
  rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_INT: {
      // sia
      if (rep->iname == 4) {
        g_received_notification.sia = (uint32_t)rep->value.integer;
      }
    } break;
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;

      while (object != NULL) {
        switch (object->type) {
        case OC_REP_STRING: {
          if (object->iname == 6) {
            oc_free_string(&g_received_notification.st);
            oc_new_string(&g_received_notification.st,
                          oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;

        case OC_REP_INT: {
          // sia
          if (object->iname == 4) {
            g_received_notification.sia = (uint32_t)object->value.integer;
          }
          // ga
          if (object->iname == 7) {
            g_received_notification.ga = (uint32_t)object->value.integer;
          }
        } break;
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
  // maximum base64 overhead, plus one byte for the null terminator
  size_t base64_max_len = (request->_payload_len / 3 + 1) * 4 + 1;
  int base64_len;
  uint8_t *base64_buf = malloc(base64_max_len);

  oc_free_string(&g_received_notification.value);
  base64_len = oc_base64_encode(request->_payload, request->_payload_len,
                                base64_buf, base64_max_len);
  if (base64_len < 0) {
    char *error_msg = "Base64 encoding error in library!";
    oc_new_string(&g_received_notification.value, error_msg, strlen(error_msg));
    OC_ERR("%s", error_msg);
  } else {
    // add null terminator
    base64_buf[base64_len] = '0';
    oc_new_string(&g_received_notification.value, base64_buf, base64_len);
  }
  free(base64_buf);
  // gateway functionality: call back for all s-mode calls
  oc_gateway_t *my_gw = oc_get_gateway_cb();
  if (my_gw != NULL && my_gw->cb) {
    // call the gateway function
    my_gw->cb(device_index, ip_address, &g_received_notification, my_gw->data);
  }

  if (oc_is_device_in_runtime(device_index) == false) {
    PRINT(" Device not in runtime state:%d - ignore message", device->lsm_s);
    oc_send_cbor_response(request, OC_IGNORE);
    return;
  }

  bool st_write = false;
  bool st_rep = false;
  bool st_read = false;
  // handle the request
  // loop over the group addresses of the /fp/r
  PRINT(" k : origin:%s sia: %d ga: %d st: %s\n", ip_address,
        g_received_notification.sia, g_received_notification.ga,
        oc_string_checked(g_received_notification.st));
  if (strcmp(oc_string_checked(g_received_notification.st), "w") == 0) {
    // case_1 :
    // Received from bus: -st w, any ga ==> @receiver:
    // cflags = w -> overwrite object value
    st_write = true;
  } else if (strcmp(oc_string_checked(g_received_notification.st), "a") == 0) {
    // Case 2) spec 1.1
    // Received from bus: -st rp, any ga
    //@receiver: cflags = u -> overwrite object value
    st_rep = true;
  } else if (strcmp(oc_string_checked(g_received_notification.st), "rp") == 0) {
    // Case 2) spec 1.0
    // Received from bus: -st a (rp), any ga
    //@receiver: cflags = u -> overwrite object value
    st_rep = true;
  } else if (strcmp(oc_string_checked(g_received_notification.st), "r") == 0) {
    // Case 4)
    // @sender: cflags = r
    // Received from bus: -st r
    // Sent: -st rp, sending association (1st assigned ga)
    st_read = true;
  }

  int index = oc_core_find_group_object_table_index(g_received_notification.ga);
  PRINT(" k : index %d\n", index);
  if (index == -1) {
    // if nothing is found (initially) then return a bad request.
    oc_send_cbor_response(request, OC_IGNORE);
    return;
  }

  // create the dummy request
  oc_request_t new_request;
  memset(&new_request, 0, sizeof(oc_request_t));
  oc_response_buffer_t response_buffer;
  memset(&response_buffer, 0, sizeof(oc_response_buffer_t));
  oc_response_t response_obj;
  memset(&response_obj, 0, sizeof(oc_response_t));
  oc_ri_new_request_from_request(&new_request, request, &response_buffer,
                                 &response_obj);
  new_request.request_payload = oc_s_mode_get_value(request);
  // new style
  new_request.uri_path = "k";
  new_request.uri_path_len = 1;

  while (index != -1) {
    oc_string_t myurl = oc_core_find_group_object_table_url_from_index(index);
    PRINT(" k : url  %s\n", oc_string_checked(myurl));
    if (oc_string_len(myurl) > 0) {
      // get the resource to do the fake post on
      const oc_resource_t *my_resource = oc_ri_get_app_resource_by_uri(
        oc_string(myurl), oc_string_len(myurl), device_index);
      if (my_resource == NULL) {
        return;
      }

      // check if the data is allowed to write or update
      oc_cflag_mask_t cflags = oc_core_group_object_table_cflag_entries(index);
      if (((cflags & OC_CFLAG_WRITE) > 0) && (st_write)) {
        PRINT(" (case1) W-WRITE: index %d handled due to flags %d\n", index,
              cflags);
        // CASE 1:
        // Received from bus: -st w, any ga
        // @receiver : cflags = w->overwrite object value
        // to be discussed:
        // get value, since the w only should be send if the value is updated
        // (e.g. different)
        // calling the put handler, since datapoints are implementing GET/PUT
        if (my_resource->put_handler.cb) {
          my_resource->put_handler.cb(&new_request, iface_mask,
                                      my_resource->put_handler.user_data);
          if ((cflags & OC_CFLAG_TRANSMISSION) > 0) {
            // Case 3) part 1
            // @sender : updated object value + cflags = t
            // Sent : -st w, sending association(1st assigned ga)
            PRINT("  (case3) (W-WRITE) sending WRITE due to TRANSMIT flag \n");
#ifdef OC_USE_MULTICAST_SCOPE_2
            oc_do_s_mode_with_scope(2, oc_string(myurl), "w");
#endif
            oc_do_s_mode_with_scope(5, oc_string(myurl), "w");
          }
        }
      }
      if (((cflags & OC_CFLAG_UPDATE) > 0) && (st_rep)) {
        PRINT(" (case2) RP-UPDATE: index %d handled due to flags %d\n", index,
              cflags);
        // Case 2)
        // Received from bus: -st rp , any ga
        // @receiver : cflags = u->overwrite object value
        // calling the put handler, since datapoints are implementing GET/PUT
        if (my_resource->put_handler.cb) {
          my_resource->put_handler.cb(&new_request, iface_mask,
                                      my_resource->put_handler.user_data);
          if ((cflags & OC_CFLAG_TRANSMISSION) > 0) {
            PRINT(
              "   (case3) (RP-UPDATE) sending WRITE due to TRANSMIT flag \n");
            // Case 3) part 2
            // @sender : updated object value + cflags = t
            // Sent : -st w, sending association(1st assigned ga)
#ifdef OC_USE_MULTICAST_SCOPE_2
            oc_do_s_mode_with_scope(2, oc_string(myurl), "w");
#endif
            oc_do_s_mode_with_scope(5, oc_string(myurl), "w");
          }
        }
      }
      if (((cflags & OC_CFLAG_READ) > 0) && (st_read)) {
        PRINT(" (case4) (R-READ) index %d handled due to flags %d\n", index,
              cflags);
        // Case 4)
        // @sender: cflags = r
        // Received from bus: -st r
        // Sent: -st rp, sending association (1st assigned ga)
        // specifically: do not check the transmission flag
        PRINT("   (case3) (RP-UPDATE) sending RP due to READ flag \n");

#ifdef OC_USE_MULTICAST_SCOPE_2
        // oc_do_s_mode_with_scope_no_check(2, oc_string(myurl), "rp");
        oc_do_s_mode_with_scope_no_check(2, oc_string(myurl), "a");
#endif
        // oc_do_s_mode_with_scope_no_check(5, oc_string(myurl), "rp");
        oc_do_s_mode_with_scope_no_check(5, oc_string(myurl), "a");
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
    PRINT(" k : Multicast - not sending response\n");
    oc_send_cbor_response(request, OC_IGNORE);
    return;
  }
  // send the response
  oc_send_response_no_format(request, OC_STATUS_CHANGED);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_k, knx_fingerprint, 0, "/k",
                                     OC_IF_LI | OC_IF_G, APPLICATION_CBOR,
                                     OC_DISCOVERABLE, oc_core_knx_k_get_handler,
                                     0, oc_core_knx_k_post_handler, 0, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:g.s");
void
oc_create_knx_k_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_k_resource (g)\n");

  oc_core_populate_resource(resource_idx, device, "/k", OC_IF_LI | OC_IF_G,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_knx_k_get_handler, 0,
                            oc_core_knx_k_post_handler, 0, 1, "urn:knx:g.s");
}

int
oc_knx_knx_ignore_smessage_from_self(bool ignore)
{
  g_ignore_smessage_from_self = ignore;
  return 0;
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_fingerprint_get_handler(oc_request_t *request,
                                    oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_knx_fingerprint_get_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // check if the state is loaded
  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_LOADED) {
    OC_ERR(" not in loaded state\n");
    oc_send_response_no_format(request, OC_STATUS_SERVICE_UNAVAILABLE);
    return;
  }

  // cbor_encode_uint(&g_encoder, g_fingerprint);
  oc_rep_begin_root_object();
  oc_rep_i_set_int(root, 1, g_fingerprint);
  oc_rep_end_root_object();

  PRINT("oc_core_knx_fingerprint_get_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fingerprint, knx_ia, 0,
                                     "/.well-known/knx/f", OC_IF_C,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_knx_fingerprint_get_handler, 0, 0,
                                     0, NULL, OC_SIZE_ZERO());

void
oc_create_knx_fingerprint_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_fingerprint_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/f", OC_IF_C,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_knx_fingerprint_get_handler, 0, 0, 0, 0);
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_ia_post_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  bool error = false;
  bool ia_set = false;
  bool iid_set = false;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 12) {
        PRINT("  oc_core_knx_ia_post_handler received 12 (ia) : %d\n",
              (int)rep->value.integer);
        oc_core_set_device_ia(device_index, (uint32_t)rep->value.integer);
        int temp = (int)rep->value.integer;
        oc_storage_write(KNX_STORAGE_IA, (uint8_t *)&temp, sizeof(temp));
        ia_set = true;
      } else if (rep->iname == 25) {
        PRINT("  oc_core_knx_ia_post_handler received 25 (fid): %" PRIu64 "\n",
              rep->value.integer);
        oc_core_set_device_fid(device_index, (uint64_t)rep->value.integer);
        uint64_t temp = (uint64_t)rep->value.integer;
        oc_storage_write(KNX_STORAGE_FID, (uint8_t *)&temp, sizeof(temp));
      } else if (rep->iname == 26) {
        PRINT("  oc_core_knx_ia_post_handler received 26 (iid): %" PRIu64 "\n",
              (uint64_t)rep->value.integer);
        oc_core_set_device_iid(device_index, (uint64_t)rep->value.integer);
        uint64_t temp = (uint64_t)rep->value.integer;
        oc_storage_write(KNX_STORAGE_IID, (uint8_t *)&temp, sizeof(temp));
        iid_set = true;
      }
    }
    rep = rep->next;
  }

  if (iid_set && ia_set) {
    // do the run time installation
    if (oc_is_device_in_runtime(device_index)) {
      oc_register_group_multicasts();
      oc_init_datapoints_at_initialization();
      oc_device_info_t *device = oc_core_get_device_info(device_index);
      knx_publish_service(oc_string(device->serialnumber), device->iid,
                          device->ia, device->pm);
    }
    oc_send_response_no_format(request, OC_STATUS_CHANGED);
  } else {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
  }
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_ia, knx_osn, 0, "/.well-known/knx/ia",
                                     OC_IF_C, APPLICATION_CBOR, OC_DISCOVERABLE,
                                     NULL, 0, oc_core_knx_ia_post_handler, 0,
                                     NULL, OC_SIZE_ZERO());

void
oc_create_knx_ia(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_ia\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/ia",
                            OC_IF_C, APPLICATION_CBOR, OC_DISCOVERABLE, NULL, 0,
                            oc_core_knx_ia_post_handler, 0, 0, "");
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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // cbor_encode_uint(&g_encoder, g_osn);
  oc_rep_begin_root_object();
  oc_rep_i_set_int(root, 1, g_osn);
  oc_rep_end_root_object();

  PRINT("oc_core_knx_osn_get_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_osn, knx, 0, "/.well-known/knx/osn",
                                     OC_IF_NONE, APPLICATION_CBOR,
                                     OC_DISCOVERABLE,
                                     oc_core_knx_osn_get_handler, 0, 0, 0, NULL,
                                     OC_SIZE_ZERO());

void
oc_create_knx_osn_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_osn_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/osn",
                            OC_IF_NONE, APPLICATION_CBOR, OC_DISCOVERABLE,
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
  if (oc_check_accept_header(request, APPLICATION_PKCS7_CMC_REQUEST) == false) {
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

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_ldevid, knx_k, 0,
                                     "/.well-known/knx/ldevid", OC_IF_D,
                                     APPLICATION_PKCS7_CMC_REQUEST,
                                     OC_DISCOVERABLE,
                                     oc_core_knx_ldevid_get_handler, 0, 0, 0,
                                     NULL, OC_SIZE_MANY(1), ":dpt.a[n]");
/* optional resource */
void
oc_create_knx_ldevid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_ldevid_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/ldevid",
                            OC_IF_D, APPLICATION_PKCS7_CMC_REQUEST,
                            OC_DISCOVERABLE, oc_core_knx_ldevid_get_handler, 0,
                            0, 0, 1, ":dpt.a[n]");
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
  if (oc_check_accept_header(request, APPLICATION_PKCS7_CMC_REQUEST) == false) {
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

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_idevid, knx_ldevid, 0,
                                     "/.well-known/knx/idevid", OC_IF_D,
                                     APPLICATION_PKCS7_CMC_REQUEST,
                                     OC_DISCOVERABLE,
                                     oc_core_knx_idevid_get_handler, 0, 0, 0,
                                     NULL, OC_SIZE_MANY(1), ":dpt.a[n]");

void
oc_create_knx_idevid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_idevid_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/idevid",
                            OC_IF_D, APPLICATION_PKCS7_CMC_REQUEST,
                            OC_DISCOVERABLE, oc_core_knx_idevid_get_handler, 0,
                            0, 0, 1, ":dpt.a[n]");
}

// ----------------------------------------------------------------------------

#ifdef OC_SPAKE
static spake_data_t spake_data = { 0 };
static int failed_handshake_count = 0;

static bool is_blocking = false;

static oc_event_callback_retval_t
decrement_counter(void *data)
{
  if (failed_handshake_count > 0) {
    --failed_handshake_count;
  }

  if (is_blocking && failed_handshake_count == 0) {
    is_blocking = false;
  }
  return OC_EVENT_CONTINUE;
}

static void
increment_counter()
{
  ++failed_handshake_count;
}

static bool
is_handshake_blocked()
{
  if (is_blocking) {
    return true;
  }

  // after 10 failed attempts per minute, block the client for the
  // next minute
  if (failed_handshake_count > 10) {
    is_blocking = true;
    return true;
  }

  return false;
}

static int
get_seconds_until_unblocked()
{
  return failed_handshake_count * 10;
}

#endif /* OC_SPAKE */

static oc_separate_response_t spake_separate_rsp;
static oc_event_callback_retval_t oc_core_knx_spake_separate_post_handler(
  void *req_p);

static void
oc_core_knx_spake_post_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_knx_spake_post_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // check if the state is unloaded
  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_UNLOADED) {
    OC_ERR(" not in unloaded state\n");
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

#ifdef OC_SPAKE
  if (is_handshake_blocked()) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_SERVICE_UNAVAILABLE);

    request->response->response_buffer->max_age = get_seconds_until_unblocked();
    return;
  }
#endif /* OC_SPAKE */

  oc_rep_t *rep = request->request_payload;

  // check input
  // note: no check if there are multiple byte strings in the request payload
  valid_request = 0;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_BYTE_STRING: {
      if (rep->iname == SPAKE_PA_SHARE_P) {
        valid_request = SPAKE_PA_SHARE_P;
      }
      if (rep->iname == SPAKE_CA_CONFIRM_P) {
        valid_request = SPAKE_CA_CONFIRM_P;
      }
      if (rep->iname == SPAKE_RND) {
        valid_request = SPAKE_RND;
      }
    } break;
    default:
      break;
    }
    rep = rep->next;
  }

  if (valid_request == 0) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  rep = request->request_payload;

  if (valid_request == SPAKE_RND) {
    // set the default id, in preparation for the response
    // this gets overwritten if the ID is present in the
    // request payload handled below
    oc_free_string(&g_pase.id);
    oc_new_byte_string(&g_pase.id, "rkey", strlen("rkey"));
  }
  // handle input
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_BYTE_STRING: {
      if (rep->iname == SPAKE_CA_CONFIRM_P) {
        memcpy(g_pase.ca, oc_cast(rep->value.string, uint8_t),
               sizeof(g_pase.ca));
      }
      if (rep->iname == SPAKE_PA_SHARE_P) {
        memcpy(g_pase.pa, oc_cast(rep->value.string, uint8_t),
               sizeof(g_pase.pa));
      }
      if (rep->iname == SPAKE_RND) {
        memcpy(g_pase.rnd, oc_cast(rep->value.string, uint8_t),
               sizeof(g_pase.rnd));
      }
      if (rep->iname == SPAKE_ID) {
        // if the ID is present, overwrite the default
        oc_free_string(&g_pase.id);
        oc_new_byte_string(&g_pase.id, oc_string(rep->value.string),
                           oc_string_len(rep->value.string));
        PRINT("==> CLIENT RECEIVES %d\n",
              (int)oc_byte_string_len(rep->value.string));
      }
    } break;
    case OC_REP_STRING: {
      if (rep->iname == SPAKE_ID) {
        // if the ID is present, overwrite the default
        oc_free_string(&g_pase.id);
        oc_new_byte_string(&g_pase.id, oc_string(rep->value.string),
                           oc_string_len(rep->value.string));
        PRINT("==> CLIENT RECEIVES %d\n",
              (int)oc_byte_string_len(rep->value.string));
      }
    } break;
    default:
      break;
    }
    rep = rep->next;
  }

  PRINT("oc_core_knx_spake_post_handler valid_request: %d\n", valid_request);
  oc_indicate_separate_response(request, &spake_separate_rsp);
  oc_set_delayed_callback(NULL, &oc_core_knx_spake_separate_post_handler, 0);
}

static oc_event_callback_retval_t
oc_core_knx_spake_separate_post_handler(void *req_p)
{
  (void)req_p;
  PRINT("oc_core_knx_spake_separate_post_handler\n");

  if (!spake_separate_rsp.active) {
    return OC_EVENT_DONE;
  }
  oc_set_separate_response_buffer(&spake_separate_rsp);

  if (valid_request == SPAKE_RND) {
#ifdef OC_SPAKE
    // get random numbers for rnd, salt & it (# of iterations)
    oc_spake_get_pbkdf_params(g_pase.rnd, g_pase.salt, &g_pase.it);
    OC_DBG_SPAKE("Rnd:");
    OC_LOGbytes_SPAKE(g_pase.rnd, sizeof(g_pase.rnd));
    OC_DBG_SPAKE("Salt:");
    OC_LOGbytes_SPAKE(g_pase.salt, sizeof(g_pase.salt));
    OC_DBG_SPAKE("Iterations: %d", g_pase.it);

#endif /* OC_SPAKE */
    oc_rep_begin_root_object();
    // id (0)
    // oc_rep_i_set_byte_string(root, SPAKE_ID, oc_cast(g_pase.id, uint8_t),
    //                         oc_byte_string_len(g_pase.id));
    // rnd (15)
    oc_rep_i_set_byte_string(root, SPAKE_RND, g_pase.rnd, 32);
    // pbkdf2
    oc_rep_i_set_key(&root_map, SPAKE_PBKDF2);
    oc_rep_begin_object(&root_map, pbkdf2);
    // it 16
    oc_rep_i_set_int(pbkdf2, SPAKE_IT, g_pase.it);
    // salt 5
    oc_rep_i_set_byte_string(pbkdf2, SPAKE_SALT, g_pase.salt, 32);
    oc_rep_end_object(&root_map, pbkdf2);
    oc_rep_end_root_object();
    oc_send_separate_response(&spake_separate_rsp, OC_STATUS_CHANGED);
    return OC_EVENT_DONE;
  }
#ifdef OC_SPAKE
  else if (valid_request == SPAKE_PA_SHARE_P) {
    // return changed, frame pb (11) & cb (13)

    const char *password = oc_spake_get_password();
    int ret;
    mbedtls_mpi_free(&spake_data.w0);
    mbedtls_ecp_point_free(&spake_data.L);
    mbedtls_mpi_free(&spake_data.y);
    mbedtls_ecp_point_free(&spake_data.pub_y);

    mbedtls_mpi_init(&spake_data.w0);
    mbedtls_ecp_point_init(&spake_data.L);
    mbedtls_mpi_init(&spake_data.y);
    mbedtls_ecp_point_init(&spake_data.pub_y);

    ret = oc_spake_get_w0_L(password, sizeof(g_pase.salt), g_pase.salt,
                            g_pase.it, &spake_data.w0, &spake_data.L);
    if (ret != 0) {
      OC_ERR("oc_spake_get_w0_L failed with code %d", ret);
      goto error;
    }

    ret = oc_spake_gen_keypair(&spake_data.y, &spake_data.pub_y);
    if (ret != 0) {
      OC_ERR("oc_spake_gen_keypair failed with code %d", ret);
      goto error;
    }

    // next step: calculate pB, encode it into the struct
    mbedtls_ecp_point pB;
    mbedtls_ecp_point_init(&pB);
    if (ret = oc_spake_calc_shareV(&pB, &spake_data.pub_y, &spake_data.w0)) {
      OC_ERR("oc_spake_calc_pB failed with code %d", ret);
      mbedtls_ecp_point_free(&pB);
      goto error;
    }

    if (ret = oc_spake_encode_pubkey(&pB, g_pase.pb)) {
      OC_ERR("oc_spake_encode_pubkey failed with code %d", ret);
      mbedtls_ecp_point_free(&pB);
      goto error;
    }

    if (ret = oc_spake_calc_transcript_responder(&spake_data, g_pase.pa, &pB)) {
      OC_ERR("oc_spake_calc_transcript_responder failed with code %d", ret);
      mbedtls_ecp_point_free(&pB);
      goto error;
    }

    oc_spake_calc_confirmV(spake_data.K_main, g_pase.cb, g_pase.pa);
    mbedtls_ecp_point_free(&pB);

    oc_rep_begin_root_object();
    // pb (11)
    oc_rep_i_set_byte_string(root, SPAKE_PB_SHARE_V, g_pase.pb,
                             sizeof(g_pase.pb));
    // cb (13)
    oc_rep_i_set_byte_string(root, SPAKE_CB_CONFIRM_V, g_pase.cb,
                             sizeof(g_pase.cb));
    oc_rep_end_root_object();
    oc_send_separate_response(&spake_separate_rsp, OC_STATUS_CHANGED);
    return OC_EVENT_DONE;
  } else if (valid_request == SPAKE_CA_CONFIRM_P) {
    // calculate expected cA
    uint8_t expected_ca[32];

    OC_DBG_SPAKE("KaKe & pB Bytes");
    OC_LOGbytes_OSCORE(spake_data.K_main, 32);
    OC_LOGbytes_OSCORE(g_pase.pb, sizeof(g_pase.pb));
    oc_spake_calc_confirmP(spake_data.K_main, expected_ca, g_pase.pb);
    OC_DBG_SPAKE("cA:");
    OC_LOGbytes_OSCORE(expected_ca, 32);

    if (memcmp(expected_ca, g_pase.ca, sizeof(g_pase.ca)) != 0) {
      OC_ERR("oc_spake_calc_confirmP failed");
      goto error;
    }

    // shared_key is 16-byte array - NOT NULL TERMINATED
    uint8_t shared_key[16];
    uint8_t shared_key_len = sizeof(shared_key);
    oc_spake_calc_K_shared(spake_data.K_main, shared_key);

    // set thet /auth/at entry with the calculated shared key
    // size_t device_index = request->resource->device;

    // knx does not have multiple devices per instance (for now), so hardcode
    // the use of the first device
    oc_device_info_t *device = oc_core_get_device_info(0);
    // serial number should be supplied as string array
    PRINT("CLIENT: pase.id length: %d\n", (int)oc_byte_string_len(g_pase.id));
    oc_oscore_set_auth_device(oc_string(g_pase.id),
                              oc_byte_string_len(g_pase.id), "", 0, shared_key,
                              shared_key_len);

    // empty payload
    oc_send_empty_separate_response(&spake_separate_rsp, OC_STATUS_CHANGED);

    // handshake completed successfully - clear state
    memset(spake_data.K_main, 0, sizeof(spake_data.K_main));
    mbedtls_ecp_point_free(&spake_data.L);
    mbedtls_ecp_point_free(&spake_data.pub_y);
    mbedtls_mpi_free(&spake_data.w0);
    mbedtls_mpi_free(&spake_data.y);

    mbedtls_ecp_point_init(&spake_data.L);
    mbedtls_ecp_point_init(&spake_data.pub_y);
    mbedtls_mpi_init(&spake_data.w0);
    mbedtls_mpi_init(&spake_data.y);

    memset(g_pase.pa, 0, sizeof(g_pase.pa));
    memset(g_pase.pb, 0, sizeof(g_pase.pb));
    memset(g_pase.ca, 0, sizeof(g_pase.ca));
    memset(g_pase.cb, 0, sizeof(g_pase.cb));
    memset(g_pase.rnd, 0, sizeof(g_pase.rnd));
    memset(g_pase.salt, 0, sizeof(g_pase.salt));
    g_pase.it = 100000;
    return OC_EVENT_DONE;
  }
error:
  // be paranoid: wipe all global data after an error
  memset(spake_data.K_main, 0, sizeof(spake_data.K_main));
  mbedtls_ecp_point_free(&spake_data.L);
  mbedtls_ecp_point_free(&spake_data.pub_y);
  mbedtls_mpi_free(&spake_data.w0);
  mbedtls_mpi_free(&spake_data.y);

  mbedtls_ecp_point_init(&spake_data.L);
  mbedtls_ecp_point_init(&spake_data.pub_y);
  mbedtls_mpi_init(&spake_data.w0);
  mbedtls_mpi_init(&spake_data.y);
#endif /* OC_SPAKE */

  memset(g_pase.pa, 0, sizeof(g_pase.pa));
  memset(g_pase.pb, 0, sizeof(g_pase.pb));
  memset(g_pase.ca, 0, sizeof(g_pase.ca));
  memset(g_pase.cb, 0, sizeof(g_pase.cb));
  memset(g_pase.rnd, 0, sizeof(g_pase.rnd));
  memset(g_pase.salt, 0, sizeof(g_pase.salt));
  g_pase.it = 100000;

#ifdef OC_SPAKE
  increment_counter();
#endif /* OC_SPAKE */
  oc_send_separate_response(&spake_separate_rsp, OC_STATUS_BAD_REQUEST);
  return OC_EVENT_DONE;
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_spake, knx_idevid, 0,
                                     "/.well-known/knx/spake", OC_IF_NONE,
                                     APPLICATION_CBOR, OC_DISCOVERABLE, 0, 0,
                                     oc_core_knx_spake_post_handler, 0, NULL,
                                     OC_SIZE_ZERO());

void
oc_create_knx_spake_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_spake_resource\n");
  oc_core_populate_resource(resource_idx, device, "/.well-known/knx/spake",
                            OC_IF_NONE, APPLICATION_CBOR, OC_DISCOVERABLE, 0, 0,
                            oc_core_knx_spake_post_handler, 0, 0);
}

#ifdef OC_SPAKE
void
oc_initialise_spake_data()
{
  // can fail if initialization of the RNG does not work
  int ret = oc_spake_init();
  assert(ret == 0);
  mbedtls_mpi_init(&spake_data.w0);
  mbedtls_ecp_point_init(&spake_data.L);
  mbedtls_mpi_init(&spake_data.y);
  mbedtls_ecp_point_init(&spake_data.pub_y);
  // start SPAKE brute force protection timer
  oc_set_delayed_callback(NULL, decrement_counter, 10);
}
#endif /* OC_SPAKE */

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

// ----------------------------------------------------------------------------

void
oc_knx_load_fingerprint()
{
  g_fingerprint = 0;
  oc_storage_read(FINGERPRINT_STORE, (uint8_t *)&g_fingerprint,
                  sizeof(g_fingerprint));
}

void
oc_knx_dump_fingerprint()
{
  oc_storage_write(FINGERPRINT_STORE, (uint8_t *)&g_fingerprint,
                   sizeof(g_fingerprint));
}

void
oc_knx_set_fingerprint(uint64_t fingerprint)
{
  g_fingerprint = fingerprint;
}

void
oc_knx_increase_fingerprint()
{
  g_fingerprint++;
  oc_knx_dump_fingerprint();
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
    OC_ERR(" could not get device %d\n", (int)device_index);
    return;
  }

  temp_size = oc_storage_read(LSM_STORE, (uint8_t *)&lsm, sizeof(lsm));
  if (temp_size > 0) {
    device->lsm_s = lsm;
    PRINT("  load state (storage) %ld [%s]\n", (long)lsm,
          oc_core_get_lsm_state_as_string((oc_lsm_state_t)lsm));
  }

  oc_knx_load_fingerprint();
}

void
oc_create_knx_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_resources");
  if (device_index == 0) {
    OC_DBG("resources for dev 0 created statically");
    return;
  }

  oc_create_a_lsm_resource(OC_A_LSM, device_index);
  oc_create_knx_k_resource(OC_KNX_K, device_index);
  oc_create_knx_fingerprint_resource(OC_KNX_FINGERPRINT, device_index);
  oc_create_knx_ia(OC_KNX_IA, device_index);
  oc_create_knx_osn_resource(OC_KNX_OSN, device_index);
  oc_create_knx_ldevid_resource(OC_KNX_LDEVID, device_index);
  oc_create_knx_idevid_resource(OC_KNX_IDEVID, device_index);
  oc_create_knx_spake_resource(OC_KNX_SPAKE, device_index);
  oc_create_knx_resource(OC_KNX, device_index);
}

// ----------------------------------------------------------------------------

bool
oc_is_device_in_runtime(size_t device_index)
{
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device->ia <= 0) {
    return false;
  }
  if (device->iid <= 0) {
    return false;
  }
  if (device->lsm_s != LSM_S_LOADED) {
    return false;
  }

  return true;
}