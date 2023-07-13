/*
 // Copyright (c) 2021-2023 Cascoda Ltd
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
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "security/oc_oscore_context.h"
#include "oc_knx.h"
#include "oc_knx_helpers.h"

uint64_t g_oscore_replaywindow = 0;
uint64_t g_oscore_osndelay = 0;

/** the list of connections */
//#define G_OCM_MAX_ENTRIES 20
// oc_oscore_cm_t g_ocm[G_OCM_MAX_ENTRIES];

/** the list of oscore profiles */
#define AT_STORE "at_store"
#define G_AT_MAX_ENTRIES 20
oc_auth_at_t g_at_entries[G_AT_MAX_ENTRIES];

// ----------------------------------------------------------------------------

static void oc_at_dump_entry(size_t device_index, int entry);

// ----------------------------------------------------------------------------

oc_at_profile_t
oc_string_to_at_profile(oc_string_t str)
{
  if (strcmp(oc_string(str), "coap_oscore") == 0) {
    return OC_PROFILE_COAP_OSCORE;
  }
  if (strcmp(oc_string(str), "coap_dtls") == 0) {
    return OC_PROFILE_COAP_DTLS;
  }
  return OC_PROFILE_UNKNOWN;
}

char *
oc_at_profile_to_string(oc_at_profile_t at_profile)
{
  if (at_profile == OC_PROFILE_COAP_OSCORE) {
    return "coap_oscore";
  }
  if (at_profile == OC_PROFILE_COAP_DTLS) {
    return "coap_dtls";
  }
  return "";
}

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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // cbor_encode_uint(&g_encoder, g_oscore_osndelay);
  oc_rep_begin_root_object();
  oc_rep_i_set_uint(root, 1, g_oscore_osndelay);
  oc_rep_end_root_object();

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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_knx_p_oscore_osndelay_put_handler type: %d value %d\n",
              (int)rep->type, (int)rep->value.integer);
        g_oscore_osndelay = rep->value.integer;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_knx_p_oscore_osndelay_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_p_oscore_osndelay_resource\n");
  //
  oc_core_populate_resource(
    resource_idx, device, "/p/oscore/osndelay", OC_IF_D, APPLICATION_CBOR,
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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // cbor_encode_uint(&g_encoder, g_oscore_replaywindow);
  oc_rep_begin_root_object();
  oc_rep_i_set_uint(root, 1, g_oscore_replaywindow);
  oc_rep_end_root_object();

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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_knx_p_oscore_replwdo_put_handler type: %d value %d\n",
              rep->type, (int)rep->value.integer);
        g_oscore_replaywindow = rep->value.integer;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_knx_p_oscore_replwdo_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_knx_p_oscore_replwdo_resource\n");
  //
  oc_core_populate_resource(
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
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  for (i = (int)OC_KNX_P_OSCORE_REPLWDO; i <= (int)OC_KNX_P_OSCORE_OSNDELAY;
       i++) {
    oc_resource_t *resource = oc_core_get_resource_by_index(i, device_index);
    if (oc_filter_resource(resource, request, device_index, &response_length,
                           matches, 1)) {
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
  // TODO: what is resource type?
  // none for now
  oc_core_populate_resource(resource_idx, device, "/f/oscore", OC_IF_LI,
                            APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                            oc_core_knx_f_oscore_get_handler, 0, 0, 0, 0);
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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  bool changed = false;
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;
  while (rep != NULL) {
    PRINT("oc_core_a_sen_post_handler: key: (check) %s \n",
          oc_string_checked(rep->name));
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
    PRINT("  oc_core_a_sen_post_handler cmd %d\n", cmd);
    // renew the credentials.
    // note: this is optional for now

    // oc_send_cbor_response(request, OC_STATUS_CHANGED);
    oc_send_cbor_response_no_payload_size(request, OC_STATUS_CHANGED);
    return;
  }
  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_a_sen_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_a_sen_resource\n");
  // "/a/sen"
  oc_core_populate_resource(resource_idx, device, "/a/sen", OC_IF_SEC,
                            APPLICATION_CBOR, OC_DISCOVERABLE, 0, 0,
                            oc_core_a_sen_post_handler, 0, 0, "");
}

// ----------------------------------------------------------------------------

static int
find_empty_at_index()
{
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    if (oc_string_len(g_at_entries[i].id) == 0) {
      return i;
    }
  }
  return -1;
}

static int
find_index_from_at(oc_string_t *at)
{
  size_t len;
  size_t len_at = oc_string_len(*at);
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    len = oc_string_len(g_at_entries[i].id);
    if (len > 0 && len == len_at &&
        (strncmp(oc_string(*at), oc_string(g_at_entries[i].id), len) == 0)) {
      return i;
    }
  }
  return -1;
}

static int
find_index_from_at_string(const char *at, int len_at)
{
  int len;
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    len = (int)oc_string_len(g_at_entries[i].id);
    if (len > 0 && len == len_at &&
        (strncmp(at, oc_string(g_at_entries[i].id), len) == 0)) {
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
        PRINT(" find_access_token_from_payload: %s \n",
              oc_string_checked(*index));
        return index;
      }
    } break;
    case OC_REP_STRING: {
      if (oc_string_len(object->name) == 0 && object->iname == 0) {
        index = &object->value.string;
        PRINT(" find_access_token_from_payload: %s \n",
              oc_string_checked(*index));
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
  PRINT("  find_access_token_from_payload Error \n");
  return index;
}

int
oc_core_get_at_table_size()
{
  return G_AT_MAX_ENTRIES;
}

int
oc_core_find_nr_used_in_auth_at_table()
{
  int counter = 0;
  for (int i = 0; i < oc_core_get_at_table_size(); i++) {
    if (oc_string_len(g_at_entries[i].id) == 0) {
      counter++;
    }
  }
  return counter;
}

// ----------------------------------------------------------------------------

static void
oc_core_auth_at_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  bool ps_exists;
  bool total_exists;
  PRINT("oc_core_auth_at_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // handle query parameters: l=ps l=total
  if (check_if_query_l_exist(request, &ps_exists, &total_exists)) {
    // example : < / fp / r / ? l = total>; total = 22; ps = 5

    length = oc_frame_query_l("/auth/at", ps_exists, total_exists);
    response_length += length;
    if (ps_exists) {
      length = oc_rep_add_line_to_buffer(";ps=");
      response_length += length;
      length = oc_frame_integer(oc_core_get_at_table_size());
      response_length += length;
    }
    if (total_exists) {
      length = oc_rep_add_line_to_buffer(";total=");
      response_length += length;
      length = oc_frame_integer(oc_core_find_nr_used_in_auth_at_table());
      response_length += length;
    }

    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }

  /* example entry: </auth/at/token-id>;ct=50 */
  for (i = 0; i < G_AT_MAX_ENTRIES; i++) {
    if (oc_string_len(g_at_entries[i].id) > 0) {
      // index  in use
      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }
      length = oc_rep_add_line_to_buffer("<auth/at/");
      response_length += length;
      length = oc_rep_add_line_to_buffer(oc_string(g_at_entries[i].id));
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
  oc_rep_t *object = NULL;
  oc_rep_t *subobject = NULL;
  oc_rep_t *oscobject = NULL;
  oc_status_t return_status = OC_STATUS_BAD_REQUEST;
  bool scope_updated = false;
  bool other_updated = false;
  int index = -1;
  PRINT("oc_core_auth_at_post_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  size_t device_index = request->resource->device;

  /* debugging info */
  oc_print_rep_as_json(request->request_payload, true);

  rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_OBJECT) {
      object = rep->value.object;
      oc_string_t *at = find_access_token_from_payload(object);
      if (at == NULL) {
        PRINT("  access token not found!\n");
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      index = find_index_from_at(at);
      if (index != -1) {
        PRINT("   entry already exist! \n");
        return_status = OC_STATUS_CHANGED;
      } else {
        index = find_empty_at_index();
        return_status = OC_STATUS_CREATED;
        if (index == -1) {
          PRINT("  no space left!\n");
          oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
          return;
        }
      }
      PRINT("  storage index: %d (%s)\n", index, oc_string_checked(*at));
      oc_free_string(&(g_at_entries[index].id));
      oc_new_string(&g_at_entries[index].id, oc_string(*at),
                    oc_string_len(*at));

      object = rep->value.object;
      while (object != NULL) {
        if (object->type == OC_REP_STRING_ARRAY) {
          // scope
          if (object->iname == 9) {
            // scope: array of interfaces as string
            oc_string_array_t str_array;
            size_t str_array_size = 0;
            oc_interface_mask_t interfaces = OC_IF_NONE;
            oc_rep_i_get_string_array(object, 9, &str_array, &str_array_size);
            for (size_t i = 0; i < str_array_size; i++) {
              char *if_str = oc_string_array_get_item(str_array, i);
              oc_interface_mask_t if_mask =
                oc_ri_get_interface_mask(if_str, strlen(if_str));
              interfaces = interfaces + if_mask;
            }
            g_at_entries[index].scope = interfaces;
            scope_updated = true;
          }
        } else if (object->type == OC_REP_INT_ARRAY) {
          // scope
          if (object->iname == 9) {
            g_at_entries[index].scope = OC_IF_NONE;
            int64_t *array = 0;
            size_t array_size = 0;
            // not making a deep copy
            oc_rep_i_get_int_array(object, 9, &array, &array_size);
            if (array_size > 0) {
              // make the deep copy
              if ((g_at_entries[index].ga_len > 0) &&
                  (&g_at_entries[index].ga != NULL)) {
                int64_t *cur_arr = g_at_entries[index].ga;
                if (cur_arr) {
                  free(cur_arr);
                }
                g_at_entries[index].ga = NULL;
              }
              g_at_entries[index].ga_len = (int)array_size;
              // always set the group address scope, if there is 1 or more ga
              // entries
              g_at_entries[index].scope = OC_IF_G;
              int64_t *new_array =
                (int64_t *)malloc(array_size * sizeof(uint64_t));
              if (new_array) {
                for (size_t i = 0; i < array_size; i++) {
                  new_array[i] = array[i];
                }
                g_at_entries[index].ga = new_array;
              } else {
                OC_ERR("out of memory");
              }
            }
          }
        } else if (object->type == OC_REP_STRING) {
          if (object->iname == 2) {
            // sub
            oc_free_string(&(g_at_entries[index].sub));
            oc_new_string(&g_at_entries[index].sub,
                          oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (object->iname == 3) {
            // aud
            oc_free_string(&(g_at_entries[index].aud));
            oc_new_string(&g_at_entries[index].aud,
                          oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } else if (object->type == OC_REP_INT) {
          if (object->iname == 38) {
            // profile (38 ("coap_dtls" ==1 or "coap_oscore" == 2))
            PRINT("   profile %d\n", (int)object->value.integer);
            g_at_entries[index].profile = (int)object->value.integer;
          }
        } else if (object->type == OC_REP_OBJECT) {
          // level of cnf or sub.
          subobject = object->value.object;
          int subobject_nr = object->iname;
          PRINT("  subobject_nr %d\n", subobject_nr);
          while (subobject) {
            if (subobject->type == OC_REP_STRING) {
              if (subobject->iname == 3 && subobject_nr == 8) {
                // cnf::kid (8::3)
                oc_free_string(&(g_at_entries[index].kid));
                oc_new_string(&g_at_entries[index].kid,
                              oc_string(subobject->value.string),
                              oc_string_len(subobject->value.string));
              }
            } else if (subobject->type == OC_REP_OBJECT) {
              oscobject = subobject->value.object;
              int oscobject_nr = subobject->iname;
              while (oscobject) {
                if (oscobject->type == OC_REP_INT) {
                  if (oscobject->iname == 4 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // not storing it: we only support value 10 at the moment.
                    // g_at_entries[index].osc_alg = (int)object->value.integer;
                    if ((int)object->value.integer != 10) {
                      OC_ERR("algorithm is not 10 : %d",
                             (int)object->value.integer);
                      return_status = OC_STATUS_BAD_REQUEST;
                    }
                    other_updated = true;
                  }
                }
                if (oscobject->type == OC_REP_BYTE_STRING) {
                  if (oscobject->iname == 2 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::ms
                    oc_free_string(&(g_at_entries[index].osc_ms));
                    oc_new_byte_string(&g_at_entries[index].osc_ms,
                                       oc_string(oscobject->value.string),
                                       oc_string_len(oscobject->value.string));
                    other_updated = true;
                  }
                  if (oscobject->iname == 6 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::contextId
                    oc_free_string(&(g_at_entries[index].osc_contextid));
                    oc_new_byte_string(&g_at_entries[index].osc_contextid,
                                       oc_string(oscobject->value.string),
                                       oc_string_len(oscobject->value.string));
                    other_updated = true;
                  }
                  if (oscobject->iname == 7 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::rid
                    oc_free_string(&(g_at_entries[index].osc_rid));
                    oc_new_byte_string(&g_at_entries[index].osc_rid,
                                       oc_string(oscobject->value.string),
                                       oc_string_len(oscobject->value.string));
                    other_updated = true;
                  }
                  if (oscobject->iname == 0 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::id
                    oc_free_string(&(g_at_entries[index].osc_id));
                    oc_new_byte_string(&g_at_entries[index].osc_id,
                                       oc_string(oscobject->value.string),
                                       oc_string_len(oscobject->value.string));
                    other_updated = true;
                  }
                } /* type */

                oscobject = oscobject->next;
              }
            }
            subobject = subobject->next;
          }
        }
        object = object->next;
      } // while (inner object)
    }   // if type == object
    // show the entry on screen
    //
    // temp backward compatibility fix: if recipient id is not there then use
    // SID for recipient ID
    if (oc_string_len(g_at_entries[index].osc_rid) == 0) {
      oc_free_string(&(g_at_entries[index].osc_rid));
      oc_new_byte_string(&g_at_entries[index].osc_rid,
                         oc_string(g_at_entries[index].osc_id),
                         oc_byte_string_len(g_at_entries[index].osc_id));
    }
    // temp backward compatibility fix: if context id is not there then use
    // SID for context ID
    /*
    if (oc_string_len(g_at_entries[index].osc_contextid) == 0) {
      oc_free_string(&(g_at_entries[index].osc_contextid));
      oc_new_byte_string(&g_at_entries[index].osc_contextid,
                         oc_string(g_at_entries[index].osc_id),
                         oc_byte_string_len(g_at_entries[index].osc_id));
    }
    */

    oc_print_auth_at_entry(device_index, index);

    // dump the entry to persistent storage
    oc_at_dump_entry(device_index, index);
    rep = rep->next;
  } // while (rep)

  PRINT("oc_core_auth_at_post_handler - activating oscore context\n");
  // add the oscore contexts by reinitializing all used oscore keys.
  // do not update the oscore when:
  // - update of the scope contents only
  if ((return_status == OC_STATUS_CHANGED) && (other_updated == false) &&
      (scope_updated == true)) {
    OC_WRN("update scope only");
  } else {
    // update the oscore context
    oc_init_oscore_from_storage(device_index, false);
  }
  PRINT("oc_core_auth_at_post_handler - end\n");
  oc_send_cbor_response_no_payload_size(request, return_status);
}

static void
oc_core_auth_at_delete_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_auth_at_delete_handler\n");

  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  oc_delete_at_table(device_index);

  PRINT("oc_core_auth_at_delete_handler - end\n");
  oc_send_cbor_response_no_payload_size(request, OC_STATUS_DELETED);
}

void
oc_create_auth_at_resource(int resource_idx, size_t device)
{
  oc_core_populate_resource(
    resource_idx, device, "/auth/at", OC_IF_LI | OC_IF_B | OC_IF_SEC,
    APPLICATION_LINK_FORMAT, OC_DISCOVERABLE, oc_core_auth_at_get_handler, 0,
    oc_core_auth_at_post_handler, oc_core_auth_at_delete_handler, 1,
    "urn:knx:fb.at");
}

// ----------------------------------------------------------------------------

// note that this list all auth/at/x resources
// it does not list the auth/at resource !!
void
oc_create_auth_resource(int resource_idx, size_t device)
{
  oc_core_populate_resource(resource_idx, device, "/auth", OC_IF_B | OC_IF_SEC,
                            APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                            oc_core_auth_at_get_handler, 0, 0, 0, 0);
}

// ----------------------------------------------------------------------------

static void
oc_core_auth_at_x_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  PRINT("oc_core_auth_at_x_get_handler\n");

  // - find the id from the URL
  const char *value;
  int value_len = oc_uri_get_wildcard_value_as_string(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len, &value);
  // - delete the index.
  if (value_len <= 0) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    PRINT("  index (at) not found\n");
    return;
  }
  PRINT("  id = %.*s\n", value_len, value);
  // get the index
  int index = find_index_from_at_string(value, value_len);
  // - delete the index.
  if (index < 0) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    PRINT("  index in structure not found\n");
    return;
  }
  oc_print_auth_at_entry(0, index);

  // return the data
  oc_rep_begin_root_object();
  // profile : 38
  oc_rep_i_set_int(root, 38, g_at_entries[index].profile);
  // id : 0
  oc_rep_i_set_text_string(root, 0, oc_string(g_at_entries[index].id));
  // audience : 3
  if (oc_string_len(g_at_entries[index].aud) > 0) {
    oc_rep_i_set_text_string(root, 3, oc_string(g_at_entries[index].aud));
  }
  // the scope as list of cflags or group object table entries
  int nr_entries = oc_total_interface_in_mask(g_at_entries[index].scope);
  if (nr_entries > 0) {
    // interface list
    oc_string_array_t cflags_entries;
    oc_new_string_array(&cflags_entries, (size_t)nr_entries);
    int framed = oc_get_interface_in_mask_in_string_array(
      g_at_entries[index].scope, nr_entries, cflags_entries);
    PRINT("  entries in cflags %d framed: %d \n", nr_entries, framed);
    oc_rep_i_set_string_array(root, 9, cflags_entries);
    oc_free_string_array(&cflags_entries);
  } else {
    // group object list
    // taking input of int64 array
    oc_rep_i_set_int_array(root, 9, g_at_entries[index].ga,
                           g_at_entries[index].ga_len);
  }
  if (g_at_entries[index].profile == OC_PROFILE_COAP_DTLS) {
    if (oc_string_len(g_at_entries[index].sub) > 0) {
      PRINT("    sub    : %s\n", oc_string_checked(g_at_entries[index].sub));
    }
    if (oc_string_len(g_at_entries[index].kid) > 0) {
      PRINT("    kid    : %s\n", oc_string_checked(g_at_entries[index].kid));
    }
  }
  if (g_at_entries[index].profile == OC_PROFILE_COAP_OSCORE) {
    // create cnf 98)
    oc_rep_i_set_key(&root_map, 8);
    CborEncoder cnf_map;
    cbor_encoder_create_map(&root_map, &cnf_map, CborIndefiniteLength);
    // create osc (4)
    oc_rep_i_set_key(&cnf_map, 4);
    CborEncoder osc_map;
    cbor_encoder_create_map(&cnf_map, &osc_map, CborIndefiniteLength);
    if (oc_string_len(g_at_entries[index].osc_ms) > 0) {
      oc_rep_i_set_byte_string(
        osc, 2, oc_string(g_at_entries[index].osc_ms),
        oc_byte_string_len(g_at_entries[index].osc_ms)); // root::cnf::osc::ms
    }
    // if (oc_string_len(g_at_entries[index].osc_alg) > 0) {
    //   oc_rep_i_set_text_string(
    //     osc, 4, oc_string(g_at_entries[index].osc_alg)); //
    //     root::cnf::osc::alg
    // }
    if (oc_string_len(g_at_entries[index].osc_contextid) > 0) {
      oc_rep_i_set_byte_string(
        osc, 6, oc_string(g_at_entries[index].osc_contextid),
        oc_byte_string_len(
          g_at_entries[index].osc_contextid)); // root::cnf::osc::contextid
    }
    if (oc_string_len(g_at_entries[index].osc_rid) > 0) {
      oc_rep_i_set_byte_string(
        osc, 7, oc_string(g_at_entries[index].osc_rid),
        oc_byte_string_len(
          g_at_entries[index].osc_rid)); // root::cnf::osc::osc_rid
    }
    if (oc_string_len(g_at_entries[index].osc_id) > 0) {
      oc_rep_i_set_byte_string(
        osc, 0, oc_string(g_at_entries[index].osc_id),
        oc_byte_string_len(
          g_at_entries[index].osc_id)); // root::cnf::osc::osc_id
    }
    cbor_encoder_close_container_checked(&cnf_map, &osc_map);
    cbor_encoder_close_container_checked(&root_map, &cnf_map);
  }

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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  PRINT("oc_core_auth_at_x_post_handler\n");
  bool changed = false;
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;
  while (rep != NULL) {
    PRINT("key: (check) %s \n", oc_string_checked(rep->name));
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
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  PRINT("oc_core_auth_at_x_delete_handler\n");
  size_t device_index = request->resource->device;

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
    PRINT("oc_core_auth_at_x_delete_handler: index in structure not found\n");
    return;
  }
  // actual delete of the context id so that this entry is seen as empty
  oc_at_delete_entry(device_index, index);
  // do the persistent storage
  oc_at_dump_entry(device_index, index);
  // remove the key by reinitializing all used oscore keys.
  oc_init_oscore_from_storage(device_index, false);
  PRINT("oc_core_auth_at_x_delete_handler - done\n");
  oc_send_cbor_response_no_payload_size(request, OC_STATUS_DELETED);
}

void
oc_create_auth_at_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_auth_at_x_resource\n");

  oc_core_populate_resource(resource_idx, device, "/auth/at/*", OC_IF_SEC,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_auth_at_x_get_handler, 0, 0,
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
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  size_t device_index = request->resource->device;
  for (i = (int)OC_KNX_A_SEN; i < (int)OC_KNX_AUTH; i++) {
    oc_resource_t *resource = oc_core_get_resource_by_index(i, device_index);
    if (oc_filter_resource(resource, request, device_index, &response_length,
                           matches, 1)) {
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
  oc_core_populate_resource(resource_idx, device, "/auth", OC_IF_LI,
                            APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                            oc_core_knx_auth_get_handler, 0, 0, 0, 0);
}

void
oc_print_auth_at_entry(size_t device_index, int index)
{
  (void)device_index;
  if (index > -1) {
    if (oc_string_len(g_at_entries[index].id) > 0) {

      PRINT("  at index: %d\n", index);
      PRINT("    id (0)        : '%s'\n",
            oc_string_checked(g_at_entries[index].id));
      PRINT("    scope (9)     : %d\n", g_at_entries[index].scope);
      PRINT("    profile (38)  : %d (%s)\n", g_at_entries[index].profile,
            oc_at_profile_to_string(g_at_entries[index].profile));
      if (g_at_entries[index].profile == OC_PROFILE_COAP_DTLS) {
        if (oc_string_len(g_at_entries[index].sub) > 0) {
          PRINT("    sub           : %s\n",
                oc_string_checked(g_at_entries[index].sub));
        }
        if (oc_string_len(g_at_entries[index].kid) > 0) {
          PRINT("    kid           : %s\n",
                oc_string_checked(g_at_entries[index].kid));
        }
      }
      if (g_at_entries[index].profile == OC_PROFILE_COAP_OSCORE) {
        if (oc_string_len(g_at_entries[index].osc_ms) > 0) {
          PRINT("    osc:ms    (h) : (%d) ",
                (int)oc_byte_string_len(g_at_entries[index].osc_ms));
          oc_string_println_hex(g_at_entries[index].osc_ms);
        }
        if (oc_string_len(g_at_entries[index].osc_contextid) > 0) {
          PRINT("    osc:ctx_id (h): (%d) ",
                (int)oc_byte_string_len(g_at_entries[index].osc_contextid));
          oc_string_println_hex(g_at_entries[index].osc_contextid);
        }
        if (oc_string_len(g_at_entries[index].osc_id) > 0) {
          PRINT("    osc:id    (h) : (%d) ",
                (int)oc_byte_string_len(g_at_entries[index].osc_id));
          oc_string_println_hex(g_at_entries[index].osc_id);
        }
        if (oc_string_len(g_at_entries[index].osc_rid) > 0) {
          PRINT("    osc:rid   (h) : (%d) ",
                (int)oc_byte_string_len(g_at_entries[index].osc_rid));
          oc_string_println_hex(g_at_entries[index].osc_rid);
        }
        if (g_at_entries[index].ga_len > 0) {
          PRINT("    osc:ga        : [");
          for (int i = 0; i < g_at_entries[index].ga_len; i++) {
            PRINT(" %" PRIu64 "", (uint64_t)g_at_entries[index].ga[i]);
          }
          PRINT(" ]\n");
        }
      }
    }
  }
}

oc_interface_mask_t
oc_at_get_interface_mask(size_t device_index, int index)
{
  (void)device_index;
  if (index < 0) {
    return OC_IF_NONE;
  }
  if (index > oc_core_get_at_table_size() - 1) {
    return OC_IF_NONE;
  }
  return g_at_entries[index].scope;
}

int
oc_at_delete_entry(size_t device_index, int index)
{
  (void)device_index;
  if (index < 0) {
    return -1;
  }
  if (index > oc_core_get_at_table_size() - 1) {
    return -1;
  }

  // generic
  oc_free_string(&g_at_entries[index].id);
  oc_new_string(&g_at_entries[index].id, "", 0);
  g_at_entries[index].scope = OC_IF_NONE;
  g_at_entries[index].profile = OC_PROFILE_UNKNOWN;
  // oscore object
  oc_free_string(&g_at_entries[index].osc_ms);
  oc_new_byte_string(&g_at_entries[index].osc_ms, "", 0);
  oc_free_string(&g_at_entries[index].osc_contextid);
  oc_new_byte_string(&g_at_entries[index].osc_contextid, "", 0);
  oc_free_string(&g_at_entries[index].osc_rid);
  oc_new_byte_string(&g_at_entries[index].osc_rid, "", 0);
  oc_free_string(&g_at_entries[index].osc_id);
  oc_new_byte_string(&g_at_entries[index].osc_id, "", 0);
  // dtls object
  oc_free_string(&g_at_entries[index].sub);
  oc_new_string(&g_at_entries[index].sub, "", 0);
  oc_free_string(&g_at_entries[index].kid);
  oc_new_string(&g_at_entries[index].kid, "", 0);

  char filename[20];
  snprintf(filename, 20, "%s_%d", AT_STORE, index);
  oc_storage_erase(filename);

  return 0;
}

// Note: storage of the fields is done via the cbor keys in the hierarchy
// so tag : 842  8.4.2 ==> "cnf":"osc":"ms"
static void
oc_at_dump_entry(size_t device_index, int entry)
{
  (void)device_index;
#ifndef OC_USE_STORAGE
  (void)entry;
  PRINT("no auth/at storage");
#else
  char filename[20];

  snprintf(filename, 20, "%s_%d", AT_STORE, entry);
  uint8_t *buf = malloc(OC_MAX_APP_DATA_SIZE);
  if (!buf)
    return;

  oc_rep_new(buf, OC_MAX_APP_DATA_SIZE);
  // write the data
  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_text_string(root, 0, oc_string(g_at_entries[entry].id));
  // interface 9 /// this is different than the response on the wire
  oc_rep_i_set_int(root, 9, g_at_entries[entry].scope);
  oc_rep_i_set_int(root, 38, g_at_entries[entry].profile);
  oc_rep_i_set_byte_string(root, 842, oc_string(g_at_entries[entry].osc_ms),
                           oc_byte_string_len(g_at_entries[entry].osc_ms));
  oc_rep_i_set_byte_string(
    root, 846, oc_string(g_at_entries[entry].osc_contextid),
    oc_byte_string_len(g_at_entries[entry].osc_contextid));
  oc_rep_i_set_byte_string(root, 847, oc_string(g_at_entries[entry].osc_rid),
                           oc_byte_string_len(g_at_entries[entry].osc_rid));
  oc_rep_i_set_byte_string(root, 840, oc_string(g_at_entries[entry].osc_id),
                           oc_byte_string_len(g_at_entries[entry].osc_id));
  oc_rep_i_set_text_string(root, 82, oc_string(g_at_entries[entry].sub));
  oc_rep_i_set_text_string(root, 81, oc_string(g_at_entries[entry].kid));

  oc_rep_i_set_int_array(root, 777, g_at_entries[entry].ga,
                         g_at_entries[entry].ga_len);

  oc_rep_end_root_object();

  int size = oc_rep_get_encoded_payload_size();
  if (size > 0) {
    OC_DBG("oc_at_dump_entry: dumped current state [%s] [%d]: "
           "size %d",
           filename, entry, size);
    long written_size = oc_storage_write(filename, buf, size);
    if (written_size != (long)size) {
      PRINT("oc_at_dump_entry: [%s] written %d != %d (towrite)\n", filename,
            (int)written_size, size);
    }
  }
  free(buf);
#endif /* OC_USE_STORAGE */
}

static void
oc_at_load_entry(int entry)
{
  int ret;
  char filename[20];
  oc_rep_t *rep, *head;
  snprintf(filename, 20, "%s_%d", AT_STORE, entry);
  uint8_t *buf = malloc(OC_MAX_APP_DATA_SIZE);
  if (!buf)
    return;

  ret = oc_storage_read(filename, buf, OC_MAX_APP_DATA_SIZE);
  if (ret > 0) {
    struct oc_memb rep_objects = { sizeof(oc_rep_t), 0, 0, 0, 0 };
    oc_rep_set_pool(&rep_objects);
    int err = oc_parse_rep(buf, ret, &rep);
    head = rep;
    if (err == 0) {
      while (rep != NULL) {
        switch (rep->type) {

        case OC_REP_INT:
          if (rep->iname == 9) {
            g_at_entries[entry].scope = (int)rep->value.integer;
          }
          if (rep->iname == 38) {
            g_at_entries[entry].profile = (int)rep->value.integer;
          }
          break;
        case OC_REP_STRING:
          if (rep->iname == 0) {
            oc_free_string(&g_at_entries[entry].id);
            oc_new_string(&g_at_entries[entry].id, oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 82) {
            oc_free_string(&g_at_entries[entry].sub);
            oc_new_string(&g_at_entries[entry].sub,
                          oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 81) {
            oc_free_string(&g_at_entries[entry].kid);
            oc_new_string(&g_at_entries[entry].kid,
                          oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          break;
        case OC_REP_BYTE_STRING:
          // note: reading back the strings should be done with oc_string_len
          // not with oc_byte_string_len
          if (rep->iname == 840) {
            oc_free_string(&g_at_entries[entry].osc_id);
            oc_new_byte_string(&g_at_entries[entry].osc_id,
                               oc_string(rep->value.string),
                               oc_string_len(rep->value.string));
          }
          if (rep->iname == 842) {
            oc_free_string(&g_at_entries[entry].osc_ms);
            oc_new_byte_string(&g_at_entries[entry].osc_ms,
                               oc_string(rep->value.string),
                               oc_string_len(rep->value.string));
          }
          if (rep->iname == 846) {
            oc_free_string(&g_at_entries[entry].osc_contextid);
            oc_new_byte_string(&g_at_entries[entry].osc_contextid,
                               oc_string(rep->value.string),
                               oc_string_len(rep->value.string));
          }

          if (rep->iname == 847) {
            oc_free_string(&g_at_entries[entry].osc_rid);
            oc_new_byte_string(&g_at_entries[entry].osc_rid,
                               oc_string(rep->value.string),
                               oc_string_len(rep->value.string));
          }
          break;
        case OC_REP_INT_ARRAY:
          if (rep->iname == 777) {
            int64_t *array = oc_int_array(rep->value.array);
            int array_size = (int)oc_int_array_size(rep->value.array);

            if (array_size > 0) {
              // make the deep copy
              if (g_at_entries[entry].ga_len > 0) {
                uint64_t *cur_arr = g_at_entries[entry].ga;
                if (cur_arr) {
                  free(cur_arr);
                }
                g_at_entries[entry].ga = NULL;
                // free(&g_at_entries[entry].ga);
              }
              g_at_entries[entry].ga_len = array_size;
              int64_t *new_array =
                (int64_t *)malloc(array_size * sizeof(uint64_t));

              if (new_array) {
                for (int i = 0; i < array_size; i++) {
                  new_array[i] = array[i];
                }
                g_at_entries[entry].ga = new_array;
              } else {
                OC_ERR("out of memory");
              }
            }
          }
          break;
        default:
          break;
        }
        rep = rep->next;
      }
    }
    oc_free_rep(head);
  }
  free(buf);
}

int
oc_core_set_at_table(size_t device_index, int index, oc_auth_at_t entry,
                     bool store)
{
  (void)device_index;
  if (index < G_AT_MAX_ENTRIES) {

    oc_free_string(&g_at_entries[index].id);
    oc_new_string(&g_at_entries[index].id, oc_string(entry.id),
                  oc_string_len(entry.id));
    g_at_entries[index].scope = entry.scope;
    g_at_entries[index].profile = entry.profile;
    oc_free_string(&g_at_entries[index].sub);
    oc_new_string(&g_at_entries[index].sub, oc_string(entry.sub),
                  oc_string_len(entry.sub));
    oc_free_string(&g_at_entries[index].kid);
    oc_new_string(&g_at_entries[index].kid, oc_string(entry.kid),
                  oc_string_len(entry.kid));
    oc_free_string(&g_at_entries[index].osc_ms);
    oc_new_byte_string(&g_at_entries[index].osc_ms, oc_string(entry.osc_ms),
                       oc_byte_string_len(entry.osc_ms));
    // oc_free_string(&g_at_entries[index].osc_alg);
    // oc_new_string(&g_at_entries[index].osc_alg, oc_string(entry.osc_alg),
    //               oc_string_len(entry.osc_alg));
    oc_free_string(&g_at_entries[index].osc_contextid);
    oc_new_byte_string(&g_at_entries[index].osc_contextid,
                       oc_string(entry.osc_contextid),
                       oc_byte_string_len(entry.osc_contextid));
    oc_free_string(&g_at_entries[index].osc_rid);
    oc_new_byte_string(&g_at_entries[index].osc_rid, oc_string(entry.osc_rid),
                       oc_byte_string_len(entry.osc_rid));
    oc_free_string(&g_at_entries[index].osc_id);
    oc_new_byte_string(&g_at_entries[index].osc_id, oc_string(entry.osc_id),
                       oc_byte_string_len(entry.osc_id));
    // clean up existing entry
    if (g_at_entries[index].ga_len > 0) {
      int64_t *cur_arr = g_at_entries[index].ga;
      if (cur_arr) {
        free(cur_arr);
      }
    }
    // copy initial data
    g_at_entries[index].ga_len = entry.ga_len;
    g_at_entries[index].ga = NULL;
    // copy the array
    if (g_at_entries[index].ga_len > 0) {
      int array_size = g_at_entries[index].ga_len;
      g_at_entries[index].ga_len = (int)array_size;
      int64_t *new_array = (int64_t *)malloc(array_size * sizeof(uint64_t));
      if (new_array) {
        for (size_t i = 0; i < array_size; i++) {
          new_array[i] = entry.ga[i];
        }
        g_at_entries[index].ga = new_array;
      } else {
        OC_ERR("out of memory");
        return -1;
      }
    }

    if (store) {
      oc_at_dump_entry(device_index, index);
    }
  }
  // activate the credentials
  OC_DBG_OSCORE("oc_core_set_at_table: activating OSCORE credentials");
  oc_init_oscore_from_storage(device_index, false);

  return 0;
}

int
oc_core_find_at_entry_with_id(size_t device_index, char *id)
{
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    if ((oc_string_len(g_at_entries[i].id) > 0) &&
        (strncmp(oc_string(g_at_entries[i].id), id, strlen(id)) == 0)) {
      return i;
    }
  }
  return -1;
}

int
oc_core_find_at_entry_empty_slot(size_t device_index)
{
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    if (oc_string_len(g_at_entries[i].id) == 0) {
      return i;
    }
  }
  return -1;
}

void
oc_load_at_table(size_t device_index)
{
  PRINT("Loading AT Table from Persistent storage\n");
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    oc_at_load_entry(i);
    if (oc_string_len(g_at_entries[i].id) > 0) {
      oc_print_auth_at_entry(device_index, i);
    }
  }
  // create the oscore contexts
  oc_init_oscore_from_storage(device_index, true);
}

void
oc_delete_at_table(size_t device_index)
{
  PRINT("Deleting AT Object Table from Persistent storage\n");
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    oc_at_delete_entry(device_index, i);
    oc_print_auth_at_entry(device_index, i);
  }
#ifdef OC_OSCORE
  oc_oscore_free_all_contexts();
#endif
}

void
oc_reset_at_table(size_t device_index, int erase_code)
{
  PRINT("Reset AT Object Table: %d\n", erase_code);

  if (erase_code == 2) {
    oc_delete_at_table(device_index);
  } else if (erase_code == 7) {
    // reset the entries that are not "if.sec"
    oc_interface_mask_t scope = OC_IF_NONE;
    for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
      scope = oc_at_get_interface_mask(device_index, i);
      // OC_IF_SEC flag not set
      if ((scope & OC_IF_SEC) == 0) {
        // reset the entries that are not "if.sec"
        oc_at_delete_entry(device_index, i);
        oc_print_auth_at_entry(device_index, i);
      }
    }
#ifdef OC_OSCORE
    // create the oscore contexts that still remain
    oc_init_oscore_from_storage(device_index, true);
#endif
  }
}

// ----------------------------------------------------------------------------

void
oc_oscore_set_auth_mac(char *serial_number, int serial_number_size,
                       char *client_recipientid, int client_recipientid_size,
                       uint8_t *shared_key, int shared_key_size)
{
  // create the token & store in at tables at position 0
  // note there should be no entries.. if there is an entry then overwrite
  // it..
  PRINT("oc_oscore_set_auth_mac sn       : %s\n", serial_number);
  PRINT("oc_oscore_set_auth_mac rid [%d] : ", client_recipientid_size);
  oc_char_println_hex(client_recipientid, client_recipientid_size);
  PRINT("oc_oscore_set_auth_mac ms  [%d] : ", shared_key_size);
  oc_char_println_hex(shared_key, shared_key_size);

  oc_auth_at_t spake_entry;
  memset(&spake_entry, 0, sizeof(spake_entry));
  // this is the index in the table, so it is the full string
  oc_new_string(&spake_entry.id, serial_number, serial_number_size);
  spake_entry.ga_len = 0;
  spake_entry.profile = OC_PROFILE_COAP_OSCORE;
  spake_entry.scope = OC_IF_SEC | OC_IF_D | OC_IF_P;
  oc_new_byte_string(&spake_entry.osc_ms, (char *)shared_key, shared_key_size);
  // no context id
  oc_new_byte_string(&spake_entry.osc_rid, client_recipientid,
                     client_recipientid_size);
  // not that HEX was NOT on the wire, but the byte string.
  // so we have to store the byte string
  oc_conv_hex_string_to_oc_string(serial_number, serial_number_size,
                                  &spake_entry.osc_id);

  PRINT("oc_oscore_set_auth_mac osc_id (hex) from serial number: ");
  oc_string_println_hex(spake_entry.osc_id);

  int index = oc_core_find_at_entry_with_id(0, serial_number);
  if (index == -1) {
    index = oc_core_find_at_entry_empty_slot(0);
  }
  if (index == -1) {
    OC_ERR("no space left in auth/at");
  } else {
    oc_core_set_at_table((size_t)0, index, spake_entry, true);
    oc_at_dump_entry((size_t)0, index);
    // add the oscore context...
    oc_init_oscore(0);
  }
}

// This looks very similar to oc_oscore_set_auth_mac, but has some very
// particular differences. The key identifier is the same as before (serial
// number), but the sender & receiver IDs are swapped. Here, the sender ID is
// client_recipientid, referring to the MAC. And the receiver ID is the serial
// number.
void
oc_oscore_set_auth_device(char *serial_number, int serial_number_size,
                          char *client_recipientid, int client_recipientid_size,
                          uint8_t *shared_key, int shared_key_size)
{
  PRINT("oc_oscore_set_auth_device sn :%s\n", serial_number);
  PRINT("oc_oscore_set_auth_device rid : (%d) ", client_recipientid_size);
  oc_char_println_hex(client_recipientid, client_recipientid_size);
  PRINT("oc_oscore_set_auth_device ms : (%d) ", shared_key_size);
  oc_char_println_hex(shared_key, shared_key_size);

  oc_auth_at_t spake_entry;
  memset(&spake_entry, 0, sizeof(spake_entry));
  // this is the index in the table, so it is the full string
  oc_new_string(&spake_entry.id, serial_number, serial_number_size);
  spake_entry.ga_len = 0;
  spake_entry.profile = OC_PROFILE_COAP_OSCORE;
  spake_entry.scope = OC_IF_SEC | OC_IF_D | OC_IF_P;
  oc_new_byte_string(&spake_entry.osc_ms, (char *)shared_key, shared_key_size);
  // no context id
  oc_new_byte_string(&spake_entry.osc_id, client_recipientid,
                     client_recipientid_size);
  oc_conv_hex_string_to_oc_string(serial_number, serial_number_size,
                                  &spake_entry.osc_rid);

  PRINT("  osc_id (hex) from serial number: ");
  oc_string_println_hex(spake_entry.osc_id);

  int index = oc_core_find_at_entry_with_id(0, serial_number);
  if (index == -1) {
    index = oc_core_find_at_entry_empty_slot(0);
  }
  if (index == -1) {
    OC_ERR("no space left in auth/at");
  } else {
    oc_core_set_at_table((size_t)0, index, spake_entry, true);
    oc_at_dump_entry((size_t)0, index);
    // add the oscore context...
    oc_init_oscore(0);
  }
}

oc_auth_at_t *
oc_get_auth_at_entry(size_t device_index, int index)
{
  (void)device_index;

  if (index < 0) {
    return NULL;
  }
  if (index >= G_AT_MAX_ENTRIES) {
    return NULL;
  }
  return &g_at_entries[index];
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

  oc_load_at_table(device_index);

  oc_create_knx_p_oscore_replwdo_resource(OC_KNX_P_OSCORE_REPLWDO,
                                          device_index);
  oc_create_knx_p_oscore_osndelay_resource(OC_KNX_P_OSCORE_OSNDELAY,
                                           device_index);
  oc_create_knx_f_oscore_resource(OC_KNX_F_OSCORE, device_index);
  oc_create_a_sen_resource(OC_KNX_A_SEN, device_index);

  oc_create_auth_resource(OC_KNX_AUTH, device_index);
  oc_create_auth_at_resource(OC_KNX_AUTH_AT, device_index);
  oc_create_auth_at_x_resource(OC_KNX_AUTH_AT_X, device_index);
  oc_create_knx_auth_resource(OC_KNX_AUTH, device_index);
}

void
oc_init_oscore(size_t device_index)
{
  oc_init_oscore_from_storage(device_index, false);
}

void
oc_init_oscore_from_storage(size_t device_index, bool from_storage)
{
#ifndef OC_OSCORE
  (void)device_index;
#else /* OC_OSCORE */
  int i;
  OC_DBG_OSCORE("oc_init_oscore deleting old contexts!!");

  // deleting all contexts!!
  oc_oscore_free_all_contexts();

  OC_DBG_OSCORE("oc_init_oscore adding OSCORE context...");
  for (i = 0; i < G_AT_MAX_ENTRIES; i++) {

    if (oc_string_len(g_at_entries[i].id) > 0) {
      oc_print_auth_at_entry(device_index, i);

      if (g_at_entries[i].profile == OC_PROFILE_COAP_OSCORE) {
        uint64_t ssn = 0;
        // one context: for sending and receiving.
        oc_oscore_context_t *ctx = oc_oscore_add_context(
          device_index, oc_string(g_at_entries[i].osc_id),
          oc_byte_string_len(g_at_entries[i].osc_id),
          oc_string(g_at_entries[i].osc_rid),
          oc_byte_string_len(g_at_entries[i].osc_rid), ssn, "desc",
          oc_string(g_at_entries[i].osc_ms),
          oc_byte_string_len(g_at_entries[i].osc_ms),
          oc_string(g_at_entries[i].osc_contextid),
          oc_byte_string_len(g_at_entries[i].osc_contextid), i, from_storage);
        if (ctx == NULL) {
          OC_ERR("  failed to load index= %d", i);
        }
      } else {
        OC_DBG_OSCORE("oc_init_oscore: no oscore context");
      }
    }
  }
#endif
}

// ----------------------------------------------------------------------------

bool
oc_is_resource_secure(oc_method_t method, oc_resource_t *resource)
{
  // see table 6.1.3: all resources with methods that do not have
  // an interface that is not secure
  if (method == OC_GET &&
      ((oc_string_len(resource->uri) == 17 &&
        memcmp(oc_string(resource->uri), "/.well-known/core", 17) == 0) ||
       (oc_string_len(resource->uri) == 16 &&
        memcmp(oc_string(resource->uri), "/.well-known/knx", 16) == 0) ||
       (oc_string_len(resource->uri) == 20 &&
        memcmp(oc_string(resource->uri), "/.well-known/knx/osn", 20) == 0))) {
    return false;
  }
  // not secure: needed for SPAKE handshake
  if (method == OC_POST &&
      ((oc_string_len(resource->uri) == 22 &&
        memcmp(oc_string(resource->uri), "/.well-known/knx/spake", 22) == 0))) {
    return false;
  }
#ifdef OC_OSCORE
  return true;
#else
  PRINT("oc_is_resource_secure: OSCORE is turned off %s\n",
        oc_string_checked(resource->uri));
  return false;
#endif /* OC_OSCORE*/
}

bool
oc_if_method_allowed_according_to_mask(oc_interface_mask_t iface_mask,
                                       oc_method_t method)
{
  if (iface_mask & OC_IF_I) {
    // logical input
    if (method == OC_POST)
      return true;
    if (method == OC_PUT)
      return true;
  }
  if (iface_mask & OC_IF_O) {
    // Logical Output
    if (method == OC_GET)
      return true;
    if (method == OC_POST)
      return true;
  }
  if (iface_mask & OC_IF_G) {
    // Group Address
    if (method == OC_POST)
      return true;
  }
  if (iface_mask & OC_IF_C) {
    // configuration
    if (method == OC_GET)
      return true;
    if (method == OC_POST)
      return true;
    if (method == OC_PUT)
      return true;
    if (method == OC_DELETE)
      return true;
  }
  if (iface_mask & OC_IF_P) {
    // parameter
    if (method == OC_GET)
      return true;
    if (method == OC_PUT)
      return true;
  }
  if (iface_mask & OC_IF_D) {
    // diagnostic
    if (method == OC_GET)
      return true;
  }
  if (iface_mask & OC_IF_A) {
    // actuator
    if (method == OC_GET)
      return true;
    if (method == OC_PUT)
      return true;
    if (method == OC_POST)
      return true;
  }
  if (iface_mask & OC_IF_S) {
    // sensor
    if (method == OC_GET)
      return true;
  }
  if (iface_mask & OC_IF_LI) {
    // link list
    if (method == OC_GET)
      return true;
  }
  if (iface_mask & OC_IF_B) {
    // batch
    if (method == OC_GET)
      return true;
    if (method == OC_PUT)
      return true;
    if (method == OC_POST)
      return true;
  }
  if (iface_mask & OC_IF_SEC) {
    // security
    if (method == OC_GET)
      return true;
    if (method == OC_PUT)
      return true;
    if (method == OC_POST)
      return true;
    if (method == OC_DELETE)
      return true;
  }
  if (iface_mask & OC_IF_SWU) {
    // software update
    if (method == OC_GET)
      return true;
    if (method == OC_PUT)
      return true;
    if (method == OC_POST)
      return true;
    if (method == OC_DELETE)
      return true;
  }
  if (iface_mask & OC_IF_PM) {
    // programming mode
    if (method == OC_GET)
      return true;
  }
  if (iface_mask & OC_IF_M) {
    // manufacturer
    return true;
  }

  return false;
}

bool
oc_knx_contains_interface(oc_interface_mask_t calling_interfaces,
                          oc_interface_mask_t resource_interfaces)
{
  if ((calling_interfaces & resource_interfaces) > 0) {
    // one of the entries is matching
    return true;
  }
  return false;
}

static bool
method_allowed(oc_method_t method, oc_resource_t *resource,
               oc_endpoint_t *endpoint)
{
  if (oc_is_resource_secure(method, resource) == false) {
    // not a secure resource
    return true;
  }
  PRINT("method allowed flags:");
  PRINTipaddr_flags(*endpoint);
#ifdef OC_OSCORE
  if ((endpoint->flags & OSCORE) == 0) {
    // not an OSCORE protected message, but OSCORE is enabled
    // so the is call is unprotected and should not go ahead
    OC_DBG_OSCORE("unprotected message, access denied for: %s [%s]",
                  get_method_name(method), oc_string_checked(resource->uri));
    return false;
  }
  if ((endpoint->flags & OSCORE_DECRYPTED) == 0) {
    // not an decrypted message
    OC_DBG_OSCORE("not a decrypted message, access denied for: %s [%s]",
                  get_method_name(method), oc_string_checked(resource->uri));
    return false;
  }
  if (endpoint->auth_at_index > 0) {
    // interface of the call, e.g. of the auth/at entry that was used to decrypt
    // the message
    oc_interface_mask_t calling_interfaces =
      oc_at_get_interface_mask(0, endpoint->auth_at_index - 1);
    // interfaces of the resource
    oc_interface_mask_t resource_interfaces = resource->interfaces;
    if (oc_knx_contains_interface(calling_interfaces, resource_interfaces) ==
        false) {
      PRINT("method_allowed : not allowed: request  %d : ", calling_interfaces);
      oc_print_interface(calling_interfaces);
      PRINT("\n");
      PRINT("method_allowed : not allowed: resource %d : ",
            resource_interfaces);
      oc_print_interface(resource_interfaces);
      PRINT("\n");
      OC_WRN(" resource %s call denied: %d  %d", oc_string(resource->uri),
             calling_interfaces, resource_interfaces);

      return false;
    }
  }

#endif

  return oc_if_method_allowed_according_to_mask(resource->interfaces, method);
}

bool
oc_knx_sec_check_acl(oc_method_t method, oc_resource_t *resource,
                     oc_endpoint_t *endpoint)
{
  // first check if the method is allowed on the resource
  // also checks if the resource is unsecured (public)
  // and if OSCORE is enabled
  if (method_allowed(method, resource, endpoint) == true) {
    return true;
  }
  OC_WRN("oc_knx_sec_check_acl: method %s NOT allowed on %s\n",
         get_method_name(method), oc_string_checked(resource->uri));

  return false;
}