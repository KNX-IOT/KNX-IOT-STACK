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
#include "oc_ri.h"
#include "api/oc_knx_gm.h"
#include "api/oc_knx_fp.h"
#include "oc_discovery.h"
#include "oc_core_res.h"
#include "oc_knx_helpers.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// DEBUGGING
//#define OC_IOT_ROUTER

#ifndef G_GM_MAX_ENTRIES
#define G_GM_MAX_ENTRIES 20
#endif

#ifdef OC_IOT_ROUTER
// ----------------------------------------------------------------------------
static uint32_t g_fra = 0; // the IPv4 sync latency fraction.
static uint32_t g_tol = 0; // the IPv4 routing latency tolerance
static uint32_t g_ttl = 0; // The value defines how many routers a multicast
                           // message
                           // MAY pass until it gets discarded.
#endif

static oc_string_t g_key; // IPv4 routing backbone key.
static uint32_t g_mcast;  // Current IPv4 routing multicast address.

int
oc_core_get_group_mapping_table_size()
{

#ifdef OC_IOT_ROUTER
  return G_GM_MAX_ENTRIES;
#else
  return 0;
#endif
}

int
oc_get_f_netip_ttl(size_t device_index)
{
  (void)device_index;

#ifdef OC_IOT_ROUTER
  return g_ttl;
#else
  PRINT("OC_IOT_ROUTER not compiled in");
  return 0;
#endif
}

int
oc_get_f_netip_fra(size_t device_index)
{
  (void)device_index;
#ifdef OC_IOT_ROUTER
  return g_fra;
#else
  OC_WRN("OC_IOT_ROUTER not compiled in");
  return 0;
#endif
}

int
oc_get_f_netip_tol(size_t device_index)
{
  (void)device_index;
#ifdef OC_IOT_ROUTER
  return g_tol;
#else
  OC_WRN("OC_IOT_ROUTER not compiled in");
  return 0;
#endif
}

oc_string_t
oc_get_f_netip_key(size_t device_index)
{
  (void)device_index;
#ifdef OC_IOT_ROUTER
  return g_key;
#else
  OC_WRN("OC_IOT_ROUTER not compiled in");
  memset(&g_key, 0, sizeof(g_key));
  return g_key;
#endif
}

uint32_t
oc_get_f_netip_mcast(size_t device_index)
{
  (void)device_index;
#ifdef OC_IOT_ROUTER
  return g_mcast;
#else
  OC_WRN("OC_IOT_ROUTER not compiled in");
  memset(&g_mcast, 0, sizeof(g_mcast));
  return g_mcast;
#endif
}

#ifdef OC_IOT_ROUTER

// ----------------------------------------------------------------------------

/**
 * @brief print the entry in the Group Mapping Table
 *
 * @param entry the index of the entry in the Group Mapping Table
 */
void oc_print_group_mapping_table_entry(int entry);

/**
 * @brief dump the entry of the Group Mapping Table (to persistent) storage
 *
 * @param entry the index of the entry in the Group Mapping Table
 */
void oc_dump_group_mapping_table_entry(int entry);

/**
 * @brief load the entry of the Group Mapping Table (from persistent) storage
 *
 * @param entry the index of the entry in the Group Mapping Table
 */
void oc_load_group_mapping_table_entry(int entry);

/** the storage identifiers */
#define GM_STORE "gm_store"
#define GM_STORE_FRA "gm_store_fra"
#define GM_STORE_TOL "gm_store_tol"
#define GM_STORE_TTL "gm_store_ttl"
#define GM_STORE_KEY "gm_store_key"
#define GM_STORE_MCAST "gm_store_mcast"

#define GM_ENTRY_MAX_SIZE (1024)

/** the list of group mappings */
oc_group_mapping_table_t g_gm_entries[G_GM_MAX_ENTRIES];

// ----------------------------------------------------------------------------

static int
find_empty_group_mapping_index()
{
  for (int i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    if (g_gm_entries[i].ga_len == 0) {
      return i;
    }
  }
  return -1;
}

static int
find_group_mapping_index(int id)
{
  int len;
  for (int i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    if (g_gm_entries[i].id == id) {
      return i;
    }
  }
  return -1;
}

int
oc_core_set_group_mapping_table(size_t device_index, int index,
                                oc_group_mapping_table_t entry, bool store)
{
  if (index > oc_core_get_group_mapping_table_size()) {
    return -1;
  }

  g_gm_entries[index].id = entry.id;
  g_gm_entries[index].dataType = entry.dataType;
  g_gm_entries[index].ga_len = entry.ga_len;

  if (entry.ga_len > 0) {
    int array_size = entry.ga_len;
    // make the deep copy of the array
    if ((g_gm_entries[index].ga_len > 0) && (&g_gm_entries[index].ga != NULL)) {
      uint64_t *cur_arr = g_gm_entries[index].ga;
      if (cur_arr) {
        free(g_gm_entries[index].ga);
      }
      g_gm_entries[index].ga = NULL;
    }
    g_gm_entries[index].ga_len = (int)array_size;
    uint64_t *new_array = (int64_t *)malloc(array_size * sizeof(uint64_t));
    if (new_array) {
      for (size_t i = 0; i < array_size; i++) {
        new_array[i] = entry.ga[i];
      }
      g_gm_entries[index].ga = new_array;
    } else {
      OC_ERR("out of memory");
    }
  }

  // security part
  oc_free_string(&(g_gm_entries[index].groupKey));
  oc_new_string(&g_gm_entries[index].groupKey, oc_string(entry.groupKey),
                oc_string_len(entry.groupKey));
  g_gm_entries[index].authentication = entry.authentication;
  g_gm_entries[index].confidentiality = entry.confidentiality;

  return 0;
}

void
oc_print_group_mapping_table_entry(int entry)
{
  if (g_gm_entries[entry].ga_len == 0) {
    return;
  }

  PRINT("    id (0)         : %d\n", g_gm_entries[entry].id);
  PRINT("    dataType (116) : %d\n", g_gm_entries[entry].dataType);
  PRINT("    ga (7)        : [");
  for (int i = 0; i < g_gm_entries[entry].ga_len; i++) {
    PRINT(" %" PRIu64 "", (uint64_t)g_gm_entries[entry].ga[i]);
  }
  PRINT(" ]\n");
  if (oc_string_len(g_gm_entries[entry].groupKey) > 0) {
    PRINT("    groupKey        : ");
    int length = (int)oc_string_len(g_gm_entries[entry].groupKey);
    char *ms = oc_string(g_gm_entries[entry].groupKey);
    for (int i = 0; i < length; i++) {
      PRINT("%02x", (unsigned char)ms[i]);
    }
    PRINT("\n");
    PRINT("    a (97)         : %d\n", g_gm_entries[entry].authentication);
    PRINT("    c (99)         : %d\n", g_gm_entries[entry].confidentiality);
  }
}

void
oc_dump_group_mapping_table_entry(int entry)
{
  char filename[20];
  snprintf(filename, 20, "%s_%d", GM_STORE, entry);

  uint8_t *buf = malloc(OC_MAX_APP_DATA_SIZE);
  if (!buf)
    return;

  oc_rep_new(buf, OC_MAX_APP_DATA_SIZE);
  // write the data
  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_gm_entries[entry].id);
  // dataType
  oc_rep_i_set_int(root, 116, g_gm_entries[entry].dataType);
  // ga - 7
  oc_rep_i_set_int_array(root, 7, g_gm_entries[entry].ga,
                         g_gm_entries[entry].ga_len);
  // security
  oc_rep_i_set_boolean(root, 97, g_gm_entries[entry].authentication);
  oc_rep_i_set_boolean(root, 99, g_gm_entries[entry].confidentiality);
  oc_rep_i_set_byte_string(root, 107, oc_string(g_gm_entries[entry].groupKey),
                           oc_string_len(g_gm_entries[entry].groupKey));

  oc_rep_end_root_object();

  int size = oc_rep_get_encoded_payload_size();
  if (size > 0) {
    OC_DBG("oc_dump_group_mapping_table_entry: dumped current state [%s] [%d]: "
           "size %d",
           filename, entry, size);
    long written_size = oc_storage_write(filename, buf, size);
    if (written_size != (long)size) {
      PRINT("oc_dump_group_mapping_table_entry: written %d != %d (towrite)\n",
            (int)written_size, size);
    }
  }

  free(buf);
}

void
oc_load_group_mapping_table_entry(int entry)
{
  long ret = 0;
  char filename[20];
  snprintf(filename, 20, "%s_%d", GM_STORE, entry);

  oc_rep_t *rep, *head;

  uint8_t *buf = malloc(GM_ENTRY_MAX_SIZE);
  if (!buf) {
    return;
  }

  ret = oc_storage_read(filename, buf, GM_ENTRY_MAX_SIZE);
  if (ret > 0) {
    struct oc_memb rep_objects = { sizeof(oc_rep_t), 0, 0, 0, 0 };
    oc_rep_set_pool(&rep_objects);
    int err = oc_parse_rep(buf, ret, &rep);
    head = rep;
    if (err == 0) {
      while (rep != NULL) {
        switch (rep->type) {
        case OC_REP_INT:
          if (rep->iname == 0) {
            g_gm_entries[entry].id = (int)rep->value.integer;
          }
          if (rep->iname == 116) {
            g_gm_entries[entry].dataType = (int)rep->value.integer;
          }
          break;

        case OC_REP_BYTE_STRING:
          if (rep->iname == 107) {
            // g_gm_entries[entry].authentication = (int)rep->value.boolean;
            oc_free_string(&g_gm_entries[entry].groupKey);
            oc_new_string(&g_gm_entries[entry].groupKey,
                          oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          break;
        case OC_REP_BOOL:
          if (rep->iname == 97) {
            g_gm_entries[entry].authentication = (int)rep->value.boolean;
          }
          if (rep->iname == 99) {
            g_gm_entries[entry].confidentiality = (int)rep->value.boolean;
          }
          break;
        case OC_REP_INT_ARRAY:
          if (rep->iname == 7) {
            int64_t *arr = oc_int_array(rep->value.array);
            int array_size = (int)oc_int_array_size(rep->value.array);
            uint64_t *new_array =
              (uint64_t *)malloc(array_size * sizeof(uint64_t));
            if ((new_array) && (array_size > 0)) {
              for (int i = 0; i < array_size; i++) {
#pragma warning(suppress : 6386)
                new_array[i] = (uint32_t)arr[i];
              }
              if (g_gm_entries[entry].ga != 0) {
                free(g_gm_entries[entry].ga);
              }
              PRINT("  ga size %d\n", array_size);
              g_gm_entries[entry].ga_len = array_size;
              g_gm_entries[entry].ga = new_array;
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

void
oc_load_group_mapping_table()
{
  PRINT("Loading Group Mapping Table from Persistent storage\n");
  for (int i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    oc_load_group_mapping_table_entry(i);
    oc_print_group_mapping_table_entry(i);
  }
}

void
oc_free_group_mapping_table_entry(int entry, bool init)
{
  g_gm_entries[entry].id = -1;
  if (init == false) {
    free(g_gm_entries[entry].ga);
  }
  // free key
  oc_free_string(&g_gm_entries[entry].groupKey);
  oc_new_string(&g_gm_entries[entry].groupKey, "", 0);

  g_gm_entries[entry].ga = NULL;
  g_gm_entries[entry].ga_len = 0;
}

void
oc_delete_group_mapping_table_entry(int entry)
{
  char filename[20];
  snprintf(filename, 20, "%s_%d", GM_STORE, entry);
  oc_storage_erase(filename);

  oc_free_group_mapping_table_entry(entry, false);
}

void
oc_free_group_mapping_table()
{
  PRINT("Free Group Mapping Table\n");
  for (int i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    oc_free_group_mapping_table_entry(i, false);
  }
}

int
oc_core_find_nr_used_in_group_mapping_table()
{
  int counter = 0;
  for (int i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    if (g_gm_entries[i].ga_len > 0) {
      counter++;
    }
  }
  return counter;
}

// -----------------------------------------------------------------------------

static void
oc_core_fp_gm_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  bool ps_exists;
  bool total_exists;
  PRINT("oc_core_fp_gm_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // handle query parameters: l=ps l=total
  if (check_if_query_l_exist(request, &ps_exists, &total_exists)) {
    // example : < / fp / r / ? l = total>; total = 22; ps = 5

    length = oc_frame_query_l("/fp/gm", ps_exists, total_exists);
    response_length += length;
    if (ps_exists) {
      length = oc_rep_add_line_to_buffer(";ps=");
      response_length += length;
      length = oc_frame_integer(oc_core_get_group_mapping_table_size());
      response_length += length;
    }
    if (total_exists) {
      length = oc_rep_add_line_to_buffer(";total=");
      response_length += length;
      length = oc_frame_integer(oc_core_find_nr_used_in_group_mapping_table());
      response_length += length;
    }

    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }

  /* example entry: </fp/gm/1>;ct=60 (cbor)*/
  for (i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    if (g_gm_entries[i].ga_len == 0) {
      // index not in use
      break;
    }
    if (response_length > 0) {
      // add the comma for the previous entry
      // there is a next one.
      length = oc_rep_add_line_to_buffer(",\n");
      response_length += length;
    }

    length = oc_rep_add_line_to_buffer("</fp/gm/");
    response_length += length;
    char string[10];
    sprintf((char *)&string, "%d>;ct=60", i + 1);
    length = oc_rep_add_line_to_buffer(string);
    response_length += length;
  }

  oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  // if (response_length > 0) {
  //   oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  // } else {
  //   oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  // }

  PRINT("oc_core_fp_gm_get_handler - end\n");
}

static void
oc_core_fp_gm_post_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  oc_rep_t *object = NULL;
  oc_rep_t *s_object = NULL;
  oc_rep_t *sec_object = NULL;
  oc_status_t return_status = OC_STATUS_BAD_REQUEST;
  int id = -1;

  PRINT("oc_core_fp_gm_post_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // check loading state
  size_t device_index = request->resource->device;
  if (oc_knx_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  // find the id of the entry
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;
      id = oc_table_find_id_from_rep(object);
      if (id == -1) {
        OC_ERR("  ERROR id %d", id);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      // entry storage
      oc_group_mapping_table_t my_entry;

      int index = find_group_mapping_index(id);
      if (index != -1) {
        PRINT("   entry already exist! \n");
        return_status = OC_STATUS_CHANGED;
      } else {
        index = find_empty_group_mapping_index();
        if (index == -1) {
          PRINT("  no space left!\n");
          oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
          return;
        }
        return_status = OC_STATUS_CREATED;
      }
      PRINT("  storage index: %d (%d)\n", index, id);
      g_gm_entries[index].id = id;

      // parse the response
      object = rep->value.object;
      while (object != NULL) {
        if (object->type == OC_REP_INT_ARRAY) {
          // ga
          if (object->iname == 7) {
            int64_t *array = 0;
            size_t array_size = 0;
            // not making a deep copy
            oc_rep_i_get_int_array(object, 7, &array, &array_size);
            if (array_size > 0) {
              // make the deep copy
              if ((g_gm_entries[index].ga_len > 0) &&
                  (&g_gm_entries[index].ga != NULL)) {
                int64_t *cur_arr = g_gm_entries[index].ga;
                if (cur_arr) {
                  free(cur_arr);
                }
                g_gm_entries[index].ga = NULL;
              }
              g_gm_entries[index].ga_len = (int)array_size;
              int64_t *new_array =
                (int64_t *)malloc(array_size * sizeof(uint64_t));
              if (new_array) {
                for (size_t i = 0; i < array_size; i++) {
                  new_array[i] = array[i];
                }
                g_gm_entries[index].ga = new_array;
              } else {
                OC_ERR("out of memory");
                return_status = OC_STATUS_INTERNAL_SERVER_ERROR;
              }
            }
          }
        } else if (object->type == OC_REP_INT) {
          if (object->iname == 116) {
            // dataType (116)
            PRINT("   dataType %d\n", (int)object->value.integer);
            g_gm_entries[index].dataType = (int)object->value.integer;
          }
        } else if (object->type == OC_REP_OBJECT) {
          // level of s
          s_object = object->value.object;
          int s_object_nr = object->iname;
          PRINT("  s_object_nr %d\n", s_object_nr);
          while (s_object) {
            if (s_object->type == OC_REP_BYTE_STRING) {
              if (s_object->iname == 107 && s_object_nr == 115) {
                // groupkey (115(s)::107)
                oc_free_string(&(g_gm_entries[index].groupKey));
                oc_new_string(&g_gm_entries[index].groupKey,
                              oc_string(s_object->value.string),
                              oc_string_len(s_object->value.string));
              }
            } else if (s_object->type == OC_REP_OBJECT) {
              sec_object = s_object->value.object;
              if (sec_object == NULL) {
                continue;
              }
              // get the number of the object which was the object in the
              // s_object.
              int sec_object_nr = s_object->iname;
              while (sec_object) {
                if (sec_object->type == OC_REP_BOOL) {
                  if (sec_object->iname == 97 && s_object_nr == 115 &&
                      sec_object_nr == 28) {
                    // 115:28:97
                    g_gm_entries[index].authentication =
                      s_object->value.boolean;
                  }
                  if (sec_object->iname == 99 && s_object_nr == 115 &&
                      sec_object_nr == 28) {
                    // 115:28:97
                    g_gm_entries[index].confidentiality =
                      s_object->value.boolean;
                  }
                } /* type */

                sec_object = sec_object->next;
              }
            }
            s_object = s_object->next;
          }
        }
        object = object->next;
      } // while (inner object)
    }   // case
    }   // switch (over all objects)
    rep = rep->next;
  }

  for (int i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    if (g_gm_entries[i].ga_len != 0) {
      oc_dump_group_mapping_table_entry(i);
    }
  }

  request->response->response_buffer->content_format = APPLICATION_CBOR;
  request->response->response_buffer->code = oc_status_code(return_status);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_fp_gm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/fp/gm", OC_IF_C | OC_IF_B, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_fp_gm_get_handler, 0, oc_core_fp_gm_post_handler,
    0, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_gm_x_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_gm_x_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  if (value >= oc_core_get_group_mapping_table_size()) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  // convert from [0,max-1] to [1-max]
  int index = value - 1;
  if (g_gm_entries[index].ga_len == 0) {
    // it is empty
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  // dataType (116) as first entry, since it is not zero
  oc_rep_i_set_int(root, 116, g_gm_entries[index].dataType);
  // id (0)
  oc_rep_i_set_int(root, 0, g_gm_entries[index].id);
  // ga (7) taking input of int64 array
  oc_rep_i_set_int_array(root, 7, g_gm_entries[index].ga,
                         g_gm_entries[index].ga_len);
  if (oc_string_len(g_gm_entries[index].groupKey) > 0) {
    // create s map (s)
    oc_rep_i_set_key(oc_rep_object(root), 115);
    oc_rep_start_object(oc_rep_object(root), s);
    // set groupKey (115:107)
    oc_rep_i_set_byte_string(s, 107, oc_string(g_gm_entries[index].groupKey),
                             oc_string_len(g_gm_entries[index].groupKey));
    // secSetting map (115:28)
    oc_rep_i_set_key(oc_rep_object(s), 28);
    oc_rep_start_object(oc_rep_object(s), secSettings);
    // add a (115:28:97)
    oc_rep_i_set_boolean(secSettings, 97,
                         (bool)g_gm_entries[index].authentication);
    // add c (115:28:99)
    oc_rep_i_set_boolean(secSettings, 99,
                         (bool)g_gm_entries[index].confidentiality);
    oc_rep_end_object(oc_rep_object(s), secSettings);
    oc_rep_end_object(oc_rep_object(root), s);
  }
  oc_rep_end_root_object();
  oc_send_cbor_response(request, OC_STATUS_OK);
  return;
}

static void
oc_core_fp_gm_x_del_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_gm_x_del_handler\n");

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  if (value >= oc_core_get_group_mapping_table_size()) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  int index = value - 1;
  // free the entries
  oc_free_string(&(g_gm_entries[index].groupKey));
  if (g_gm_entries[index].ga_len > 0) {
    uint64_t *cur_arr = g_gm_entries[index].ga;
    if (cur_arr) {
      free(cur_arr);
    }
    g_gm_entries[index].ga = NULL;
  }
  // set the indicator on zero
  g_gm_entries[index].ga_len = 0;
  PRINT("oc_core_fp_gm_x_del_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_gm_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/gm/*", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_gm_x_get_handler, 0, 0,
                            oc_core_fp_gm_x_del_handler, 0, 1, "urn:knx:if.c");
}

// -----------------------------------------------------------------------------

void
dump_fra(void)
{
  oc_storage_write(GM_STORE_FRA, (uint8_t *)&g_fra, sizeof(g_fra));
}

void
load_fra(void)
{
  int temp_size;

  temp_size = oc_storage_read(GM_STORE_FRA, (uint8_t *)&g_fra, sizeof(g_fra));
  // if (temp_size > 0) {
  //   device->ia = ia;
  //   PRINT("  ia (storage) %d\n", ia);
  // }
}

static void
oc_core_f_netip_fra_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_fra_get_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // size_t device_index = request->resource->device;
  // oc_device_info_t *device = oc_core_get_device_info(device_index);
  // if (device == NULL) {
  //   oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  //   return;
  // }
  //  Content-Format: "application/cbor"
  oc_rep_begin_root_object();
  oc_rep_i_set_int(root, 1, g_fra);
  oc_rep_end_root_object();
  oc_send_cbor_response(request, OC_STATUS_OK);
  PRINT("oc_core_f_netip_fra_get_handler - end\n");
}

static void
oc_core_f_netip_fra_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_fra_put_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_f_netip_fra_put_handler received (fra) : %d\n",
              (int)rep->value.integer);
        g_fra = (int)rep->value.integer;
        dump_fra();
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_CHANGED);
  PRINT("oc_core_f_netip_fra_put_handler - end\n");
}

static void
oc_create_f_netip_fra_resource(size_t device)
{
  OC_DBG("oc_create_f_netip_fra_resource\n");
  oc_resource_t *res = oc_new_resource("netip_fra", "/p/netip/fra", 2, 0);
  oc_resource_bind_resource_type(res, "urn:knx:dpa.11.96");
  oc_resource_bind_resource_type(res, "urn:knx:dpt.Scaling");
  oc_resource_bind_dpt(res, "");
  oc_resource_bind_content_type(res, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res, OC_IF_D + OC_IF_P); /* if.d + if.p*/
  oc_resource_set_function_block_instance(res, 1);             /* instance 1 */
  oc_resource_set_discoverable(res, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  /* oc_resource_set_periodic_observable(res, 1); */
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  oc_resource_set_observable(res, true);
  oc_resource_set_request_handler(res, OC_GET, oc_core_f_netip_fra_get_handler,
                                  NULL);
  oc_resource_set_request_handler(res, OC_PUT, oc_core_f_netip_fra_put_handler,
                                  NULL);
  oc_add_resource(res);
}

// -----------------------------------------------------------------------------

void
dump_tol(void)
{
  oc_storage_write(GM_STORE_TOL, (uint8_t *)&g_tol, sizeof(g_tol));
}

void
load_tol(void)
{
  int temp_size;

  temp_size = oc_storage_read(GM_STORE_TOL, (uint8_t *)&g_tol, sizeof(g_tol));
  // if (temp_size > 0) {
  //  device->ia = ia;
  //  PRINT("  ia (storage) %d\n", ia);
  //}
}

static void
oc_core_f_netip_tol_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_tol_get_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // size_t device_index = request->resource->device;
  // oc_device_info_t *device = oc_core_get_device_info(device_index);
  // if (device == NULL) {
  //   oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  //   return;
  // }
  //  Content-Format: "application/cbor"
  oc_rep_begin_root_object();
  oc_rep_i_set_int(root, 1, g_tol);
  oc_rep_end_root_object();
  oc_send_cbor_response(request, OC_STATUS_OK);
  PRINT("oc_core_f_netip_tol_get_handler - end\n");
}

static void
oc_core_f_netip_tol_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_tol_put_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_f_netip_tol_put_handler received (tol) : %d\n",
              (int)rep->value.integer);
        g_tol = (int)rep->value.integer;
        dump_tol();
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_CHANGED);
  PRINT("oc_core_f_netip_tol_put_handler - end\n");
}

static void
oc_create_f_netip_tol_resource(size_t device)
{
  OC_DBG("oc_create_f_netip_tol_resource\n");
  oc_resource_t *res = oc_new_resource("netip_tol", "/p/netip/tol", 2, 0);
  oc_resource_bind_resource_type(res, "urn:knx:dpa.11.95");
  oc_resource_bind_resource_type(res, "urn:knx:dpt.timePeriodMsec");
  oc_resource_bind_dpt(res, "");
  oc_resource_bind_content_type(res, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res, OC_IF_D + OC_IF_P); /* if.d +  if.p*/
  oc_resource_set_function_block_instance(res, 1);             /* instance 1 */
  oc_resource_set_discoverable(res, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  /* oc_resource_set_periodic_observable(res, 1); */
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  oc_resource_set_observable(res, true);
  oc_resource_set_request_handler(res, OC_GET, oc_core_f_netip_tol_get_handler,
                                  NULL);
  oc_resource_set_request_handler(res, OC_PUT, oc_core_f_netip_tol_put_handler,
                                  NULL);
  oc_add_resource(res);
}

// -----------------------------------------------------------------------------

void
dump_key(void)
{
  int key_size = oc_string_len(g_key);
  // oc_storage_write(GM_STORE_KEY, (uint8_t *)&key_size, sizeof(key_size));
  int written = oc_storage_write(GM_STORE_KEY, oc_string(g_key), key_size);
  if (written != key_size) {
    PRINT("dump_key %d %d\n", key_size, written);
  }
}

void
load_key(void)
{
  int temp_size;
  int key_size;
  char tempstring[100];
  temp_size = oc_storage_read(GM_STORE_KEY, (uint8_t *)&tempstring, 99);
  if (temp_size > 1) {
    tempstring[temp_size] = 0;
    oc_new_string(&g_key, tempstring, temp_size);
  }
}

static void
oc_core_f_netip_key_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_key_put_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_BYTE_STRING) {
      if (rep->iname == 1) {
        oc_free_string(&(g_key));
        oc_new_string(&g_key, oc_string(rep->value.string),
                      oc_string_len(rep->value.string));
        dump_key();
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_CHANGED);
  PRINT("oc_core_f_netip_key_put_handler - end\n");
}

static void
oc_create_f_netip_key_resource(size_t device)
{
  OC_DBG("oc_create_f_netip_key_resource\n");
  oc_resource_t *res = oc_new_resource("netip_key", "/p/netip/key", 2, 0);
  oc_resource_bind_resource_type(res, "urn:knx:dpa.11.91");
  oc_resource_bind_resource_type(res, "urn:knx:dpt.varOctet");
  oc_resource_bind_dpt(res, "");
  oc_resource_bind_content_type(res, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res, OC_IF_D + OC_IF_P); /* if.d + if.p */
  oc_resource_set_function_block_instance(res, 1);             /* instance 1 */
  oc_resource_set_discoverable(res, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  /* oc_resource_set_periodic_observable(res, 1); */
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  oc_resource_set_observable(res, true);
  // no GET handler
  // oc_resource_set_request_handler(res, OC_GET,
  // oc_core_f_netip_ttl_get_handler,
  //                                NULL);
  oc_resource_set_request_handler(res, OC_PUT, oc_core_f_netip_key_put_handler,
                                  NULL);
  oc_add_resource(res);
}

// -----------------------------------------------------------------------------

void
dump_ttl(void)
{
  oc_storage_write(GM_STORE_TTL, (uint8_t *)&g_ttl, sizeof(g_ttl));
}

void
load_ttl(void)
{
  int temp_size;

  temp_size = oc_storage_read(GM_STORE_TTL, (uint8_t *)&g_ttl, sizeof(g_ttl));
  // if (temp_size > 0) {
  //  device->ia = ia;
  //  PRINT("  ia (storage) %d\n", ia);
  //}
}

static void
oc_core_f_netip_ttl_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_ttl_get_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // size_t device_index = request->resource->device;
  // oc_device_info_t *device = oc_core_get_device_info(device_index);
  // if (device == NULL) {
  //   oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  //   return;
  // }
  // Content-Format: "application/cbor"
  oc_rep_begin_root_object();
  oc_rep_i_set_int(root, 1, g_ttl);
  oc_rep_end_root_object();
  oc_send_cbor_response(request, OC_STATUS_OK);
  PRINT("oc_core_f_netip_ttl_get_handler - end\n");
}

static void
oc_core_f_netip_ttl_put_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_ttl_put_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_f_netip_ttl_put_handler received (ttl) : %d\n",
              (int)rep->value.integer);
        g_ttl = (int)rep->value.integer;
        dump_ttl();
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_CHANGED);
  PRINT("oc_core_f_netip_ttl_put_handler - end\n");
}

static void
oc_create_f_netip_ttl_resource(size_t device)
{
  OC_DBG("oc_create_f_netip_ttl_resource\n");
  oc_resource_t *res = oc_new_resource("netip_ttl", "/p/netip/ttl", 2, 0);
  oc_resource_bind_resource_type(res, "urn:knx:dpa.11.67");
  oc_resource_bind_resource_type(res, "urn:knx:dpt.value1Ucount");
  oc_resource_bind_dpt(res, "");
  oc_resource_bind_content_type(res, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res, OC_IF_D + OC_IF_P); /* if.d + if.p */
  oc_resource_set_function_block_instance(res, 1);             /* instance 1 */
  oc_resource_set_discoverable(res, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  /* oc_resource_set_periodic_observable(res, 1); */
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  oc_resource_set_observable(res, true);
  oc_resource_set_request_handler(res, OC_GET, oc_core_f_netip_ttl_get_handler,
                                  NULL);
  oc_resource_set_request_handler(res, OC_PUT, oc_core_f_netip_ttl_put_handler,
                                  NULL);
  oc_add_resource(res);
}

// -----------------------------------------------------------------------------

void
dump_mcast(void)
{
  oc_storage_write(GM_STORE_MCAST, (uint8_t *)&g_mcast, sizeof(g_mcast));
}

void
load_mcast(void)
{
  int temp_size;

  temp_size =
    oc_storage_read(GM_STORE_MCAST, (uint8_t *)&g_mcast, sizeof(g_mcast));
}

static void
oc_core_f_netip_mcast_get_handler(oc_request_t *request,
                                  oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_mcast_get_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // size_t device_index = request->resource->device;
  // oc_device_info_t *device = oc_core_get_device_info(device_index);
  // if (device == NULL) {
  //   oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  //   return;
  // }
  //  Content-Format: "application/cbor"
  oc_rep_begin_root_object();
  oc_rep_i_set_int(root, 1, g_mcast);
  oc_rep_end_root_object();
  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_f_netip_mcast_get_handler - end\n");
}

static void
oc_core_f_netip_mcast_put_handler(oc_request_t *request,
                                  oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_f_netip_mcast_put_handler\n");

  /* check if the accept header is cbor */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_f_netip_mcast_put_handler (mcast) : %d\n",
              (int)rep->value.integer);
        g_mcast = (int)rep->value.integer;
        dump_mcast();
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_CHANGED);
  PRINT("oc_core_f_netip_mcast_put_handler - end\n");
}

static void
oc_create_f_netip_mcast_resource(size_t device)
{
  OC_DBG("oc_create_f_netip_mcast_resource\n");
  oc_resource_t *res = oc_new_resource("netip_mcast", "/p/netip/mcast", 2, 0);
  oc_resource_bind_resource_type(res, "urn:knx:dpa.11.66");
  oc_resource_bind_resource_type(res, "urn:knx:dpt.IPV4");
  oc_resource_bind_dpt(res, "");
  oc_resource_bind_content_type(res, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res, OC_IF_D + OC_IF_P); /* if.d + if.p */
  oc_resource_set_function_block_instance(res, 1);             /* instance 1 */
  oc_resource_set_discoverable(res, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  /* oc_resource_set_periodic_observable(res, 1); */
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  oc_resource_set_observable(res, true);
  oc_resource_set_request_handler(res, OC_GET,
                                  oc_core_f_netip_mcast_get_handler, NULL);
  oc_resource_set_request_handler(res, OC_PUT,
                                  oc_core_f_netip_mcast_put_handler, NULL);
  oc_add_resource(res);
}

// -----------------------------------------------------------------------------

// to be removed
void
oc_create_f_netip_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_f_netip_resource\n");
  oc_core_populate_resource(resource_idx, device, "/p/netip", OC_IF_D,
                            APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                            oc_core_f_netip_get_handler, 0, 0, 0, 0, 1,
                            "urn:knx:fb.11");
}

// -----------------------------------------------------------------------------

#endif /* OC_IOT_ROUTER */

// to be removed
void
oc_core_f_netip_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  bool ps_exists;
  bool total_exists;
  PRINT("oc_core_f_netip_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /* example entry: </f/netip/xxx>;ct=60 (cbor)*/

#ifdef OC_IOT_ROUTER

  // handle query parameters: l=ps l=total
  if (check_if_query_l_exist(request, &ps_exists, &total_exists)) {
    // example : < / fp / r / ? l = total>; total = 22; ps = 5

    length = oc_frame_query_l("/fp/gm", ps_exists, total_exists);
    response_length += length;
    if (ps_exists) {
      length = oc_rep_add_line_to_buffer(";ps=");
      response_length += length;
      length = oc_frame_integer(5);
      response_length += length;
    }
    if (total_exists) {
      length = oc_rep_add_line_to_buffer(";total=");
      response_length += length;
      length = oc_frame_integer(5);
      response_length += length;
    }
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }

  length = oc_rep_add_line_to_buffer("</p/netip/mcast>");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";rt=\":dpa.11.66 :dpt.IPv4\"");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";ct=60,\n");
  response_length += length;

  length = oc_rep_add_line_to_buffer("</p/netip/ttl>");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";rt=\":dpa.11.67 :dpt.value1Ucount\"");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";ct=60,\n");
  response_length += length;

  length = oc_rep_add_line_to_buffer("</p/netip/key>");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";rt=\":dpa.11.91 :dpt.varOctet\"");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";ct=60,\n");
  response_length += length;

  length = oc_rep_add_line_to_buffer("</p/netip/tol>");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";rt=\":dpa.11.95 :dpt.timePeriodMsec\"");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";ct=60,\n");
  response_length += length;

  length = oc_rep_add_line_to_buffer("</p/netip/fra>");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";rt=\":dpa.11.96 :dpt.scaling\"");
  response_length += length;
  length = oc_rep_add_line_to_buffer(";ct=60");
  response_length += length;

#endif /* OC_IOT_ROUTER */

  if (response_length > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_f_netip_get_handler - end\n");
}

void
oc_create_knx_iot_router_resources(size_t device_index)
{
  (void)device_index;
#ifdef OC_IOT_ROUTER
  OC_DBG("oc_create_knx_gm_resources");
  // creating the resources
  oc_create_fp_gm_resource(OC_KNX_FP_GM, device_index);
  oc_create_fp_gm_x_resource(OC_KNX_FP_GM_X, device_index);

  // loading the stored state
  oc_load_group_mapping_table();
  load_ttl();
  load_tol();
  load_fra();
  load_mcast();
  load_key();
#endif /* OC_IOT_ROUTER */
}

void
oc_create_iot_router_functional_block(size_t device_index)
{
  (void)device_index;
#ifdef OC_IOT_ROUTER
  oc_create_f_netip_mcast_resource(device_index);
  oc_create_f_netip_ttl_resource(device_index);
  oc_create_f_netip_tol_resource(device_index);
  oc_create_f_netip_key_resource(device_index);
  oc_create_f_netip_fra_resource(device_index);
#endif /* OC_IOT_ROUTER */
}

void
oc_delete_group_mapping_table()
{

#ifdef OC_IOT_ROUTER
  PRINT("Deleting Group Mapping Table from Persistent storage\n");
  for (int i = 0; i < oc_core_get_group_mapping_table_size(); i++) {
    oc_delete_group_mapping_table_entry(i);
    oc_print_group_mapping_table_entry(i);
  }
#endif /* OC_IOT_ROUTER */
}

oc_group_mapping_table_t *
oc_get_group_mapping_entry(size_t device_index, int index)
{
  (void)device_index;

  if (index < 0) {
    return NULL;
  }
  if (index >= oc_core_get_group_mapping_table_size()) {
    return NULL;
  }

#ifdef OC_IOT_ROUTER
  return &g_gm_entries[index];
#else
  return NULL;
#endif
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

static oc_gateway_t app_gateway = { NULL, NULL };

int
oc_set_gateway_cb(oc_gateway_s_mode_cb_t cb, void *data)
{
  app_gateway.cb = cb;
  app_gateway.data = data;

  return 0;
}

oc_gateway_t *
oc_get_gateway_cb(void)
{
  return &app_gateway;
}
