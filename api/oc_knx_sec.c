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
#include "api/oc_knx_sec.h"
#include "oc_discovery.h"
#include "oc_core_res.h"
#include <stdio.h>

uint64_t g_oscore_replaywindow = 0;
uint64_t g_oscore_osndelay = 0;

oc_oscore_cc_t g_occ[20];  

// ----------------------------------------------------------------------------

static void
oc_core_knx_f_oscore_osndelay_get_handler(oc_request_t *request,
                                          oc_interface_mask_t iface_mask,
                                          void *data)
{
  (void)data;
  (void)iface_mask;

  PRINT("oc_core_knx_f_oscore_osndelay_get_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  cbor_encode_uint(&g_encoder, g_oscore_osndelay);

  PRINT("oc_core_knx_f_oscore_osndelay_get_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

static void
oc_core_knx_p_oscore_osndelay_put_handler(oc_request_t *request,
                                          oc_interface_mask_t iface_mask,
                                          void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_knx_p_oscore_osndelay_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    // PRINT("  oc_core_knx_p_oscore_osndelay_put_handler received : %d\n",
    //      rep->value.integer);
    g_oscore_osndelay = rep->value.integer;
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    // oc_storage_write(KNX_STORAGE_PM, (uint8_t *)&(rep->value.boolean), 1);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_knx_p_oscore_osndelay_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_p_oscore_osndelay_resource\n");
  //
  oc_core_lf_populate_resource(
    resource_idx, device, "p/oscore/osndelay", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_knx_f_oscore_osndelay_get_handler,
    oc_core_knx_p_oscore_osndelay_put_handler, 0, 0, 1, ":dpt:timePeriodMsec");
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_p_oscore_replwdo_get_handler(oc_request_t *request,
                                         oc_interface_mask_t iface_mask,
                                         void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  cbor_encode_uint(&g_encoder, g_oscore_replaywindow);

  PRINT("oc_core_knx_f_oscore_osndelay_get_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

static void
oc_core_knx_p_oscore_replwdo_put_handler(oc_request_t *request,
                                         oc_interface_mask_t iface_mask,
                                         void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_knx_p_oscore_replwdo_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    // PRINT("  oc_core_knx_p_oscore_replwdo_put_handler received : %d\n",
    //      rep->value.integer);
    g_oscore_replaywindow = rep->value.integer;
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    // oc_storage_write(KNX_STORAGE_PM, (uint8_t *)&(rep->value.boolean), 1);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_knx_p_oscore_replwdo_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_p_oscore_replwdo_resource\n");
  //
  oc_core_lf_populate_resource(
    resource_idx, device, "/p/oscore/replwdo", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_knx_p_oscore_replwdo_get_handler,
    oc_core_knx_p_oscore_replwdo_put_handler, 0, 0, 1, ":dpt.value2UCount");
}

// ----------------------------------------------------------------------------

static void
oc_core_knx_f_oscore_get_handler(oc_request_t *request,
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

  for (i = (int)OC_KNX_P_OSCORE_REPLWDO; i <= (int)OC_KNX_P_OSCORE_OSNDELAY;
       i++) {
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
oc_create_knx_f_oscore_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_f_oscore_resource\n");
  //
  oc_core_lf_populate_resource(resource_idx, device, "/f/oscore", OC_IF_LIL,
                               APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                               oc_core_knx_f_oscore_get_handler, 0, 0, 0, 1,
                               "urn:knx:fbswu");
}

// ----------------------------------------------------------------------------

#define LDEVID_RENEW 1
#define LDEVID_STOP 2


static int
a_sen_convert_cmd(char *cmd)
{
  if (strncmp(cmd, "renew", strlen("renew")) == 0) {
    return LDEVID_RENEW;
  }
  if (strncmp(cmd, "stop", strlen("stop")) == 0) {
    return LDEVID_STOP;
  }

  OC_DBG("convert_cmd command not recognized: %s", cmd);
  return 0;
}


// { 2: "renew" }
static void
oc_core_a_sen_post_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  oc_rep_t *rep = NULL;
  int cmd = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  bool changed = false;
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;
  while (rep != NULL) {
    PRINT("key: (check) %s \n", oc_string(rep->name));
    if (rep->type == OC_REP_STRING) {
      if (rep->iname == 2) {
        cmd = a_sen_convert_cmd(oc_string(rep->value.string));
        changed = true;
        break;
      }
    }

    rep = rep->next;
  }

  /* input was set, so create the response*/
  if (changed == true) {
    PRINT("  cmd %d\n", cmd);
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_a_sen_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_a_sen_resource\n");
  // "/a/sen"
  oc_core_lf_populate_resource(resource_idx, device, "/a/sen",
                               OC_IF_LL | OC_IF_BASELINE, APPLICATION_CBOR,
                               OC_DISCOVERABLE, 0, 0,
                               oc_core_a_sen_post_handler, 0, 0, "");
}


// ----------------------------------------------------------------------------

/*
{
"access_token": "OC5BLLhkAG ...",
"profile": "coap_oscore",
"scope": ["if.g.s.<ga>"],
"cnf": {
"osc": {
"alg": "AES-CCM-16-64-128",
"id": "<kid>",
"ms": "f9af8….6bd94e6f"
}}}
*/
static void
oc_core_auth_at_post_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  oc_rep_t *rep = NULL;
  int cmd = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  bool changed = false;
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;
  while (rep != NULL) {
    PRINT("key: (check) %s \n", oc_string(rep->name));
    if (rep->type == OC_REP_STRING) {
      if (rep->iname == 2) {
        cmd = a_sen_convert_cmd(oc_string(rep->value.string));
        changed = true;
        break;
      }
    }

    rep = rep->next;
  }

  /* input was set, so create the response*/
  if (changed == true) {
    PRINT("  cmd %d\n", cmd);
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_auth_at_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_auth_at_resource\n");
  // "/a/sen"
  oc_core_lf_populate_resource(
    resource_idx, device, "/auth/at", OC_IF_LL | OC_IF_BASELINE, APPLICATION_CBOR,
    OC_DISCOVERABLE, 0, 0, oc_core_auth_at_post_handler, 0, 0, "");
}


// ----------------------------------------------------------------------------

static void
oc_core_knx_auth_get_handler(oc_request_t *request,
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

  for (i = (int)OC_KNX_A_SEN; i = (int)OC_KNX_AUTH;
       i++) {
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
oc_create_knx_auth_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_auth_resource\n");
  //
  oc_core_lf_populate_resource(resource_idx, device, "/auth", OC_IF_LIL,
                               APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                               oc_core_knx_auth_get_handler, 0, 0, 0, 1,
                               "urn:knx:fbswu");
}

// ----------------------------------------------------------------------------

uint64_t
oc_oscore_get_rplwdo()
{
  return g_oscore_replaywindow;
}

uint64_t
oc_oscore_get_osndelay()
{
  return g_oscore_osndelay;
}

// ----------------------------------------------------------------------------

void
oc_create_knx_sec_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_sec_resources");

  oc_create_knx_p_oscore_replwdo_resource(OC_KNX_P_OSCORE_REPLWDO,
                                          device_index);
  oc_create_knx_p_oscore_osndelay_resource(OC_KNX_P_OSCORE_OSNDELAY,
                                           device_index);
  oc_create_knx_f_oscore_resource(OC_KNX_F_OSCORE, device_index);

  oc_create_a_sen_resource(OC_KNX_A_SEN, device_index);

  oc_create_auth_at_resource(OC_KNX_AUTH_AT, device_index);
  oc_create_knx_auth_resource(OC_KNX_AUTH, device_index);
}
