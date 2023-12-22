/*
 // Copyright (c) 2021,2023 Cascoda Ltd
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
#include "api/oc_knx_fp.h"
#include "oc_discovery.h"
#include "oc_core_res.h"
#include "oc_helpers.h"
#include "oc_knx_helpers.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define TAGS_AS_STRINGS

#define GOT_STORE "GOT_STORE"
#define GPT_STORE "GPUBT_STORE"
#define GRT_STORE "GRECT_STORE"

#ifndef GOT_MAX_ENTRIES
#define GOT_MAX_ENTRIES 20
#endif
static oc_group_object_table_t g_got[GOT_MAX_ENTRIES];

#ifdef OC_PUBLISHER_TABLE
#ifndef GPT_MAX_ENTRIES
#define GPT_MAX_ENTRIES 20
#endif
static oc_group_rp_table_t g_gpt[GPT_MAX_ENTRIES];
#endif /* OC_PUBLISHER_TABLE */

#ifndef GRT_MAX_ENTRIES
#define GRT_MAX_ENTRIES 20
#endif
static oc_group_rp_table_t g_grt[GRT_MAX_ENTRIES];

// -----------------------------------------------------------------------------

static void oc_print_group_rp_table_entry(int entry, char *Store,
                                          oc_group_rp_table_t *rp_table,
                                          int max_size);

static void oc_print_reduced_group_rp_table_entry(int entry, char *Store,
                                                  oc_group_rp_table_t *rp_table,
                                                  int max_size);

static void oc_dump_group_rp_table_entry(int entry, char *Store,
                                         oc_group_rp_table_t *rp_table,
                                         int max_size);

static int oc_core_find_index_in_rp_table_from_id(int id,
                                                  oc_group_rp_table_t *rp_table,
                                                  int max_size);

int find_empty_slot_in_rp_table(int id, oc_group_rp_table_t *rp_table,
                                int max_size);

static int oc_core_find_used_nr_in_rp_table(oc_group_rp_table_t *rp_table,
                                            int max_size);

static void oc_delete_group_rp_table_entry(int entry, char *Store,
                                           oc_group_rp_table_t *rp_table,
                                           int max_size);

// -----------------------------------------------------------------------------

int
oc_print_reduced_group_publisher_table(void)
{
#ifdef OC_PUBLISHER_TABLE
  for (int i = 0; i < oc_core_get_publisher_table_size(); i++) {
    oc_print_reduced_group_rp_table_entry(i, GPT_STORE, g_gpt,
                                          oc_core_get_publisher_table_size());
  }
#endif
  return 0;
}

int
oc_print_reduced_group_recipient_table(void)
{
  for (int i = 0; i < oc_core_get_recipient_table_size(); i++) {
    oc_print_reduced_group_rp_table_entry(i, GRT_STORE, g_grt,
                                          oc_core_get_recipient_table_size());
  }
  return 0;
}

int
oc_table_find_id_from_rep(oc_rep_t *object)
{
  int id = -1;
  while (object != NULL) {
    switch (object->type) {
    case OC_REP_INT: {
      if (oc_string_len(object->name) == 0 && object->iname == 0) {
        id = (int)object->value.integer;
        PRINT(" oc_table_find_id_from_rep id=%d \n", id);
        return id;
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    } /* switch */
    object = object->next;
  } /* while */
  PRINT("  oc_table_find_id_from_rep ERR: id=%d \n", id);
  return id;
}

// -----------------------------------------------------------------------------

int
find_empty_slot_in_group_object_table(int id)
{
  int index = -1;
  if (id < 0) {
    /* should be a positive number */
    return index;
  }
  index = oc_core_find_index_in_group_object_table_from_id(id);
  if (index > -1) {
    /* overwrite existing index */
    return index;
  }
  /* empty slot */
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    if (g_got[i].id == -1) {
      return i;
    }
  }
  return -1;
}

int
oc_core_set_group_object_table(int index, oc_group_object_table_t entry)
{
  if (index >= oc_core_get_group_object_table_total_size()) {
    OC_ERR("index too large index:%d %d", index,
           oc_core_get_group_object_table_total_size());
  }
  g_got[index].cflags = entry.cflags;
  g_got[index].id = entry.id;

  oc_free_string(&g_got[index].href);
  oc_new_string(&g_got[index].href, oc_string(entry.href),
                oc_string_len(entry.href));
  /* copy the ga array */
  g_got[index].ga_len = 0;
  uint32_t *new_array = (uint32_t *)malloc(entry.ga_len * sizeof(uint32_t));

  if ((new_array != NULL) && (entry.id > -1)) {
    for (int i = 0; i < entry.ga_len; i++) {
#pragma warning(suppress : 6386)
      new_array[i] = entry.ga[i];
    }
    if (g_got[index].ga != 0) {
      free(g_got[index].ga);
    }
    g_got[index].ga_len = entry.ga_len;
    g_got[index].ga = new_array;
  }
  return 0;
}

int
oc_core_get_group_object_table_total_size()
{
  return GOT_MAX_ENTRIES;
}

oc_group_object_table_t *
oc_core_get_group_object_table_entry(int index)
{
  if (index < 0) {
    return NULL;
  }
  if (index >= GOT_MAX_ENTRIES) {
    return NULL;
  }
  return &g_got[index];
}

int
oc_core_find_index_in_group_object_table_from_id(int id)
{
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    if (g_got[i].id == id) {
      return i;
    }
  }
  return -1;
}

int
oc_core_find_group_object_table_index(uint32_t group_address)
{
  int i, j;
  for (i = 0; i < GOT_MAX_ENTRIES; i++) {

    if (g_got[i].id > -1) {
      for (j = 0; j < g_got[i].ga_len; j++) {
        if (group_address == g_got[i].ga[j]) {
          return i;
        }
      }
    }
  }
  return -1;
}

int
oc_core_find_next_group_object_table_index(uint32_t group_address,
                                           int cur_index)
{
  if (cur_index == -1) {
    return -1;
  }

  int i, j;
  for (i = cur_index + 1; i < GOT_MAX_ENTRIES; i++) {

    if (g_got[i].id > -1) {
      for (j = 0; j < g_got[i].ga_len; j++) {
        if (group_address == g_got[i].ga[j]) {
          return i;
        }
      }
    }
  }
  return -1;
}

oc_string_t
oc_core_find_group_object_table_url_from_index(int index)
{
  // if (index < GOT_MAX_ENTRIES) {
  return g_got[index].href;
  //}
  // return oc_string_t();
}

oc_cflag_mask_t
oc_core_group_object_table_cflag_entries(int index)
{
  if (index < GOT_MAX_ENTRIES) {
    return g_got[index].cflags;
  }
  return 0;
}

int
oc_core_find_group_object_table_number_group_entries(int index)
{
  if (index < GOT_MAX_ENTRIES) {
    return g_got[index].ga_len;
  }
  return 0;
}

int
oc_core_find_group_object_table_group_entry(int index, int entry)
{
  if (index < GOT_MAX_ENTRIES) {
    if (entry < g_got[index].ga_len) {
      return g_got[index].ga[entry];
    }
  }
  return 0;
}

int
oc_core_find_group_object_table_url(const char *url)
{
  int i;
  size_t url_len = strlen(url);
  for (i = 0; i < GOT_MAX_ENTRIES; i++) {
    if ((url_len == oc_string_len(g_got[i].href)) &&
        (strcmp(url, oc_string(g_got[i].href)) == 0)) {
      return i;
    }
  }
  return -1;
}

int
oc_core_find_next_group_object_table_url(const char *url, int cur_index)
{
  if (cur_index == -1) {
    return -1;
  }

  int i;
  size_t url_len = strlen(url);
  for (i = cur_index + 1; i < GOT_MAX_ENTRIES; i++) {
    if ((url_len == oc_string_len(g_got[i].href)) &&
        (strcmp(url, oc_string(g_got[i].href)) == 0)) {
      return i;
    }
  }
  return -1;
}

int
oc_core_find_nr_used_in_group_object_table()
{
  int counter = 0;
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    if (g_got[i].id > -1) {
      counter++;
    }
  }
  return counter;
}

#ifdef OC_PUBLISHER_TABLE
int
oc_core_find_nr_used_in_group_publisher_table()
{
  int counter = 0;
  for (int i = 0; i < GPT_MAX_ENTRIES; i++) {
    if (g_gpt[i].id > -1) {
      counter++;
    }
  }
  return counter;
}
#endif /* OC_PUBLISHER_TABLE */

int
oc_core_find_nr_used_in_group_recipient_table()
{
  int counter = 0;
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    if (g_grt[i].id > -1) {
      counter++;
    }
  }
  return counter;
}

bool
oc_belongs_href_to_resource(oc_string_t href, bool discoverable,
                            size_t device_index)
{

  const oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (discoverable) {
      if (resource->device != device_index ||
          !(resource->properties & OC_DISCOVERABLE)) {
        continue;
      }
    }
    if (oc_url_cmp(href, resource->uri) == 0) {
      return true;
    }
  }

  return false;
}

static void
oc_core_fp_g_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  int matches = 0;

  bool ps_exists = false;
  bool total_exists = false;
  int total = oc_core_find_nr_used_in_group_object_table();
  int first_entry = 0;              // inclusive
  int last_entry = GOT_MAX_ENTRIES; // exclusive
  // int query_ps = -1;
  int query_pn = -1;
  bool more_request_needed =
    false; // If more requests (pages) are needed to get the full list

  PRINT("oc_core_fp_g_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /fp/g > l = total>;total=22;ps=5
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

  if (total > first_entry + PAGE_SIZE) {
    more_request_needed = true;
  }

  /* example entry: </fp/g/1>;ct=60   (cbor)*/
  for (i = first_entry; i < last_entry; i++) {

    if (g_got[i].id > -1) {
      // index  in use
      PRINT("  . adding %d\n", i);
      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("</fp/g/");
      response_length += length;
      char string[10];
      sprintf((char *)&string, "%d>", g_got[i].id);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=60");
      response_length += length;

      matches++;

      if (matches >= PAGE_SIZE) {
        break;
      }
    }
  }

  if (more_request_needed) {
    int next_page_num = query_pn > -1 ? query_pn + 1 : 1;
    response_length +=
      add_next_page_indicator(oc_string(request->resource->uri), next_page_num);
  }
  oc_send_linkformat_response(request, OC_STATUS_OK, response_length);

  PRINT("oc_core_fp_g_get_handler - end\n");
}

static bool
oc_fp_g_check_and_save(int index, size_t device_index, bool status_ok)
{
  bool do_save = true;
  if (status_ok == false) {
    return status_ok;
  }

  // do additional sanity checks
  if (g_got[index].ga_len == 0) {
    do_save = false;
    OC_ERR("  index %d no groups %d", index, g_got[index].ga_len);
  }
  if (g_got[index].cflags == 0) {
    do_save = false;
    OC_ERR("  index %d no cflags set %d", index, g_got[index].cflags);
  }
  if (oc_string_len(g_got[index].href) > OC_MAX_URL_LENGTH) {
    do_save = false;
    OC_ERR("  index %d href is longer than %d", index, (int)OC_MAX_URL_LENGTH);
  }
  if (oc_belongs_href_to_resource(g_got[index].href, true, device_index) ==
      false) {
    do_save = false;
    OC_ERR("  index %d href '%s' does not belong to device\n", index,
           oc_string_checked(g_got[index].href));
  }

  oc_print_group_object_table_entry(index);
  if (do_save) {
    oc_dump_group_object_table_entry(index);
  } else {
    OC_DBG("Deleting Index %d", index);
    oc_delete_group_object_table_entry(index);
    oc_dump_group_object_table_entry(index);
    status_ok = false;
  }
  return status_ok;
}

static void
oc_core_fp_g_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  bool status_ok = true;
  oc_status_t return_status = OC_STATUS_BAD_REQUEST;

  PRINT("oc_core_fp_g_post_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_response_no_format(request, OC_STATUS_METHOD_NOT_ALLOWED);
    return;
  }

  /* debugging info */
  oc_print_rep_as_json(request->request_payload, true);

  int index = -1;
  int id;
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {

      // find id and the storage index for this object
      oc_rep_t *object = rep->value.object;
      id = oc_table_find_id_from_rep(object);
      if (id == -1) {
        OC_ERR("  ERROR id %d", index);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      index = oc_core_find_index_in_group_object_table_from_id(id);
      if (index != -1) {
        // index already in use, so it will be changed
        return_status = OC_STATUS_CHANGED;
      } else {
        // no index, so we will create one
        return_status = OC_STATUS_CREATED;
      }
      index = find_empty_slot_in_group_object_table(id);
      if (index == -1) {
        OC_ERR("  ERROR index %d", index);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      }

      bool id_only = true;
      int mandatory_items =
        0; // Needs to be 4 for creating new entry, i.e. id, ga, cflag & href
      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {
        case OC_REP_STRING: {
          id_only = false;
          if (object->iname == 11) {
            // href (11)
            mandatory_items++;
            oc_free_string(&g_got[index].href);
            oc_new_string(&g_got[index].href, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;
        case OC_REP_INT: {
          if (oc_string_len(object->name) == 0 && object->iname == 0) {
            // id (0)
            mandatory_items++;
            g_got[index].id = (int)object->value.integer;
          }
          if (oc_string_len(object->name) == 0 && object->iname == 8) {
            // cflags (8)
            mandatory_items++;
            id_only = false;
            g_got[index].cflags = object->value.integer;
          }
        } break;
        case OC_REP_INT_ARRAY: {
          id_only = false;
          if (object->iname == 8) {
            // cflags (8)
            mandatory_items++;
            g_got[index].cflags = OC_CFLAG_NONE;
            int64_t *arr = oc_int_array(object->value.array);
            for (int i = 0; i < (int)oc_int_array_size(object->value.array);
                 i++) {
              if (arr[i] == 1) {
                g_got[index].cflags += OC_CFLAG_READ;
              } else if (arr[i] == 2) {
                g_got[index].cflags += OC_CFLAG_WRITE;
              } else if (arr[i] == 3) {
                g_got[index].cflags += OC_CFLAG_TRANSMISSION;
              } else if (arr[i] == 4) {
                g_got[index].cflags += OC_CFLAG_UPDATE;
              } else if (arr[i] == 5) {
                g_got[index].cflags += OC_CFLAG_INIT;
              }
            }
          }
          if (object->iname == 7) {
            // ga (7)
            mandatory_items++;
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            uint32_t *new_array =
              (uint32_t *)malloc(array_size * sizeof(uint32_t));
            if (new_array) {
              for (int i = 0; i < array_size; i++) {
                new_array[i] = (uint32_t)arr[i];
              }
              if (g_got[index].ga != 0) {
                free(g_got[index].ga);
              }
              g_got[index].ga_len = array_size;
              g_got[index].ga = new_array;
            } else {
              OC_ERR("out of memory");
              return_status = OC_STATUS_INTERNAL_SERVER_ERROR;
            }
          }
        } break;
        default:
          break;
        } // switch

        object = object->next;
      } // next object in array
      if (id_only) {
        PRINT("  only found id in request, deleting entry at index: %d\n",
              index);
        oc_delete_group_object_table_entry(index);
      } else if (return_status == OC_STATUS_CREATED && mandatory_items != 4) {
        PRINT("Mandatory items missing!\n");
        oc_delete_group_object_table_entry(index);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      } else {
        PRINT("  storing at index: %d\n", index);
        status_ok = oc_fp_g_check_and_save(index, device_index, status_ok);
      }
    }
    default:
      break;
    } // object level
    rep = rep->next;
  } // top level

  PRINT("oc_core_fp_g_post_handler status=%d - end\n", (int)status_ok);
  if (status_ok) {
    oc_knx_increase_fingerprint();
    oc_send_response_no_format(request, return_status);
    return;
  }
  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fp_g, knx_fp_g_x, 0, "/fp/g",
                                     OC_IF_C | OC_IF_B, APPLICATION_CBOR,
                                     OC_DISCOVERABLE, oc_core_fp_g_get_handler,
                                     0, oc_core_fp_g_post_handler, 0, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:if.c");

void
oc_create_fp_g_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_g_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/g", OC_IF_C | OC_IF_B,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_g_get_handler, 0,
                            oc_core_fp_g_post_handler, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_g_x_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_g_x_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index = oc_core_find_index_in_group_object_table_from_id(id);
  PRINT("  id=%d index = %d\n", id, index);
  if (index == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }

  if (g_got[index].id == -1) {
    // it is empty
    oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_got[index].id);
  // href- 11
  oc_rep_i_set_text_string(root, 11, oc_string(g_got[index].href));
  // ga - 7
  oc_rep_i_set_int_array(root, 7, g_got[index].ga, g_got[index].ga_len);
  // cflags 8
  oc_rep_i_set_int(root, 8, g_got[index].cflags);
  oc_rep_end_root_object();
  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_fp_g_x_get_handler - end\n");
  return;
}

static void
oc_core_fp_g_x_del_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_g_x_del_handler\n");

  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index = oc_core_find_index_in_group_object_table_from_id(id);
  PRINT("  id=%d index = %d\n", id, index);

  PRINT(" deleting %d\n", index);

  if (index == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }

  // delete the entry
  oc_delete_group_object_table_entry(index);

  // make the deletion persistent
  oc_dump_group_object_table_entry(index);
  // update the finger print
  oc_knx_increase_fingerprint();

  PRINT("oc_core_fp_g_x_del_handler - end\n");
  oc_send_response_no_format(request, OC_STATUS_DELETED);
}

#ifdef OC_PUBLISHER_TABLE
OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fp_g_x, knx_fp_p, 0, "/fp/g/*",
                                     OC_IF_D | OC_IF_C, APPLICATION_CBOR,
                                     OC_DISCOVERABLE,
                                     oc_core_fp_g_x_get_handler, 0, 0,
                                     oc_core_fp_g_x_del_handler, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:if.c");
#else
OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fp_g_x, knx_fp_r, 0, "/fp/g/*",
                                     OC_IF_D | OC_IF_C, APPLICATION_CBOR,
                                     OC_DISCOVERABLE,
                                     oc_core_fp_g_x_get_handler, 0, 0,
                                     oc_core_fp_g_x_del_handler, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:if.c");
#endif
void
oc_create_fp_g_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_g_x_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/g/*", OC_IF_D | OC_IF_C,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_g_x_get_handler, 0, 0,
                            oc_core_fp_g_x_del_handler, 1, "urn:knx:if.c");
}

// ---------------------PUBLISHER------------------------------------------

#ifdef OC_PUBLISHER_TABLE

int
oc_core_find_publisher_table_index(uint32_t group_address)
{
  int i, j;
  for (i = 0; i < GPT_MAX_ENTRIES; i++) {

    if (g_gpt[i].id > -1) {
      for (j = 0; j < g_gpt[i].ga_len; j++) {
        if (group_address == g_gpt[i].ga[j]) {
          return i;
        }
      }
    }
  }
  return -1;
}

oc_string_t
oc_core_find_publisher_table_url_from_index(int index)
{
  return g_gpt[index].url;
}

static void
oc_core_fp_p_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  int matches = 0;

  bool ps_exists = false;
  bool total_exists = false;
  int total = oc_core_find_nr_used_in_group_publisher_table();
  int first_entry = 0;              // inclusive
  int last_entry = GPT_MAX_ENTRIES; // exclusive
  // int query_ps = -1;
  int query_pn = -1;
  bool more_request_needed =
    false; // If more requests (pages) are needed to get the full list

  PRINT("oc_core_fp_p_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /fp/p > l = total>;total=22;ps=5
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

  if (total > first_entry + PAGE_SIZE) {
    more_request_needed = true;
  }

  /* example entry: </fp/p/1>;ct=60 */
  for (i = first_entry; i < last_entry; i++) {

    if (g_gpt[i].id > -1) {
      // index  in use

      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("</fp/p/");
      response_length += length;
      char string[10];
      sprintf((char *)&string, "%d>", g_gpt[i].id);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=60");
      response_length += length;

      matches++;

      if (matches >= PAGE_SIZE) {
        break;
      }
    }
  }

  if (more_request_needed) {
    int next_page_num = query_pn > -1 ? query_pn + 1 : 1;
    response_length +=
      add_next_page_indicator(oc_string(request->resource->uri), next_page_num);
  }
  oc_send_linkformat_response(request, OC_STATUS_OK, response_length);

  PRINT("oc_core_fp_p_get_handler - end\n");
}

static void
oc_core_fp_p_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  oc_status_t return_status = OC_STATUS_BAD_REQUEST;

  PRINT("oc_core_fp_p_post_handler\n");

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_response_no_format(request, OC_STATUS_METHOD_NOT_ALLOWED);
    return;
  }

  oc_print_rep_as_json(request->request_payload, true);

  int index = -1;
  int id;
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      /* find the storage index, e.g. for this object */
      oc_rep_t *object = rep->value.object;
      id = oc_table_find_id_from_rep(object);
      index = oc_core_find_index_in_rp_table_from_id(
        id, g_gpt, oc_core_get_publisher_table_size());
      if (index != -1) {
        // index already in use, so it will be changed
        return_status = OC_STATUS_CHANGED;
      } else {
        // no index, so we will create one
        return_status = OC_STATUS_CREATED;
      }
      index = find_empty_slot_in_rp_table(id, g_gpt,
                                          oc_core_get_publisher_table_size());
      if (index == -1) {
        PRINT("  ERROR index %d\n", index);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      g_gpt[index].id = id;

      bool id_only = true;
      int mandatory_items = 0; // Needs to be 2 for creating entry, i.e. id & ga
      bool identifier_exists = false; // Once of ia, grpid or url
      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {
        case OC_REP_INT: {
          if (object->iname == 0) {
            mandatory_items++;
          } else {
            id_only = false;
          }
          if (object->iname == 12) {
            identifier_exists = true;
            g_gpt[index].ia = (int)object->value.integer;
          }
          if (object->iname == 13) {
            identifier_exists = true;
            g_gpt[index].grpid = (uint32_t)object->value.integer;
          }
          if (object->iname == 26) {
            g_gpt[index].iid = object->value.integer;
          }
          if (object->iname == 25) {
            g_gpt[index].fid = object->value.integer;
          }
        } break;
        case OC_REP_STRING: {
          id_only = false;
          if (object->iname == 112) {
            oc_free_string(&g_gpt[index].path);
            oc_new_string(&g_gpt[index].path, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (object->iname == 10) {
            identifier_exists = true;
            oc_free_string(&g_gpt[index].url);
            oc_new_string(&g_gpt[index].url, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (object->iname == 14) {
            oc_free_string(&g_gpt[index].at);
            oc_new_string(&g_gpt[index].at, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;
        case OC_REP_INT_ARRAY: {
          id_only = false;
          if (object->iname == 7) {
            mandatory_items++;
            // g_got[index].id = object->value.integer;
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            uint32_t *new_array =
              (uint32_t *)malloc(array_size * sizeof(uint32_t));
            if (new_array) {
              for (int i = 0; i < array_size; i++) {
                new_array[i] = (uint32_t)arr[i];
              }
              if (g_gpt[index].ga != 0) {
                free(g_gpt[index].ga);
              }
              g_gpt[index].ga_len = array_size;
              g_gpt[index].ga = new_array;
            } else {
              OC_ERR("out of memory");
              return_status = OC_STATUS_INTERNAL_SERVER_ERROR;
            }
          }
        } break;
        case OC_REP_NIL: {
          if (object->iname == 7) {
            mandatory_items++;
          }
        } break;
        default:
          break;
        }
        object = object->next;
      }
      if (id_only) {
        PRINT("  only found id in request, deleting entry at index: %d\n",
              index);
        oc_delete_group_rp_table_entry(index, GPT_STORE, g_gpt,
                                       GPT_MAX_ENTRIES);
      } else if (return_status == OC_STATUS_CREATED &&
                 (mandatory_items != 2 || !identifier_exists)) {
        PRINT("Mandatory items missing!\n");
        oc_delete_group_rp_table_entry(index, GPT_STORE, g_gpt,
                                       GPT_MAX_ENTRIES);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      } else {
        oc_print_group_rp_table_entry(index, GPT_STORE, g_gpt,
                                      oc_core_get_publisher_table_size());
        bool do_save = true;
        if (oc_string_len(g_gpt[index].url) > OC_MAX_URL_LENGTH) {
          // do_save = false;
          OC_ERR("  url is longer than %d \n", (int)OC_MAX_URL_LENGTH);
        }
        if (oc_string_len(g_gpt[index].path) > OC_MAX_URL_LENGTH) {
          // do_save = false;
          OC_ERR("  path is longer than %d \n", (int)OC_MAX_URL_LENGTH);
        }

        oc_print_group_rp_table_entry(index, GPT_STORE, g_gpt,
                                      oc_core_get_publisher_table_size());
        if (do_save) {
          oc_dump_group_rp_table_entry(index, GPT_STORE, g_gpt,
                                       oc_core_get_publisher_table_size());
        }
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    }
    rep = rep->next;
  };

  oc_knx_increase_fingerprint();
  PRINT("oc_core_fp_p_post_handler - end\n");
  oc_send_response_no_format(request, return_status);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fp_p, knx_fp_p_x, 0, "/fp/p",
                                     OC_IF_C | OC_IF_B, APPLICATION_CBOR,
                                     OC_DISCOVERABLE, oc_core_fp_p_get_handler,
                                     0, oc_core_fp_p_post_handler, 0, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:if.c");

void
oc_create_fp_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_p_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/p", OC_IF_C | OC_IF_B,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_p_get_handler, 0,
                            oc_core_fp_p_post_handler, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_p_x_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_p_x_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index = oc_core_find_index_in_rp_table_from_id(
    id, g_gpt, oc_core_get_publisher_table_size());
  PRINT("  id:%d index = %d\n", id, index);

  if (index == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }

  if (g_gpt[index].id == -1) {
    /* it is empty */
    oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  /* id 0 */
  oc_rep_i_set_int(root, 0, g_gpt[index].id);
  /* ia - 12 */
  if (g_gpt[index].ia > -1) {
    oc_rep_i_set_int(root, 12, g_gpt[index].ia);
  }
  // grpid - 13
  if (g_gpt[index].grpid > 0) {
    oc_rep_i_set_int(root, 13, g_gpt[index].grpid);
  }
  /* iid - 26 */
  if (g_gpt[index].iid > -1) {
    oc_rep_i_set_int(root, 26, g_gpt[index].iid);
  }
  /* fid - 25 */
  if (g_gpt[index].fid > -1) {
    oc_rep_i_set_int(root, 25, g_gpt[index].fid);
  }

  /* frame url as ia exist.*/
  if (g_gpt[index].ia > -1) {
    if (oc_string_len(g_gpt[index].path) > 0) {
      /* set the path only if it not empty
         path- 112 */
      oc_rep_i_set_text_string(root, 112, oc_string(g_gpt[index].path));
    }
  } else {
    /* url -10 */
    oc_rep_i_set_text_string(root, 10, oc_string(g_gpt[index].url));
  }
  // at - 14
  if (oc_string_len(g_gpt[index].at) > 0) {
    oc_rep_i_set_text_string(root, 14, oc_string(g_gpt[index].at));
  }

  /* ga -7 */
  oc_rep_i_set_int_array(root, 7, g_gpt[index].ga, g_gpt[index].ga_len);
  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_fp_p_x_get_handler - end\n");
  return;
}

static void
oc_core_fp_p_x_del_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_p_x_del_handler\n");

  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index = oc_core_find_index_in_rp_table_from_id(
    id, g_gpt, oc_core_get_publisher_table_size());

  if (index == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }

  // delete the entry
  oc_delete_group_rp_table_entry(index, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);

  // make the change persistent
  oc_dump_group_rp_table_entry(index, GPT_STORE, g_gpt,
                               oc_core_get_publisher_table_size());
  oc_knx_increase_fingerprint();
  PRINT("oc_core_fp_p_x_del_handler - end\n");

  oc_send_response_no_format(request, OC_STATUS_DELETED);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fp_p_x, knx_fp_r, 0, "/fp/p/*",
                                     OC_IF_D | OC_IF_C, APPLICATION_CBOR,
                                     OC_DISCOVERABLE,
                                     oc_core_fp_p_x_get_handler, 0, 0,
                                     oc_core_fp_p_x_del_handler, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:if.c");

void
oc_create_fp_p_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_p_x_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/p/*", OC_IF_D | OC_IF_C,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_p_x_get_handler, 0, 0,
                            oc_core_fp_p_x_del_handler, 0, 1, "urn:knx:if.c");
}

#endif /* OC_PUBLISHER_TABLE */

// --------------------------RECIPIENT-----------------------------------------

static void
oc_core_fp_r_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  int matches = 0;

  bool ps_exists = false;
  bool total_exists = false;
  int total = oc_core_find_nr_used_in_group_recipient_table();
  int first_entry = 0;              // inclusive
  int last_entry = GRT_MAX_ENTRIES; // exclusive
  // int query_ps = -1;
  int query_pn = -1;
  bool more_request_needed =
    false; // If more requests (pages) are needed to get the full list

  PRINT("oc_core_fp_r_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /fp/r > l = total>;total=22;ps=5
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

  if (total > first_entry + PAGE_SIZE) {
    more_request_needed = true;
  }

  /* example entry: </fp/r/1>;ct=60 (cbor) */
  for (i = first_entry; i < last_entry; i++) {

    if (g_grt[i].id > -1) {
      // index  in use

      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("</fp/r/");
      response_length += length;
      char string[10];
      sprintf((char *)&string, "%d>", g_grt[i].id);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=60");
      response_length += length;

      matches++;

      if (matches >= PAGE_SIZE) {
        break;
      }
    }
  }

  if (more_request_needed) {
    int next_page_num = query_pn > -1 ? query_pn + 1 : 1;
    response_length +=
      add_next_page_indicator(oc_string(request->resource->uri), next_page_num);
  }
  oc_send_linkformat_response(request, OC_STATUS_OK, response_length);

  PRINT("oc_core_fp_r_get_handler - end\n");
}

static void
oc_core_fp_r_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  oc_status_t return_status = OC_STATUS_BAD_REQUEST;

  /* check if the accept header is cbor-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_response_no_format(request, OC_STATUS_METHOD_NOT_ALLOWED);
    return;
  }

  oc_print_rep_as_json(request->request_payload, true);

  int index = -1;
  int id = -1;
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;
      id = oc_table_find_id_from_rep(object);
      if (id == -1) {
        OC_ERR("  ERROR id %d", index);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      index = oc_core_find_index_in_rp_table_from_id(
        id, g_grt, oc_core_get_recipient_table_size());
      if (index != -1) {
        // index already in use, so it will be changed
        return_status = OC_STATUS_CHANGED;
      } else {
        // no index, so we will create one
        return_status = OC_STATUS_CREATED;
      }
      index = find_empty_slot_in_rp_table(id, g_grt,
                                          oc_core_get_recipient_table_size());
      if (index == -1) {
        OC_ERR("  ERROR index %d", index);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      g_grt[index].id = id;

      bool id_only = true;
      int mandatory_items = 0; // Needs to be 2 for creating entry, i.e. id & ga
      bool identifier_exists = false; // Once of ia, grpid or url
      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {

        case OC_REP_INT: {
          if (object->iname == 0) {
            mandatory_items++;
          } else {
            id_only = false;
          }
          if (object->iname == 12) {
            identifier_exists = true;
            g_grt[index].ia = (int)object->value.integer;
          }
          if (object->iname == 13) {
            identifier_exists = true;
            g_grt[index].grpid = (uint32_t)object->value.integer;
          }
          if (object->iname == 26) {
            g_grt[index].iid = object->value.integer;
          }
          if (object->iname == 25) {
            g_grt[index].fid = object->value.integer;
          }
        } break;

        case OC_REP_STRING: {
          id_only = false;
          if (object->iname == 112) {
            oc_free_string(&g_grt[index].path);
            oc_new_string(&g_grt[index].path, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (object->iname == 10) {
            identifier_exists = true;
            oc_free_string(&g_grt[index].url);
            oc_new_string(&g_grt[index].url, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (object->iname == 14) {
            oc_free_string(&g_grt[index].at);
            oc_new_string(&g_grt[index].at, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;
        case OC_REP_INT_ARRAY: {
          id_only = false;
          if (object->iname == 7) {
            mandatory_items++;
            // g_got[index].id = object->value.integer;
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            uint32_t *new_array =
              (uint32_t *)malloc(array_size * sizeof(uint32_t));
            if (new_array) {
              for (int i = 0; i < array_size; i++) {
                new_array[i] = (uint32_t)arr[i];
              }
              if (g_grt[index].ga != 0) {
                free(g_grt[index].ga);
              }
              g_grt[index].ga_len = array_size;
              g_grt[index].ga = new_array;
            } else {
              OC_ERR("out of memory");
              return_status = OC_STATUS_INTERNAL_SERVER_ERROR;
            }
          }
        } break;
        case OC_REP_NIL: {
          if (object->iname == 7) {
            mandatory_items++;
          }
        } break;
        default:
          break;
        }
        object = object->next;
      }
      if (id_only) {
        PRINT("  only found id in request, deleting entry at index: %d\n",
              index);
        oc_delete_group_rp_table_entry(index, GRT_STORE, g_grt,
                                       GRT_MAX_ENTRIES);
      } else if (return_status == OC_STATUS_CREATED &&
                 (mandatory_items != 2 || !identifier_exists)) {
        PRINT("Mandatory items missing!\n");
        oc_delete_group_rp_table_entry(index, GRT_STORE, g_grt,
                                       GRT_MAX_ENTRIES);
        oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
        return;
      } else {
        bool do_save = true;
        if (oc_string_len(g_grt[index].url) > OC_MAX_URL_LENGTH) {
          // do_save = false;
          OC_ERR("  url is longer than %d \n", (int)OC_MAX_URL_LENGTH);
        }
        if (oc_string_len(g_grt[index].path) > OC_MAX_URL_LENGTH) {
          // do_save = false;
          OC_ERR("  path is longer than %d \n", (int)OC_MAX_URL_LENGTH);
        }

        oc_print_group_rp_table_entry(index, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
        if (do_save) {
          PRINT("  storing at %d\n", index);
          oc_dump_group_rp_table_entry(index, GRT_STORE, g_grt,
                                       GRT_MAX_ENTRIES);
        }
      }
    }
    case OC_REP_NIL:
      break;
    default:
      break;
    }
    rep = rep->next;
  };

  oc_knx_increase_fingerprint();

  PRINT("oc_core_fp_r_post_handler - end\n");
  oc_send_response_no_format(request, return_status);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fp_r, knx_fp_r_x, 0, "/fp/r",
                                     OC_IF_C | OC_IF_B, APPLICATION_CBOR,
                                     OC_DISCOVERABLE, oc_core_fp_r_get_handler,
                                     0, oc_core_fp_r_post_handler, 0, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:if.c");
void
oc_create_fp_r_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/r", OC_IF_C | OC_IF_B,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_r_get_handler, 0,
                            oc_core_fp_r_post_handler, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_r_x_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_r_x_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index =
    oc_core_find_index_in_rp_table_from_id(id, g_grt, GRT_MAX_ENTRIES);

  PRINT("  id:%d index = %d\n", id, index);

  if (index == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }

  if (g_grt[index].id == -1) {
    // it is empty
    oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_grt[index].id);
  // ia - 12
  if (g_grt[index].ia > 0) {
    oc_rep_i_set_int(root, 12, g_grt[index].ia);
  }
  // grpid - 13
  if (g_grt[index].grpid > 0) {
    oc_rep_i_set_int(root, 13, g_grt[index].grpid);
  }
  // fid - 25
  if (g_grt[index].fid > 0) {
    oc_rep_i_set_int(root, 25, g_grt[index].fid);
  }
  // iid - 26
  if (g_grt[index].iid > 0) {
    oc_rep_i_set_int(root, 26, g_grt[index].iid);
  }
  // url- 10
  oc_rep_i_set_text_string(root, 10, oc_string(g_grt[index].url));
  // at - 14
  if (oc_string_len(g_grt[index].at) > 0) {
    oc_rep_i_set_text_string(root, 14, oc_string(g_grt[index].at));
  }
  // ga - 7
  oc_rep_i_set_int_array(root, 7, g_grt[index].ga, g_grt[index].ga_len);

  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_fp_r_x_get_handler - end\n");
  return;
}

static void
oc_core_fp_r_x_del_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_r_x_del_handler\n");

  size_t device_index = request->resource->device;
  if (oc_a_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index =
    oc_core_find_index_in_rp_table_from_id(id, g_grt, GRT_MAX_ENTRIES);

  if (index == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }
  PRINT("oc_core_fp_r_x_del_handler: deleting id %d at index %d\n", id, index);

  // delete the entry
  oc_delete_group_rp_table_entry(index, GRT_STORE, g_grt, GRT_MAX_ENTRIES);

  // make the change persistent
  oc_dump_group_rp_table_entry(index, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
  oc_knx_increase_fingerprint();

  PRINT("oc_core_fp_r_x_del_handler - end\n");

  oc_send_response_no_format(request, OC_STATUS_DELETED);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(knx_fp_r_x, knx_p, 0, "/fp/r/*",
                                     OC_IF_D | OC_IF_C, APPLICATION_CBOR,
                                     OC_DISCOVERABLE,
                                     oc_core_fp_r_x_get_handler, 0, 0,
                                     oc_core_fp_r_x_del_handler, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:if.c");
void
oc_create_fp_r_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_x_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/r/*", OC_IF_D | OC_IF_C,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_r_x_get_handler, 0, 0,
                            oc_core_fp_r_x_del_handler, 1, "urn:knx:if.c");
}

bool
oc_core_check_recipient_index_on_group_address(int index,
                                               uint32_t group_address)
{
  if (index >= GRT_MAX_ENTRIES) {
    return -1;
  }
  if (group_address <= 0) {
    return -1;
  }
  for (int i = 0; g_grt[index].ga_len; i++) {
    if (g_grt[index].ga[i] == group_address) {
      return true;
    }
  }
  return false;
}

uint32_t
oc_core_get_recipient_ia(int index)
{
  if (index >= GRT_MAX_ENTRIES) {
    return 0;
  }

  return g_grt[index].ia;
}

char *
oc_core_get_recipient_index_url_or_path(int index)
{
  if (index >= GRT_MAX_ENTRIES) {
    return NULL;
  }

  if (g_grt[index].ia > 0) {
    PRINT("oc_core_get_recipient_index_url_or_path: ia %d\n", g_grt[index].ia);
    if (oc_string_len(g_grt[index].path) > 0) {
      PRINT("      oc_core_get_recipient_index_url_or_path path %s\n",
            oc_string_checked(g_grt[index].path));
      return oc_string(g_grt[index].path);

    } else {
      // do .knx
      // spec 1.1. change this to /k
      PRINT("      oc_core_get_recipient_index_url_or_path (default) %s\n",
            ".knx");
      return ".knx";
    }

  } else {
    if (oc_string_len(g_grt[index].url) > 0) {
      //
      PRINT("      oc_core_get_recipient_index_url_or_path url %s\n",
            oc_string_checked(g_grt[index].url));
      return oc_string(g_grt[index].url);
    }
  }
  return NULL;
}

//#endif /* OC_RECIPIENT_TABLE */

// -----------------------------------------------------------------------------

void
oc_cflags_as_string(char *buffer, oc_cflag_mask_t cflags)
{

  if (cflags & OC_CFLAG_READ) {
    strcat(buffer, "r");
  } else {
    strcat(buffer, ".");
  }
  if (cflags & OC_CFLAG_WRITE) {
    strcat(buffer, "w");
  } else {
    strcat(buffer, ".");
  }
  if (cflags & OC_CFLAG_INIT) {
    strcat(buffer, "i");
  } else {
    strcat(buffer, ".");
  }
  if (cflags & OC_CFLAG_TRANSMISSION) {
    strcat(buffer, "t");
  } else {
    strcat(buffer, ".");
  }
  if (cflags & OC_CFLAG_UPDATE) {
    strcat(buffer, "u");
  } else {
    strcat(buffer, ".");
  }
}

void
oc_print_cflags(oc_cflag_mask_t cflags)
{

  if (cflags & OC_CFLAG_READ) {
    PRINT("r");
  }
  if (cflags & OC_CFLAG_WRITE) {
    PRINT("w");
  }
  if (cflags & OC_CFLAG_INIT) {
    PRINT("i");
  }
  if (cflags & OC_CFLAG_TRANSMISSION) {
    PRINT("t");
  }
  if (cflags & OC_CFLAG_UPDATE) {
    PRINT("u");
  }
  PRINT("\n");
}

void
oc_print_group_object_table_entry(int entry)
{
  if (g_got[entry].id == -1) {
    return;
  }

  PRINT("    id (0)     : %d\n", g_got[entry].id);
  PRINT("    href (11)  : %s\n", oc_string_checked(g_got[entry].href));
  PRINT("    cflags (8) : %d string: ", g_got[entry].cflags);
  oc_print_cflags(g_got[entry].cflags);
  PRINT("    ga (7)     : [");
  for (int i = 0; i < g_got[entry].ga_len; i++) {
    PRINT(" %u", g_got[entry].ga[i]);
  }
  PRINT(" ]\n");
}

void
oc_dump_group_object_table_entry(int entry)
{
  char filename[20];
  snprintf(filename, 20, "%s_%d", GOT_STORE, entry);

  uint8_t *buf = malloc(OC_MAX_APP_DATA_SIZE);
  if (!buf)
    return;

  oc_rep_new(buf, OC_MAX_APP_DATA_SIZE);
  // write the data
  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_got[entry].id);
  // href- 11
  oc_rep_i_set_text_string(root, 11, oc_string(g_got[entry].href));
  // ga - 7
  oc_rep_i_set_int_array(root, 7, g_got[entry].ga, g_got[entry].ga_len);

  // cflags 8 /// this is different than the response on the wire
  oc_rep_i_set_int(root, 8, g_got[entry].cflags);

  oc_rep_end_root_object();

  int size = oc_rep_get_encoded_payload_size();
  if (size > 0) {
    OC_DBG("oc_dump_group_object_table_entry: dumped current state [%s] [%d]: "
           "size %d",
           filename, entry, size);
    long written_size = oc_storage_write(filename, buf, size);
    if (written_size != (long)size) {
      PRINT("oc_dump_group_object_table_entry: written %d != %d (towrite)\n",
            (int)written_size, size);
    }
  }

  free(buf);
}

#define GOT_ENTRY_MAX_SIZE (1024)
void
oc_load_group_object_table_entry(int entry)
{
  long ret = 0;
  char filename[20];
  snprintf(filename, 20, "%s_%d", GOT_STORE, entry);

  oc_rep_t *rep, *head;

  uint8_t *buf = malloc(GOT_ENTRY_MAX_SIZE);
  if (!buf) {
    return;
  }

  ret = oc_storage_read(filename, buf, GOT_ENTRY_MAX_SIZE);
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
            g_got[entry].id = (int)rep->value.integer;
          }
          if (rep->iname == 8) {
            g_got[entry].cflags = (int)rep->value.integer;
          }
          break;
        case OC_REP_STRING:
          if (rep->iname == 11) {

            oc_free_string(&g_got[entry].href);
            oc_new_string(&g_got[entry].href, oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          break;
        case OC_REP_INT_ARRAY:
          if (rep->iname == 7) {
            int64_t *arr = oc_int_array(rep->value.array);
            int array_size = (int)oc_int_array_size(rep->value.array);
            uint32_t *new_array =
              (uint32_t *)malloc(array_size * sizeof(uint32_t));
            if ((new_array) && (array_size > 0)) {
              for (int i = 0; i < array_size; i++) {
#pragma warning(suppress : 6386)
                new_array[i] = (uint32_t)arr[i];
              }
              if (g_got[entry].ga != 0) {
                free(g_got[entry].ga);
              }
              PRINT("  ga size %d\n", array_size);
              g_got[entry].ga_len = array_size;
              g_got[entry].ga = new_array;
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
oc_load_group_object_table()
{
  PRINT("Loading Group Object Table from Persistent storage\n");
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    oc_load_group_object_table_entry(i);
    oc_print_group_object_table_entry(i);
  }
}

void
oc_free_group_object_table_entry(int entry, bool init)
{
  g_got[entry].id = -1;
  if (init == false) {
    oc_free_string(&g_got[entry].href);
    free(g_got[entry].ga);
  }

  g_got[entry].ga = NULL;
  g_got[entry].ga_len = 0;
  g_got[entry].cflags = 0;
}

void
oc_delete_group_object_table_entry(int entry)
{
  char filename[20];
  snprintf(filename, 20, "%s_%d", GOT_STORE, entry);
  oc_storage_erase(filename);

  oc_free_group_object_table_entry(entry, false);
}

void
oc_delete_group_object_table()
{
  PRINT("Deleting Group Object Table from Persistent storage\n");
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    oc_delete_group_object_table_entry(i);
    oc_print_group_object_table_entry(i);
  }
}

void
oc_free_group_object_table()
{
  PRINT("Free Group Object Table\n");
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    oc_free_group_object_table_entry(i, false);
  }
}

// -----------------------------------------------------------------------------

int
oc_core_find_index_in_rp_table_from_id(int id, oc_group_rp_table_t *rp_table,
                                       int max_size)
{
  for (int i = 0; i < max_size; i++) {
    if (rp_table[i].id == id) {
      return i;
    }
  }
  return -1;
}

static void
oc_print_group_rp_table_entry(int entry, char *Store,
                              oc_group_rp_table_t *rp_table, int max_size)
{
  (void)max_size;
  if (rp_table[entry].id == -1) {
    return;
  }
  PRINT("  %s [%d] --> [%d]\n", Store, entry, rp_table[entry].ga_len);
  PRINT("    id (0)     : %d\n", rp_table[entry].id);
  PRINT("    ia (12)    : %d\n", rp_table[entry].ia);
  PRINT("    iid (26)   : %" PRIu64 "\n", rp_table[entry].iid);
  PRINT("    fid (25)   : %" PRIu64 "\n", rp_table[entry].fid);
  PRINT("    grpid (13) : %u\n", rp_table[entry].grpid);
  if (oc_string_len(rp_table[entry].path) > 0) {
    PRINT("    path (112) : '%s'\n", oc_string_checked(rp_table[entry].path));
  }
  if (oc_string_len(rp_table[entry].url) > 0) {
    PRINT("    url (10)   : '%s'\n", oc_string_checked(rp_table[entry].url));
  }
  if (oc_string_len(rp_table[entry].at) > 0) {
    PRINT("    at (14) : %s\n", oc_string_checked(rp_table[entry].at));
  }
  PRINT("    ga (7)     : [");
  for (int i = 0; i < rp_table[entry].ga_len; i++) {
    PRINT(" %u", rp_table[entry].ga[i]);
  }
  PRINT(" ]\n");
}

static void
oc_print_reduced_group_rp_table_entry(int entry, char *Store,
                                      oc_group_rp_table_t *rp_table,
                                      int max_size)
{
  (void)max_size;
  if (rp_table[entry].id == -1) {
    return;
  }
  printf("  %s [%d] --> [%d]\n", Store, entry, rp_table[entry].ga_len);
  printf("    id (0)     : %d\n", rp_table[entry].id);

  printf("    iid (26)   : ");
  oc_print_uint64_t(rp_table[entry].iid, DEC_REPRESENTATION);
  printf("\n");

  printf("    grpid (13) : %u\n", rp_table[entry].grpid);

  printf("    ga (7)     : [");
  for (int i = 0; i < rp_table[entry].ga_len; i++) {
    printf(" %u", rp_table[entry].ga[i]);
  }
  printf(" ]\n");
}

#define RP_ENTRY_MAX_SIZE (1024)
static void
oc_dump_group_rp_table_entry(int entry, char *Store,
                             oc_group_rp_table_t *rp_table, int max_size)
{
  (void)max_size;
  char filename[20];
  snprintf(filename, 20, "%s_%d", Store, entry);
  // PRINT("oc_dump_group_rp_table_entry %s, fname=%s\n", Store, filename);

  uint8_t *buf = malloc(RP_ENTRY_MAX_SIZE);
  if (!buf) {
    PRINT("oc_dump_group_rp_table_entry: no allocated mem\n");
    return;
  }

  oc_rep_new(buf, RP_ENTRY_MAX_SIZE);
  // write the data
  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, rp_table[entry].id);
  // ia- 12
  oc_rep_i_set_int(root, 12, rp_table[entry].ia);
  // iid 26
  oc_rep_i_set_int(root, 26, rp_table[entry].iid);
  // fid - 25
  oc_rep_i_set_int(root, 25, rp_table[entry].fid);
  // grpid - 13
  oc_rep_i_set_int(root, 13, rp_table[entry].grpid);
  // at - 14
  oc_rep_i_set_text_string(root, 14, oc_string(rp_table[entry].at));
  // path- 112
  oc_rep_i_set_text_string(root, 112, oc_string(rp_table[entry].path));
  // url - 10
  oc_rep_i_set_text_string(root, 10, oc_string(rp_table[entry].url));
  // ga - 7
  oc_rep_i_set_int_array(root, 7, rp_table[entry].ga, rp_table[entry].ga_len);

  // cflags 8 /// this is different than the response on the wire
  // oc_rep_i_set_int(root, 8, rp_table[entry].cflags);

  oc_rep_end_root_object();

  int size = oc_rep_get_encoded_payload_size();
  if (size > 0) {
    OC_DBG("oc_dump_group_rp_table_entry: dumped current state [%s] [%s] [%d]: "
           "size %d",
           filename, Store, entry, size);
    long written_size = oc_storage_write(filename, buf, size);
    if (written_size != (long)size) {
      PRINT("oc_dump_group_rp_table_entry: written %d != %d (towrite)\n",
            (int)written_size, size);
    }
  }

  free(buf);
}

void
oc_load_group_rp_table_entry(int entry, char *Store,
                             oc_group_rp_table_t *rp_table, int max_size)
{
  (void)max_size;
  long ret = 0;
  char filename[20];
  snprintf(filename, 20, "%s_%d", Store, entry);

  oc_rep_t *rep, *head;

  uint8_t *buf = malloc(OC_MAX_APP_DATA_SIZE);
  if (!buf) {
    return;
  }

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
          if (rep->iname == 0) {
            rp_table[entry].id = (int)rep->value.integer;
          }
          if (rep->iname == 12) {
            rp_table[entry].ia = (int)rep->value.integer;
          }
          if (rep->iname == 13) {
            rp_table[entry].grpid = (uint32_t)rep->value.integer;
          }
          if (rep->iname == 25) {
            rp_table[entry].fid = rep->value.integer;
          }
          if (rep->iname == 26) {
            rp_table[entry].iid = rep->value.integer;
          }
          break;
        case OC_REP_STRING:
          if (rep->iname == 112) {
            oc_free_string(&rp_table[entry].path);
            oc_new_string(&rp_table[entry].path, oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 10) {
            oc_free_string(&rp_table[entry].url);
            oc_new_string(&rp_table[entry].url, oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          if (rep->iname == 14) {
            oc_free_string(&rp_table[entry].at);
            oc_new_string(&rp_table[entry].at, oc_string(rep->value.string),
                          oc_string_len(rep->value.string));
          }
          break;
        case OC_REP_INT_ARRAY:
          if (rep->iname == 7) {
            int64_t *arr = oc_int_array(rep->value.array);
            int array_size = (int)oc_int_array_size(rep->value.array);
            uint32_t *new_array =
              (uint32_t *)malloc(array_size * sizeof(uint32_t));
            if ((new_array != NULL) && (array_size > 0)) {
              for (int i = 0; i < array_size; i++) {
#pragma warning(suppress : 6386)
                new_array[i] = (uint32_t)arr[i];
              }
              // assign only when the new array is allocated correctly
              rp_table[entry].ga_len = array_size;
            }
            if (rp_table[entry].ga != 0) {
              free(rp_table[entry].ga);
            }
            PRINT("  ga size %d\n", array_size);
            // if (rp_table[entry].ga) {
            //   free(rp_table[entry].ga);
            // }
            rp_table[entry].ga = new_array;
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
oc_load_rp_object_table()
{

  PRINT("Loading Group Recipient Table from Persistent storage\n");
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    oc_load_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
    oc_print_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
  }

#ifdef OC_PUBLISHER_TABLE
  PRINT("Loading Group Publisher Table from Persistent storage\n");
  for (int i = 0; i < oc_core_get_publisher_table_size(); i++) {
    oc_load_group_rp_table_entry(i, GPT_STORE, g_gpt,
                                 oc_core_get_publisher_table_size());
    oc_print_group_rp_table_entry(i, GPT_STORE, g_gpt,
                                  oc_core_get_publisher_table_size());
  }
#endif /* OC_PUBLISHER_TABLE */
}

static void
oc_free_group_rp_table_entry(int entry, char *Store,
                             oc_group_rp_table_t *rp_table, int max_size,
                             bool init)
{
  (void)max_size;
  (void)Store;
  rp_table[entry].id = -1;
  rp_table[entry].ia = -1;
  rp_table[entry].iid = -1;
  rp_table[entry].fid = -1;
  rp_table[entry].grpid = 0;
  if (init == false) {
    oc_free_string(&rp_table[entry].path);
    oc_free_string(&rp_table[entry].url);
    oc_free_string(&rp_table[entry].at);
    free(rp_table[entry].ga);
  }
  rp_table[entry].ga = NULL;
  rp_table[entry].ga_len = 0;
}

static void
oc_delete_group_rp_table_entry(int entry, char *Store,
                               oc_group_rp_table_t *rp_table, int max_size)
{
  char filename[20];
  snprintf(filename, 20, "%s_%d", Store, entry);
  oc_storage_erase(filename);

  oc_free_group_rp_table_entry(entry, Store, rp_table, max_size, false);
}

void
oc_delete_group_rp_table()
{
  PRINT("Deleting Group Recipient Table from Persistent storage\n");
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    oc_delete_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
    oc_print_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
  }

#ifdef OC_PUBLISHER_TABLE
  PRINT("Deleting Group Publisher Table from Persistent storage\n");
  for (int i = 0; i < oc_core_get_publisher_table_size(); i++) {
    oc_delete_group_rp_table_entry(i, GPT_STORE, g_gpt,
                                   oc_core_get_publisher_table_size());
    oc_print_group_rp_table_entry(i, GPT_STORE, g_gpt,
                                  oc_core_get_publisher_table_size());
  }
#endif /*  OC_PUBLISHER_TABLE */
}

void
oc_free_group_rp_table()
{
  PRINT("Free Group Recipient Table from Persistent storage\n");
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    oc_free_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES, false);
  }

#ifdef OC_PUBLISHER_TABLE
  PRINT("Deleting Group Publisher Table from Persistent storage\n");
  for (int i = 0; i < oc_core_get_publisher_table_size(); i++) {
    oc_free_group_rp_table_entry(i, GPT_STORE, g_gpt,
                                 oc_core_get_publisher_table_size(), false);
  }
#endif /*  OC_PUBLISHER_TABLE */
}

int
find_empty_slot_in_rp_table(int id, oc_group_rp_table_t *rp_table, int max_size)
{
  int index = -1;
  if (id < 0) {
    // should be a positive number
    return index;
  }
  index = oc_core_find_index_in_rp_table_from_id(id, rp_table, max_size);
  if (index > -1) {
    // overwrite existing index
    return index;
  }

  // empty slot
  for (int i = 0; i < max_size; i++) {
    if (rp_table[i].id == -1) {
      return i;
    }
  }
  return -1;
}

static int
oc_core_find_used_nr_in_rp_table(oc_group_rp_table_t *rp_table, int max_size)
{
  int counter = 0;
  PRINT("Deleting Group Recipient Table from Persistent storage\n");

  for (int i = 0; i < max_size; i++) {
    if (rp_table[i].id > -1) {
      counter++;
    }
  }
  return counter;
}

int
oc_core_add_rp_entry(int index, oc_group_rp_table_t *rp_table,
                     int rp_table_size, oc_group_rp_table_t entry)
{
  if (index >= rp_table_size) {
    OC_ERR("recipient table index is too large: index(%d) max_size(%d)", index,
           rp_table_size);
  }

  // Store entries
  rp_table[index].id = entry.id;
  rp_table[index].iid = entry.iid;
  rp_table[index].fid = entry.fid;
  rp_table[index].grpid = entry.grpid;

  // Copy group addresses
  rp_table[index].ga_len = 0;
  uint32_t *new_array = (uint32_t *)malloc(entry.ga_len * sizeof(uint32_t));
  if ((new_array != NULL) && (entry.id > -1)) {
    for (int i = 0; i < entry.ga_len; i++) {
#pragma warning(suppress : 6386)
      new_array[i] = entry.ga[i];
    }
    // copy only when the allocation was done correctly
    rp_table[index].ga_len = entry.ga_len;
    if (rp_table[index].ga != 0) {
      free(rp_table[index].ga);
    }
    rp_table[index].ga = new_array;
  }

  return 0;
}

int
oc_core_add_recipient_entry(int index, oc_group_rp_table_t entry)
{
  return oc_core_add_rp_entry(index, g_grt, oc_core_get_recipient_table_size(),
                              entry);
}

int
oc_core_get_recipient_table_size()
{
  return GRT_MAX_ENTRIES;
}

oc_group_rp_table_t *
oc_core_get_recipient_table_entry(int index)
{
  if (index < 0) {
    return NULL;
  }
  if (index >= GRT_MAX_ENTRIES) {
    return NULL;
  }
  return &g_grt[index];
}

int
oc_core_get_publisher_table_size()
{
#ifdef OC_PUBLISHER_TABLE
  return GPT_MAX_ENTRIES;
#else
  return 0;
#endif
}

oc_group_rp_table_t *
oc_core_get_publisher_table_entry(int index)
{

#ifdef OC_PUBLISHER_TABLE
  if (index < 0) {
    return NULL;
  }
  if (index >= oc_core_get_publisher_table_size()) {
    return NULL;
  }
  return &g_gpt[index];
#else
  return NULL;
#endif
}

int
oc_core_add_publisher_entry(int index, oc_group_rp_table_t entry)
{
  return oc_core_add_rp_entry(index, g_gpt, oc_core_get_publisher_table_size(),
                              entry);
}

int
oc_core_find_empty_slot_in_publisher_table(int id)
{
  return find_empty_slot_in_rp_table(id, g_gpt,
                                     oc_core_get_publisher_table_size());
}

int
oc_core_find_index_in_publisher_table_from_id(int id)
{
  return oc_core_find_index_in_rp_table_from_id(
    id, g_gpt, oc_core_get_publisher_table_size());
}

int
oc_core_find_empty_slot_in_recipient_table(int id)
{
  return find_empty_slot_in_rp_table(id, g_grt,
                                     oc_core_get_recipient_table_size());
}

int
oc_core_find_index_in_recipient_table_from_id(int id)
{
  return oc_core_find_index_in_rp_table_from_id(
    id, g_grt, oc_core_get_recipient_table_size());
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void
oc_init_tables()
{
#ifdef OC_PUBLISHER_TABLE
  for (int i = 0; i < oc_core_get_publisher_table_size(); i++) {
    // oc_delete_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
    oc_free_group_rp_table_entry(i, GPT_STORE, g_gpt,
                                 oc_core_get_publisher_table_size(), true);
  }
#endif /* OC_PUBLISHER_TABLE */
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    // oc_delete_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
    oc_free_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES, true);
  }
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    oc_free_group_object_table_entry(i, true);
    // oc_delete_group_object_table_entry(i);
  }
}

// -----------------------------------------------------------------------------

void
oc_create_knx_fp_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_fp_resources");

  if (device_index == 0) {
    OC_DBG("resources for dev 0 created statically");
  } else {
    oc_create_fp_g_resource(OC_KNX_FP_G, device_index);
    oc_create_fp_g_x_resource(OC_KNX_FP_G_X, device_index);

#ifdef OC_PUBLISHER_TABLE
    oc_create_fp_p_resource(OC_KNX_FP_P, device_index);
    oc_create_fp_p_x_resource(OC_KNX_FP_P_X, device_index);
#endif /* OC_PUBLISHER_TABLE */

    oc_create_fp_r_resource(OC_KNX_FP_R, device_index);
    oc_create_fp_r_x_resource(OC_KNX_FP_R_X, device_index);
  }
  oc_init_tables();
  oc_load_group_object_table();
  oc_load_rp_object_table();

  // oc_register_group_multicasts();
}

void
oc_free_knx_fp_resources(size_t device_index)
{
  oc_free_group_rp_table();
  oc_free_group_object_table();
}

// -----------------------------------------------------------------------------

bool
is_in_array(uint32_t value, uint32_t *array, int array_size)
{
  if (array_size <= 0) {
    return false;
  }
  if (array == NULL) {
    return false;
  }
  for (int i = 0; i < array_size; i++) {
    if (array[i] == value) {
      return true;
    }
  }
  return false;
}

bool
oc_add_points_in_group_object_table_to_response(oc_request_t *request,
                                                size_t device_index,
                                                uint32_t group_address,
                                                size_t *response_length,
                                                int matches)
{
  (void)request;
  (void)device_index;
  (void)response_length;
  (void)matches;
  // int length = 0;
  bool return_value = false;

  PRINT("oc_add_points_in_group_object_table_to_response %d\n", group_address);

  int index;
  for (index = 0; index < GOT_MAX_ENTRIES; index++) {
    if (g_got[index].id > -1) {
      if (is_in_array(group_address, g_got[index].ga, g_got[index].ga_len)) {
        // add the resource
        // note, not checked if the resource is already there...
        PRINT("oc_add_points_in_group_object_table_to_response [%d] %s\n",
              index, oc_string_checked(g_got[index].href));
        oc_add_resource_to_wk(oc_ri_get_app_resource_by_uri(
                                oc_string(g_got[index].href),
                                oc_string_len(g_got[index].href), device_index),
                              request, device_index, response_length, 1);
        return_value = true;
      }
    }
  }
  return return_value;
}

oc_endpoint_t
oc_create_multicast_group_address_with_port(oc_endpoint_t in, uint32_t group_nr,
                                            int64_t iid, int scope, int port)
{
  // create the multicast address from group and scope
  // FF3_:FD__:____:____:(8-f)___:____
  // FF35:30:<ULA-routing-prefix>::<group id>
  //    | 5 == scope
  //    | 3 == scope
  // Multicast prefix: FF35:0030:  [4 bytes]
  // ULA routing prefix: FD11:2222:3333::  [6 bytes + 2 empty bytes]
  // Group Identifier: 8000 : 0068 [4 bytes ]

  // group number to the various bytes
  uint8_t byte_1 = (uint8_t)group_nr;
  uint8_t byte_2 = (uint8_t)(group_nr >> 8);
  uint8_t byte_3 = (uint8_t)(group_nr >> 16);
  uint8_t byte_4 = (uint8_t)(group_nr >> 24);

  // iid as  ula prefix to various bytes
  uint8_t ula_1 = (uint8_t)iid;
  uint8_t ula_2 = (uint8_t)(iid >> 8);
  uint8_t ula_3 = (uint8_t)(iid >> 16);
  uint8_t ula_4 = (uint8_t)(iid >> 24);
  uint8_t ula_5 = (uint8_t)(iid >> 32);

  int my_transport_flags = 0;
  my_transport_flags += IPV6;
  my_transport_flags += MULTICAST;
  my_transport_flags += DISCOVERY;
#ifdef OC_OSCORE
  my_transport_flags += OSCORE;
#endif

  oc_make_ipv6_endpoint(group_mcast, my_transport_flags, port, 0xff,
                        0x30 + scope, 0, 0x30, //  FF35::30:
                        0xfd, ula_5, ula_4, ula_3, ula_2,
                        ula_1, // FD11 : 2222 : 3333
                        0, 0,  // ::
                        byte_4, byte_3, byte_2, byte_1);
  PRINT("  oc_create_multicast_group_address_with_port S=%d iid=%" PRIu64
        " G=%u B4=%d "
        "B3=%d B2=%d "
        "B1=%d\n",
        scope, iid, group_nr, byte_4, byte_3, byte_2, byte_1);
  PRINT("  ");
  PRINTipaddr(group_mcast);
  PRINT("\n");
  group_mcast.group_address = group_nr;
  memcpy(&in, &group_mcast, sizeof(oc_endpoint_t));

  return in;
}

oc_endpoint_t
oc_create_multicast_group_address(oc_endpoint_t in, uint32_t group_nr,
                                  int64_t iid, int scope)
{
  return oc_create_multicast_group_address_with_port(in, group_nr, iid, scope,
                                                     5683);
}

void
subscribe_group_to_multicast_with_port(uint32_t group_nr, int64_t iid,
                                       int scope, int port)
{
  // create the multi cast address from group and scope and port
  oc_endpoint_t group_mcast;
  memset(&group_mcast, 0, sizeof(group_mcast));

  group_mcast = oc_create_multicast_group_address_with_port(
    group_mcast, group_nr, iid, scope, port);

  // subscribe
  oc_connectivity_subscribe_mcast_ipv6(&group_mcast);
}

void
subscribe_group_to_multicast(uint32_t group_nr, int64_t iid, int scope)
{
  // FF35::30: <ULA-routing-prefix>::<group id>
  //
  // create the multi cast address from group and scope
  oc_endpoint_t group_mcast;
  memset(&group_mcast, 0, sizeof(group_mcast));

  group_mcast =
    oc_create_multicast_group_address(group_mcast, group_nr, iid, scope);

  // subscribe
  oc_connectivity_subscribe_mcast_ipv6(&group_mcast);
}

void
unsubscribe_group_to_multicast_with_port(uint32_t group_nr, int64_t iid,
                                         int scope, int port)
{
  // create the multi cast address from group and scope
  oc_endpoint_t group_mcast;
  memset(&group_mcast, 0, sizeof(group_mcast));

  group_mcast = oc_create_multicast_group_address_with_port(
    group_mcast, group_nr, iid, scope, port);

  // un subscribe
  oc_connectivity_unsubscribe_mcast_ipv6(&group_mcast);
}

void
unsubscribe_group_to_multicast(uint32_t group_nr, int64_t iid, int scope)
{
  // FF35::30: <ULA-routing-prefix>::<group id>
  //
  // create the multi cast address from group and scope
  oc_endpoint_t group_mcast;
  memset(&group_mcast, 0, sizeof(group_mcast));

  group_mcast =
    oc_create_multicast_group_address(group_mcast, group_nr, iid, scope);

  // un subscribe
  oc_connectivity_unsubscribe_mcast_ipv6(&group_mcast);
}

uint32_t
oc_find_grpid_in_table(oc_group_rp_table_t *rp_table, int max_size,
                       uint32_t group_address)
{
  int index;
  for (index = 0; index < max_size; index++) {
    uint32_t *array = rp_table[index].ga;
    int array_size = rp_table[index].ga_len;
    bool found = is_in_array(group_address, array, array_size);
    if (found) {
      return rp_table[index].grpid;
    }
  }
  // not found
  return 0;
}

uint32_t
oc_find_grpid_in_publisher_table(uint32_t group_address)
{
  return oc_find_grpid_in_table(g_gpt, oc_core_get_publisher_table_size(),
                                group_address);
}

uint32_t
oc_find_grpid_in_recipient_table(uint32_t group_address)
{
  return oc_find_grpid_in_table(g_grt, GRT_MAX_ENTRIES, group_address);
}

void
oc_register_group_multicasts()
{
  // installation id will be used as ULA prefix
  oc_device_info_t *device = oc_core_get_device_info(0);
  if (!device) {
    PRINT("oc_register_group_multicasts: no device info\n");
    return;
  }
  int64_t installation_id = device->iid;
  uint32_t port = device->mport;

  PRINT("oc_register_group_multicasts: mport %d \n", port);

  bool registered_pub = false;
  bool pub_entry = false;

  int index;
  for (index = 0; index < oc_core_get_publisher_table_size(); index++) {
    uint32_t grpid = g_gpt[index].grpid;
    if (grpid > 0) {
      pub_entry = true;
      break;
    }
  }
  if (pub_entry == true) {
    for (index = 0; index < oc_core_get_publisher_table_size(); index++) {
      int nr_entries = g_got[index].ga_len;

      oc_cflag_mask_t cflags = g_got[index].cflags;
      // check if the group address is used for receiving.
      // e.g. WRITE or UPDATE
      if (((cflags & OC_CFLAG_WRITE) > 0) || ((cflags & OC_CFLAG_UPDATE) > 0) ||
          ((cflags & OC_CFLAG_READ) > 0)) {

        for (int i = 0; i < nr_entries; i++) {
          uint32_t grpid = oc_find_grpid_in_table(
            g_gpt, oc_core_get_publisher_table_size(), g_got[index].ga[i]);

          PRINT(" oc_register_group_multicasts index=%d i=%d grpid: %u "
                "group_address: %d cflags=",
                index, i, grpid, g_got[index].ga[i]);
          oc_print_cflags(cflags);

          if (grpid > 0) {
            subscribe_group_to_multicast_with_port(grpid, installation_id, 2,
                                                   port);
            subscribe_group_to_multicast_with_port(grpid, installation_id, 5,
                                                   port);
          }
        }
      }
    }
  }
  if (pub_entry == false) {
    // register via group object table
    int index;
    for (index = 0; index < GOT_MAX_ENTRIES; index++) {
      int nr_entries = g_got[index].ga_len;

      oc_cflag_mask_t cflags = g_got[index].cflags;
      // check if the group address is used for receiving.
      // e.g. WRITE or UPDATE
      if (((cflags & OC_CFLAG_WRITE) > 0) || ((cflags & OC_CFLAG_UPDATE) > 0) ||
          ((cflags & OC_CFLAG_READ) > 0)) {

        for (int i = 0; i < nr_entries; i++) {
          PRINT(
            " oc_register_group_multicasts index=%d i=%d group: %u  cflags=",
            index, i, g_got[index].ga[i]);
          oc_print_cflags(cflags);
          subscribe_group_to_multicast_with_port(g_got[index].ga[i],
                                                 installation_id, 2, port);
          subscribe_group_to_multicast_with_port(g_got[index].ga[i],
                                                 installation_id, 5, port);
        }
      }
    }
  }
}

void
oc_init_datapoints_at_initialization()
{
  int index;
  PRINT("oc_init_datapoints_at_initialization\n");

  for (index = 0; index < GOT_MAX_ENTRIES; index++) {
    int nr_entries = g_got[index].ga_len;

    if (nr_entries > 0) {
      oc_cflag_mask_t cflags = g_got[index].cflags;
      if ((cflags & OC_CFLAG_INIT) > 0) {
        // Case 5)
        // @sender : cflags = i After device restart(power up)
        // Sent : -st r, sending association(1st assigned ga)
        PRINT("oc_init_datapoints_at_initialization: index: %d issue read on "
              "group address %d\n",
              index, g_got[index].ga[0]);
        oc_do_s_mode_read(g_got[index].ga[0]);
      }
    }
  }
}
