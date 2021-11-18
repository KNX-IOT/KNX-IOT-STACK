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

/** the list of connections */
#define G_OCM_MAX_ENTRIES 20
oc_oscore_cm_t g_ocm[G_OCM_MAX_ENTRIES];

/** the list of oscore profiles */
#define G_O_PROFILE_MAX_ENTRIES 20
oc_oscore_profile_t g_o_profile[G_O_PROFILE_MAX_ENTRIES];

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
                               "urn:knx:xxx");
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
  oc_core_lf_populate_resource(
    resource_idx, device, "/a/sen", OC_IF_LL | OC_IF_BASELINE, APPLICATION_CBOR,
    OC_DISCOVERABLE, 0, 0, oc_core_a_sen_post_handler, 0, 0, "");
}

// ----------------------------------------------------------------------------

static int
find_empty_at_index()
{
  for (int i = 0; i < G_O_PROFILE_MAX_ENTRIES; i++) {
    if (oc_string_len(g_o_profile[i].id) == 0) {
      return i;
    }
  }

  return -1;
}

static int
find_index_from_at(oc_string_t *at)
{
  int len;
  int len_at = oc_string_len(*at);
  for (int i = 0; i < G_O_PROFILE_MAX_ENTRIES; i++) {
    len = oc_string_len(g_o_profile[i].id);
    if (len > 0 && len == len_at &&
        (strncmp(oc_string(*at), oc_string(g_o_profile[i].id), len) == 0)) {
      return i;
    }
  }

  return -1;
}

static int
find_index_from_at_string(const char *at, int len_at)
{
  int len;
  for (int i = 0; i < G_O_PROFILE_MAX_ENTRIES; i++) {
    len = oc_string_len(g_o_profile[i].id);
    PRINT("   find_index_from_at_string %s\n", oc_string(g_o_profile[i].id));
    if (len > 0 && len == len_at &&
        (strncmp(at, oc_string(g_o_profile[i].id), len) == 0)) {
      PRINT("     found %d\n", i);
      return i;
    }
  }

  return -1;
}

/* finds 0 ==> id */
static oc_string_t *
find_access_token_from_payload(oc_rep_t *object)
{
  oc_string_t *index = NULL;
  while (object != NULL) {
    switch (object->type) {
    case OC_REP_BYTE_STRING: {
      if (oc_string_len(object->name) == 0 && object->iname == 0) {
        index = &object->value.string;
        PRINT(" find_access_token_from_payload storing at %s \n",
              oc_string(*index));
        return index;
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    } /* switch */
    object = object->next;
  } /* while */
  PRINT("  find_access_token_from_payload ERR: storing \n");
  return index;
}

static void
oc_core_auth_at_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_auth_at_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  /* example entry: </auth/at/token-id>;ct=50 */
  for (i = 0; i < G_O_PROFILE_MAX_ENTRIES; i++) {

    // g_o_profile[i].contextId != NULL &&
    if (oc_string_len(g_o_profile[i].contextId) > 0) {
      // index  in use

      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("<auth/at/");

      length = oc_rep_add_line_to_buffer(oc_string(g_o_profile[i].contextId));
      response_length += length;
      // return cbor
      length = oc_rep_add_line_to_buffer(">;ct=60");
      response_length += length;
    }
  }

  if (response_length > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_auth_at_get_handler - end\n");
}

static void
oc_core_auth_at_post_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  oc_rep_t *rep = NULL;

  PRINT("oc_core_auth_at_post_handler - end\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  rep = request->request_payload;
  oc_string_t *at = find_access_token_from_payload(rep);
  if (at == NULL) {
    PRINT("   access token not found!\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  int index = find_index_from_at(at);
  if (index != -1) {
    PRINT("   already exist!\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  index = find_empty_at_index();
  if (index == -1) {
    PRINT("   no space left!\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;
  while (rep != NULL) {

    if (rep->type == OC_REP_INT) {
      // version (1)
      if (rep->iname == 1) {
        g_o_profile[index].version = rep->value.integer;
      }
    } else if (rep->type == OC_REP_STRING) {
      // hkdf (3) (as string)
      if (rep->iname == 3) {
        oc_free_string(&(g_o_profile[index].hkdf));
        oc_new_string(&g_o_profile[index].hkdf, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
      // alg (4)
      if (rep->iname == 4) {
        oc_free_string(&(g_o_profile[index].alg));
        oc_new_string(&g_o_profile[index].alg, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
    } else if (rep->type == OC_REP_BYTE_STRING) {
      // id == access token == 0
      if (rep->iname == 0) {
        oc_free_string(&(g_o_profile[index].id));
        oc_new_string(&g_o_profile[index].id, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
      // ms (2)
      if (rep->iname == 2) {
        oc_free_string(&(g_o_profile[index].ms));
        oc_new_string(&g_o_profile[index].ms, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
      // salt (5)
      if (rep->iname == 5) {
        oc_free_string(&(g_o_profile[index].salt));
        oc_new_string(&g_o_profile[index].salt, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
      // contextId (6)
      if (rep->iname == 6) {
        oc_free_string(&(g_o_profile[index].contextId));
        oc_new_string(&g_o_profile[index].contextId,
                      oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
      }
    } /*if */

    rep = rep->next;
  }

  PRINT("oc_core_auth_at_post_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_CHANGED);
}

void
oc_create_auth_at_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_auth_at_resource\n");
  // "/a/sen"
  oc_core_lf_populate_resource(resource_idx, device, "/auth/at", OC_IF_LIL,
                               APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                               oc_core_auth_at_get_handler, 0,
                               oc_core_auth_at_post_handler, 0, 1, "dpt.a[n]");
}

// ----------------------------------------------------------------------------

static void
oc_core_auth_at_x_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  // - find the id from the URL
  const char *value;
  int value_len = oc_uri_get_wildcard_value_as_string(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len, &value);
  // - delete the index.
  if (value_len <= 0) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    PRINT("index (at) not found\n");
    return;
  }

  PRINT(" id = %.*s\n", value_len, value);
  // get the index
  int index = find_index_from_at_string(value, value_len);
  // - delete the index.
  if (index < 0) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    PRINT("index in struct not found\n");
    return;
  }
  // return the data

  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_text_string(root, 0, oc_string(g_o_profile[index].id));

  // version 1
  oc_rep_i_set_int(root, 1, g_o_profile[index].version);
  // ia - 12
  // oc_rep_i_set_text_string(root, 11, oc_string(g_gpt[value].ia));
  // path- 112
  // oc_rep_i_set_text_string(root, 112, oc_string(g_gpt[value].path));
  // url- 10
  // oc_rep_i_set_text_string(root, 10, oc_string(g_gpt[value].url));
  // ga - 7
  // oc_rep_i_set_int_array(root, 7, g_gpt[value].ga, g_gpt[value].ga_len);

  oc_rep_end_root_object();

  PRINT("oc_core_auth_at_x_get_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

// probably no post handler needed
// partial update?
// how does that look like?
void
oc_core_auth_at_x_post_handler(oc_request_t *request,
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

  PRINT("oc_core_auth_at_x_post_handler\n");

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

static void
oc_core_auth_at_x_delete_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  const char *value;
  int value_len = -1;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  PRINT("oc_core_auth_at_x_delete_handler\n");

  // - find the id from the URL
  value_len = oc_uri_get_wildcard_value_as_string(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len, &value);
  // - delete the index.
  if (value_len <= 0) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    PRINT("index (at) not found\n");
    return;
  }

  PRINT("  id = %.*s\n", value_len, value);
  // get the index
  int index = find_index_from_at_string(value, value_len);
  // - delete the index.
  if (index < 0) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    PRINT("index in struct not found\n");
    return;
  }
  // actual delete of the context id so that this entry is seen as empty

  oc_free_string(&g_o_profile[index].id);
  oc_new_string(&g_o_profile[index].id, "", 0);

  PRINT("oc_core_auth_at_x_delete_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_auth_at_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_auth_at_x_resource\n");
  PRINT("oc_create_auth_at_x_resource\n");
  // "/a/sen"
  oc_core_lf_populate_resource(
    resource_idx, device, "/auth/at/*", OC_IF_LL | OC_IF_BASELINE,
    APPLICATION_CBOR, OC_DISCOVERABLE, oc_core_auth_at_x_get_handler, 0, 0,
    oc_core_auth_at_x_delete_handler, 1, "dpt.a[n]");
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

  for (i = (int)OC_KNX_A_SEN; i < (int)OC_KNX_AUTH; i++) {
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
  oc_core_lf_populate_resource(
    resource_idx, device, "/auth", OC_IF_LIL, APPLICATION_LINK_FORMAT,
    OC_DISCOVERABLE, oc_core_knx_auth_get_handler, 0, 0, 0, 1, "urn:knx:xxx");
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
  oc_create_auth_at_x_resource(OC_KNX_AUTH_AT_X, device_index);
  oc_create_knx_auth_resource(OC_KNX_AUTH, device_index);
}
