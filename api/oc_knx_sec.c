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
#include "security/oc_oscore_context.h"
#include "oc_knx.h"

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

static void oc_at_entry_print(size_t device_index, int index);
static void oc_at_delete_entry(size_t device_index, int index);
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
  if (request->accept != APPLICATION_CBOR) {
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
  if (request->accept != APPLICATION_CBOR) {
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
  if (request->accept != APPLICATION_CBOR) {
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
  // TODO: what is resource type?
  oc_core_populate_resource(resource_idx, device, "/f/oscore", OC_IF_LI,
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
    PRINT("oc_core_a_sen_post_handler: key: (check) %s \n",
          oc_string(rep->name));
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
        PRINT(" find_access_token_from_payload: %s \n", oc_string(*index));
        return index;
      }
    } break;
    case OC_REP_STRING: {
      if (oc_string_len(object->name) == 0 && object->iname == 0) {
        index = &object->value.string;
        PRINT(" find_access_token_from_payload: %s \n", oc_string(*index));
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
  for (i = 0; i < G_AT_MAX_ENTRIES; i++) {
    // g_at_entries[i].contextId != NULL &&
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
  int index = -1;
  PRINT("oc_core_auth_at_post_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  size_t device_index = request->resource->device;

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
        PRINT("   already exist!\n");
      } else {
        index = find_empty_at_index();
        if (index == -1) {
          PRINT("  no space left!\n");
          oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
          return;
        }
      }
      PRINT("  storage index: %d (%s)\n", index, oc_string(*at));
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
                uint64_t *cur_arr = g_at_entries[index].ga;
                if (cur_arr) {
                  free(cur_arr);
                }
                g_at_entries[index].ga = NULL;
              }
              g_at_entries[index].ga_len = (int)array_size;
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
          // old, but keep it there for now...
          // profile (19 ("coap_dtls" or "coap_oscore"))
          if (object->iname == 19) {
            g_at_entries[index].profile =
              oc_string_to_at_profile(object->value.string);
          }
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
          if (object->iname == 19) {
            // profile (19 ("coap_dtls" ==1 or "coap_oscore" == 2))
            g_at_entries[index].profile = (int)object->value.integer;
          }
        } else if (object->type == OC_REP_OBJECT) {
          // level of cnf or sub.
          subobject = object->value.object;
          int subobject_nr = object->iname;
          PRINT("  subobject_nr %d\n", subobject_nr);
          while (subobject) {
            if (subobject->type == OC_REP_STRING) {
              // if (subobject->iname == 2 && subobject_nr == 2) {
              //  // sub::dnsname :: 2:x?
              //  oc_free_string(&(g_at_entries[index].dnsname));
              //  oc_new_string(&g_at_entries[index].dnsname,
              //                oc_string(subobject->value.string),
              //                oc_string_len(subobject->value.string));
              //}
              // if (subobject->iname == 3 && subobject_nr == 8) {
              //  // cnf::kty 8::2
              //  oc_free_string(&(g_at_entries[index].kty));
              //  oc_new_string(&g_at_entries[index].kty,
              //                oc_string(subobject->value.string),
              //                oc_string_len(subobject->value.string));
              //}
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
                if (oscobject->type == OC_REP_STRING) {
                  if (oscobject->iname == 0 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::kid (id)
                    oc_free_string(&(g_at_entries[index].osc_id));
                    oc_new_string(&g_at_entries[index].osc_id,
                                  oc_string(oscobject->value.string),
                                  oc_string_len(oscobject->value.string));
                  }
                  if (oscobject->iname == 2 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::ms
                    oc_free_string(&(g_at_entries[index].osc_ms));
                    oc_new_string(&g_at_entries[index].osc_ms,
                                  oc_string(oscobject->value.string),
                                  oc_string_len(oscobject->value.string));
                  }
                  if (oscobject->iname == 4 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::alg
                    oc_free_string(&(g_at_entries[index].osc_alg));
                    oc_new_string(&g_at_entries[index].osc_alg,
                                  oc_string(oscobject->value.string),
                                  oc_string_len(oscobject->value.string));
                  }
                  if (oscobject->iname == 6 && subobject_nr == 8 &&
                      oscobject_nr == 4) {
                    // cnf::osc::contextId
                    oc_free_string(&(g_at_entries[index].osc_contextid));
                    oc_new_string(&g_at_entries[index].osc_contextid,
                                  oc_string(oscobject->value.string),
                                  oc_string_len(oscobject->value.string));
                  }
                }
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
    oc_at_entry_print(device_index, index);
    // dump the entry to persistent storage
    oc_at_dump_entry(device_index, index);
    rep = rep->next;
  } // while (rep)

  PRINT("oc_core_auth_at_post_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_CHANGED);
}

static void
oc_core_auth_at_delete_handler(oc_request_t *request,
                               oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_auth_at_delete_handler\n");

  size_t device_index = request->resource->device;

  oc_delete_at_table(device_index);
}

void
oc_create_auth_at_resource(int resource_idx, size_t device)
{
  oc_core_populate_resource(resource_idx, device, "/auth/at",
                            OC_IF_B | OC_IF_SEC, APPLICATION_LINK_FORMAT,
                            OC_DISCOVERABLE, oc_core_auth_at_get_handler, 0,
                            oc_core_auth_at_post_handler,
                            oc_core_auth_at_delete_handler, 1, "dpt.a[n]");
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
    PRINT("index in structure not found\n");
    return;
  }
  // return the data
  oc_rep_begin_root_object();
  // id : 0
  oc_rep_i_set_text_string(root, 0, oc_string(g_at_entries[index].id));
  // profile : 19
  oc_rep_i_set_int(root, 19, g_at_entries[index].profile);
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
    oc_rep_i_set_int_array(root, 9, g_at_entries[index].ga,
                           g_at_entries[index].ga_len);
  }
  if (g_at_entries[index].profile == OC_PROFILE_COAP_DTLS) {
    if (oc_string_len(g_at_entries[index].sub) > 0) {
      PRINT("    sub    : %s\n", oc_string(g_at_entries[index].sub));
    }
    if (oc_string_len(g_at_entries[index].kid) > 0) {
      PRINT("    kid    : %s\n", oc_string(g_at_entries[index].kid));
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
    if (oc_string_len(g_at_entries[index].osc_id) > 0) {
      oc_rep_i_set_text_string(
        osc, 0, oc_string(g_at_entries[index].osc_id)); // root::cnf::osc::id
    }
    if (oc_string_len(g_at_entries[index].osc_ms) > 0) {
      oc_rep_i_set_text_string(
        osc, 2, oc_string(g_at_entries[index].osc_ms)); // root::cnf::osc::ms
    }
    if (oc_string_len(g_at_entries[index].osc_alg) > 0) {
      oc_rep_i_set_text_string(
        osc, 4, oc_string(g_at_entries[index].osc_alg)); // root::cnf::osc::alg
    }
    if (oc_string_len(g_at_entries[index].osc_contextid) > 0) {
      oc_rep_i_set_text_string(
        osc, 6,
        oc_string(
          g_at_entries[index].osc_contextid)); // root::cnf::osc::contextid
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
    PRINT("index in struct not found\n");
    return;
  }
  // actual delete of the context id so that this entry is seen as empty
  oc_at_delete_entry(device_index, index);
  // do the persistent storage
  oc_at_dump_entry(device_index, index);
  PRINT("oc_core_auth_at_x_delete_handler - done\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
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
  oc_core_populate_resource(
    resource_idx, device, "/auth", OC_IF_LI, APPLICATION_LINK_FORMAT,
    OC_DISCOVERABLE, oc_core_knx_auth_get_handler, 0, 0, 0, 1, "urn:knx:xxx");
}

static void
oc_at_entry_print(size_t device_index, int index)
{
  (void)device_index;
  // PRINT("  at index: %d\n", index);
  if (index > -1) {
    if (g_at_entries[index].profile != OC_PROFILE_UNKNOWN) {

      PRINT("  at index: %d\n", index);
      PRINT("    id (0)        : %s\n", oc_string(g_at_entries[index].id));
      PRINT("    interfaces    : %d\n", g_at_entries[index].scope);
      PRINT("    profile (38)  : %d (%s)\n", g_at_entries[index].profile,
            oc_at_profile_to_string(g_at_entries[index].profile));
      if (g_at_entries[index].profile == OC_PROFILE_COAP_DTLS) {
        if (oc_string_len(g_at_entries[index].sub) > 0) {
          PRINT("    sub           : %s\n", oc_string(g_at_entries[index].sub));
        }
        if (oc_string_len(g_at_entries[index].kid) > 0) {
          PRINT("    kid           : %s\n", oc_string(g_at_entries[index].kid));
        }
      }
      if (g_at_entries[index].profile == OC_PROFILE_COAP_OSCORE) {
        if (oc_string_len(g_at_entries[index].osc_id) > 0) {
          PRINT("    osc:id        : %s\n",
                oc_string(g_at_entries[index].osc_id));
        }
        if (oc_string_len(g_at_entries[index].osc_ms) > 0) {
          PRINT("    osc:ms        : ");
          int length = (int)oc_string_len(g_at_entries[index].osc_ms);
          char *ms = oc_string(g_at_entries[index].osc_ms);
          for (int i = 0; i < length; i++) {
            PRINT("%02x", (unsigned char)ms[i]);
          }
          PRINT("\n");
        }
        if (oc_string_len(g_at_entries[index].osc_alg) > 0) {
          PRINT("    osc:alg       : %s\n",
                oc_string(g_at_entries[index].osc_alg));
        }
        if (oc_string_len(g_at_entries[index].osc_contextid) > 0) {
          PRINT("    osc:contextid : %s\n",
                oc_string(g_at_entries[index].osc_contextid));
        }
      }
    }
  }
}

static void
oc_at_delete_entry(size_t device_index, int index)
{
  (void)device_index;
  // generic
  oc_free_string(&g_at_entries[index].id);
  oc_new_string(&g_at_entries[index].id, "", 0);
  g_at_entries[index].scope = OC_IF_NONE;
  g_at_entries[index].profile = OC_PROFILE_UNKNOWN;
  // oscore
  oc_free_string(&g_at_entries[index].osc_alg);
  oc_new_string(&g_at_entries[index].osc_alg, "", 0);
  oc_free_string(&g_at_entries[index].osc_id);
  oc_new_string(&g_at_entries[index].osc_id, "", 0);
  oc_free_string(&g_at_entries[index].osc_ms);
  oc_new_string(&g_at_entries[index].osc_ms, "", 0);
  oc_free_string(&g_at_entries[index].osc_contextid);
  oc_new_string(&g_at_entries[index].osc_contextid, "", 0);
  // dtls
  oc_free_string(&g_at_entries[index].sub);
  oc_new_string(&g_at_entries[index].sub, "", 0);
  oc_free_string(&g_at_entries[index].kid);
  oc_new_string(&g_at_entries[index].kid, "", 0);
}

static void
oc_at_dump_entry(size_t device_index, int entry)
{
  (void)device_index;
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
  oc_rep_i_set_int(root, 19, g_at_entries[entry].profile);

  oc_rep_i_set_text_string(root, 840, oc_string(g_at_entries[entry].osc_id));
  oc_rep_i_set_text_string(root, 842, oc_string(g_at_entries[entry].osc_ms));
  oc_rep_i_set_text_string(root, 844, oc_string(g_at_entries[entry].osc_alg));
  oc_rep_i_set_text_string(root, 846,
                           oc_string(g_at_entries[entry].osc_contextid));

  oc_rep_i_set_text_string(root, 82, oc_string(g_at_entries[entry].sub));
  oc_rep_i_set_text_string(root, 81, oc_string(g_at_entries[entry].kid));

  oc_rep_end_root_object();

  int size = oc_rep_get_encoded_payload_size();
  if (size > 0) {
    OC_DBG("oc_at_dump_entry: dumped current state [%s] [%d]: "
           "size %d",
           filename, entry, size);
    long written_size = oc_storage_write(filename, buf, size);
    if (written_size != (long)size) {
      PRINT("oc_at_dump_entry: written %d != %d (towrite)\n", (int)written_size,
            size);
    }
  }
  free(buf);
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
          if (rep->iname == 19) {
            g_at_entries[entry].profile = (int)rep->value.integer;
          }
          break;
        case OC_REP_STRING:
          if (rep->iname == 0) {
            oc_free_string(&g_at_entries[entry].id);
            oc_new_string(&g_at_entries[entry].id, oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 840) {
            oc_free_string(&g_at_entries[entry].osc_id);
            oc_new_string(&g_at_entries[entry].osc_id,
                          oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 842) {
            oc_free_string(&g_at_entries[entry].osc_ms);
            oc_new_string(&g_at_entries[entry].osc_ms,
                          oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 844) {
            oc_free_string(&g_at_entries[entry].osc_alg);
            oc_new_string(&g_at_entries[entry].osc_alg,
                          oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 846) {
            oc_free_string(&g_at_entries[entry].osc_contextid);
            oc_new_string(&g_at_entries[entry].osc_contextid,
                          oc_string(rep->value.string),
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
oc_core_set_at_table(size_t device_index, int index, oc_auth_at_t entry)
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
    oc_free_string(&g_at_entries[index].osc_id);
    oc_new_string(&g_at_entries[index].osc_id, oc_string(entry.osc_id),
                  oc_string_len(entry.osc_id));
    oc_free_string(&g_at_entries[index].osc_ms);
    oc_new_string(&g_at_entries[index].osc_ms, oc_string(entry.osc_ms),
                  oc_string_len(entry.osc_ms));
    oc_free_string(&g_at_entries[index].osc_id);
    oc_new_string(&g_at_entries[index].osc_alg, oc_string(entry.osc_alg),
                  oc_string_len(entry.osc_alg));
    oc_free_string(&g_at_entries[index].osc_contextid);
    oc_new_string(&g_at_entries[index].osc_contextid,
                  oc_string(entry.osc_contextid),
                  oc_string_len(entry.osc_contextid));

    g_at_entries[index].ga_len = entry.ga_len;

    // make deep copy..
    size_t array_size = (int)entry.ga_len;
    // not making a deep copy
    if (array_size > 0) {
      // make the deep copy
      if (g_at_entries[index].ga_len > 0) {
        int64_t *cur_arr = g_at_entries[index].ga;
        if (cur_arr) {
          free(cur_arr);
        }
        // free(&g_at_entries[index].ga);
      }
      g_at_entries[index].ga_len = (int)array_size;
      int64_t *new_array = (int64_t *)malloc(array_size * sizeof(uint64_t));

      if (new_array) {
        for (size_t i = 0; i < array_size; i++) {
          new_array[i] = entry.ga[i];
        }
        g_at_entries[index].ga = new_array;
      } else {
        OC_ERR("out of memory");
      }
    }

    oc_at_dump_entry(device_index, index);
  }
  if (index == 0) {
    // set the OSCORE stuff
  }

  return 0;
}

int
oc_core_find_at_entry_with_context_id(size_t device_index, char *context_id)
{
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    if ((oc_string_len(g_at_entries[i].id) > 0) &&
        (strncmp(oc_string(g_at_entries[i].id), context_id,
                 strlen(context_id)) == 0)) {
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
      oc_at_entry_print(device_index, i);
    }
  }
}

void
oc_delete_at_table(size_t device_index)
{
  PRINT("Deleting AT Object Table from Persistent storage\n");
  for (int i = 0; i < G_AT_MAX_ENTRIES; i++) {
    oc_at_delete_entry(device_index, i);
    oc_at_entry_print(device_index, i);
    oc_at_dump_entry(device_index, i);
  }
#ifdef OC_OSCORE
  oc_oscore_free_all_contexts();
#endif
}

// ----------------------------------------------------------------------------

void
oc_oscore_set_auth(char *serial_number, char *context_id, uint8_t *shared_key,
                   int shared_key_size)
{
  // create the token & store in at tables at position 0
  // note there should be no entries.. if there is an entry then overwrite
  // it..
  PRINT("oc_oscore_set_auth sn:%s ci:%s\n", serial_number, context_id);

  oc_auth_at_t os_token;
  memset(&os_token, 0, sizeof(os_token));
  oc_new_string(&os_token.id, context_id, strlen(context_id));
  os_token.ga_len = 0;
  os_token.profile = OC_PROFILE_COAP_OSCORE;
  os_token.scope = OC_IF_SEC | OC_IF_D | OC_IF_P;
  oc_new_string(&os_token.osc_ms, (char *)shared_key, shared_key_size);
  // TODO this is the default, when no context_id is supplied
  // oc_new_string(&os_token.osc_id, "responderkey", strlen("responderkey"));
  oc_new_string(&os_token.osc_id, context_id, strlen(context_id));
  oc_new_string(&os_token.osc_contextid, context_id, strlen(context_id));
  oc_new_string(&os_token.sub, "", strlen(""));

  int index = oc_core_find_at_entry_with_context_id(0, context_id);
  if (index == -1) {
    index = oc_core_find_at_entry_empty_slot(0);
  }
  if (index == -1) {
    OC_ERR("no space left in auth/at");
  } else {
    oc_core_set_at_table((size_t)0, index, os_token);
    // add the oscore context...
    oc_init_oscore(0);
  }
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
  oc_create_auth_at_resource(OC_KNX_AUTH_AT, device_index);
  oc_create_auth_at_x_resource(OC_KNX_AUTH_AT_X, device_index);
  oc_create_knx_auth_resource(OC_KNX_AUTH, device_index);
}

void
oc_init_oscore(size_t device_index)
{
#ifndef OC_OSCORE
  (void)device_index;
#else /* OC_OSCORE */
  int i;
  PRINT("oc_init_oscore deleting old contexts!!\n");

  // deleting all contexts!!
  oc_oscore_free_all_contexts();

  PRINT("oc_init_oscore adding OSCORE context, using context id for sender & "
        "receiver\n");
  for (i = 0; i < G_AT_MAX_ENTRIES; i++) {

    if (oc_string_len(g_at_entries[i].id) > 0) {
      oc_at_entry_print(device_index, i);

      uint64_t ssn = 0;
      // ssn = oc_knx_get_osn();

      // one context: for sending and receiving.
      oc_oscore_context_t *ctx = oc_oscore_add_context(
        device_index, oc_string(g_at_entries[i].osc_contextid),
        oc_string(g_at_entries[i].osc_contextid), ssn, "desc",
        oc_string(g_at_entries[i].osc_ms),
        oc_string(g_at_entries[i].osc_contextid), false);
      if (ctx == NULL) {
        PRINT("   fail...\n ");
      }

      // ctx = oc_oscore_add_context(
      //  device_index, "reci", "sender", oc_knx_get_osn(), "desc",
      //  oc_string(g_at_entries[i].osc_ms), "token2", false);
      // if (ctx == NULL) {
      //  PRINT("   fail...\n ");
      //}
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
  PRINT("oc_is_resource_secure: OSCORE is turned off\n");
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

static bool
method_allowed(oc_method_t method, oc_resource_t *resource,
               oc_endpoint_t *endpoint)
{
  if (oc_is_resource_secure(method, resource) == false) {
    return true;
  }
  PRINT("method allowed flags:");
  PRINTipaddr_flags(*endpoint);
#ifdef OC_OSCORE
  if ((endpoint->flags & OSCORE) == 0) {
    // not an OSCORE protected message, but OSCORE is enabled
    // so the is call is unprotected and should not go ahead
    OC_DBG_OSCORE("unprotected message, access denied for: %s [%s]",
                  get_method_name(method), oc_string(resource->uri));
    return false;
  }
  if ((endpoint->flags & OSCORE_DECRYPTED) == 0) {
    // not an decrypted message
    OC_DBG_OSCORE("not a decrypted message, access denied for: %s [%s]",
                  get_method_name(method), oc_string(resource->uri));
    return false;
  }
#endif

  return oc_if_method_allowed_according_to_mask(resource->interfaces, method);
}

bool
oc_knx_contains_interface(oc_interface_mask_t at_interface,
                          oc_interface_mask_t resource_interface)
{
  int i;
  oc_interface_mask_t new_mask;
  oc_interface_mask_t at_mask;
  oc_interface_mask_t resource_mask;
  // PRINT("------ oc_knx_contains_interface  at  %d resource %d  \n",
  // at_interface, resource_interface);
  for (i = 1; i < OC_MAX_IF_MASKS + 1; i++) {
    new_mask = 1 << i;
    at_mask = at_interface & new_mask;
    resource_mask = resource_interface & new_mask;
    // PRINT("oc_knx_contains_interface  %d %d %d %d  \n", i, new_mask, at_mask,
    //      resource_mask);
    if ((at_mask != 0) && (at_mask == resource_mask)) {
      return true;
    }
  }

  return false;
}

bool
oc_knx_sec_check_interface(oc_resource_t *resource, oc_string_t *token)
{
  if (resource == NULL) {
    return false;
  }
  if (token == NULL) {
    return false;
  }
  oc_interface_mask_t resource_interfaces = resource->interfaces;
  int index = find_index_from_at(token);
  if (index < 0) {
    return false;
  }

  return oc_knx_contains_interface(g_at_entries[index].scope,
                                   resource_interfaces);
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
  PRINT("oc_knx_sec_check_acl: method %s NOT allowed on %s\n",
        get_method_name(method), oc_string(resource->uri));

  return false;
}