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
#include "api/oc_knx_fp.h"
#include "oc_discovery.h"
#include "oc_core_res.h"
#include <stdio.h>

#define TAGS_AS_STRINGS

#define GOT_STORE "GOT_STORE"
#define GPT_STORE "GPT_STORE"
#define GRT_STORE "GRT_STORE"

#ifdef OC_PUBLISHER_TABLE
#define GPT_STORE "GPT_STORE"
#endif /* OC_PUBLISHER_TABLE */

/**
 * @brief Group Address Mapping Table Resource
 *      data for mapping between KNX-IOT and KNX-classic
 *
 * array of objects (as json)
 * [
 *  {
 *    "ga": 2305,
 *    "dpt": "1"
 *  },
 *  {
 *    "ga": 2306,
 *    "dpt": "5.1",
 *    "s": {
 *       "ga": 1306,
 *       "groupkey": "<key2>",
 *       "secSettings": {
 *        "a": true,
 *        "c": true
 *       }
 *    }
 *  }
 * ]
 */
typedef struct oc_group_address_mapping_table_t
{
  int ga;          ///< KNX IoT group address, "ga" - 7
  oc_string_t dpt; ///< Datapoint type, "dpt" 116

  int s_ga;                ///< KNX Classic group address  "ga" - 7
  oc_string_t s_group_key; ///< KNX secure shared group key "groupKey" - 107
  bool
    s_secSettings_a; ///< "a" - 97 The field determine if authentication shall
                     ///< be applied for KNX Classic secure group communication.
  bool
    s_secSettings_b; ///< "c" - 99 The field determine if authentication shall
                     ///< be applied for KNX Classic secure group communication.
} oc_group_address_mapping_table_t;

#define GAMT_MAX_ENTRIES 20
static oc_group_address_mapping_table_t g_groups[GAMT_MAX_ENTRIES];

#define GOT_MAX_ENTRIES 20
static oc_group_object_table_t g_got[GOT_MAX_ENTRIES];

#ifdef OC_PUBLISHER_TABLE
#define GPT_MAX_ENTRIES 20
static oc_group_rp_table_t g_gpt[GPT_MAX_ENTRIES];
#endif /* OC_PUBLISHER_TABLE */

#define GRT_MAX_ENTRIES 20
static oc_group_rp_table_t g_grt[GRT_MAX_ENTRIES];

// -----------------------------------------------------------------------------

static void oc_print_group_rp_table_entry(int entry, char *Store,
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

// -----------------------------------------------------------------------------

static int
table_find_id_from_rep(oc_rep_t *object)
{
  int id = -1;
  while (object != NULL) {
    switch (object->type) {
    case OC_REP_INT: {
#ifdef TAGS_AS_STRINGS
      if ((oc_string_len(object->name) == 2 &&
           memcmp(oc_string(object->name), "id", 2) == 0)) {
        id = object->value.integer;
        PRINT("  table_find_id_from_rep id=%d \n", id);
        return id;
      }
#endif
      if (oc_string_len(object->name) == 0 && object->iname == 0) {
        id = object->value.integer;
        PRINT(" table_find_id_from_rep id=%d \n", id);
        return id;
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    } /* switch */
  }   /* while */
  PRINT("  table_find_id_from_rep ERR: id=%d \n", id);
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
    if (g_got[i].ga_len == 0) {
      return i;
    }
  }
  return -1;
}

int
oc_core_set_group_object_table(int index, oc_group_object_table_t entry)
{
  g_got[index].cflags = entry.cflags;
  g_got[index].id = entry.id;

  oc_free_string(&g_got[index].href);
  oc_new_string(&g_got[index].href, oc_string(entry.href),
                oc_string_len(entry.href));
  /* copy the ga array */
  g_got[index].ga_len = entry.ga_len;
  int *new_array = (int *)malloc(entry.ga_len * sizeof(int));

  if (new_array != NULL) {
    for (int i = 0; i < entry.ga_len; i++) {
      new_array[i] = entry.ga[i];
    }
    if (g_got[index].ga != 0) {
      free(g_got[index].ga);
    }
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
oc_core_find_group_object_table_index(int group_address)
{
  int i, j;
  for (i = 0; i < GOT_MAX_ENTRIES; i++) {

    if (g_got[i].ga_len != 0) {
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
oc_core_find_next_group_object_table_index(int group_address, int cur_index)
{
  if (cur_index == -1) {
    return -1;
  }

  int i, j;
  for (i = cur_index + 1; i < GOT_MAX_ENTRIES; i++) {

    if (g_got[i].ga_len != 0) {
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
oc_core_find_group_object_table_url(char *url)
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
oc_core_find_next_group_object_table_url(char *url, int cur_index)
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

static bool
oc_belongs_href_to_resource(oc_string_t href, size_t device_index)
{

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
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
  PRINT("oc_core_fp_g_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  /* example entry: </fp/g/1>;ct=60   (cbor)*/
  for (i = 0; i < GOT_MAX_ENTRIES; i++) {

    if (g_got[i].ga_len > 0) {
      // index  in use
      PRINT("  . adding %d\n", i);
      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("<fp/g/");
      response_length += length;
      char string[10];
      sprintf((char *)&string, "%d>", g_got[i].id);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=60");
      response_length += length;
    }
  }

  if (response_length > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_g_get_handler - end\n");
}

static bool
oc_fp_p_check_and_save(int index, size_t device_index, bool status_ok)
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
  if (oc_belongs_href_to_resource(g_got[index].href, device_index) == false) {
    do_save = false;
    OC_ERR("  index %d href does not belong to device '%s'", index,
           oc_string(g_got[index].href));
  }

  oc_print_group_object_table_entry(index);
  if (do_save) {
    oc_dump_group_object_table_entry(index);
  } else {
    OC_DBG("Deleting %s", index);
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

  PRINT("oc_core_fp_g_post_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  if (oc_knx_lsm_state(device_index) != LSM_S_LOADING) {
    OC_ERR(" not in loading state\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  char buffer[200];
  memset(buffer, 200, 1);
  oc_rep_to_json(request->request_payload, (char *)&buffer, 200, true);
  PRINT("%s", buffer);
  int index = -1;
  int id;
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {

      // find id and the storage index for this object
      oc_rep_t *object = rep->value.object;
      id = table_find_id_from_rep(object);
      if (id == -1) {
        OC_ERR("  ERROR id %d", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      index = find_empty_slot_in_group_object_table(id);
      if (index == -1) {
        OC_ERR("  ERROR index %d", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      PRINT("  storing at index: %d\n", index);
      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {
        case OC_REP_STRING: {
          if (object->iname == 11) {
            oc_free_string(&g_got[index].href);
            oc_new_string(&g_got[index].href, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;
        case OC_REP_INT: {
          if (oc_string_len(object->name) == 0 && object->iname == 0) {
            // id (0)
            g_got[index].id = object->value.integer;
          }
          if (oc_string_len(object->name) == 0 && object->iname == 8) {
            // cflags (8)
            g_got[index].cflags = object->value.integer;
          }
        } break;
        case OC_REP_INT_ARRAY: {
          if (object->iname == 8) {
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
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            int *new_array = (int *)malloc(array_size * sizeof(int));

            for (int i = 0; i < array_size; i++) {
              new_array[i] = arr[i];
            }
            if (g_got[index].ga != 0) {
              free(g_got[index].ga);
            }
            g_got[index].ga_len = array_size;
            g_got[index].ga = new_array;
          }
        } break;
        default:
          break;
        } // switch

        object = object->next;
      } // next object in array
      status_ok = oc_fp_p_check_and_save(index, device_index, status_ok);
    }
    default:
      break;
    } // object level
    rep = rep->next;
  } // top level

  PRINT("oc_core_fp_g_post_handler - end\n");
  if (status_ok) {
    oc_knx_increase_fingerprint();
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
  }
  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_fp_g_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_g_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/g", OC_IF_C | OC_IF_B,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_g_get_handler, 0,
                            oc_core_fp_g_post_handler, 0, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_g_x_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_g_x_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index = oc_core_find_index_in_group_object_table_from_id(id);
  PRINT("  id=%d index = %d\n", id, index);
  if (index >= GOT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  if (&g_got[index].ga_len == 0) {
    // it is empty
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
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

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index = oc_core_find_index_in_group_object_table_from_id(id);
  PRINT("  id=%d index = %d\n", id, index);

  size_t device_index = request->resource->device;
  if (oc_knx_lsm_state(device_index) != LSM_S_LOADING) {
    PRINT(" not in loading state\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  PRINT(" deleting %d\n", index);

  if (index >= GOT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  // delete the entry
  oc_delete_group_object_table_entry(index);

  // make the deletion persistent
  oc_dump_group_object_table_entry(index);

  PRINT("oc_core_fp_g_x_del_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_DELETED);
}

void
oc_create_fp_g_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_g_x_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/g/*", OC_IF_D | OC_IF_C,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_g_x_get_handler, 0, 0,
                            oc_core_fp_g_x_del_handler, 0, 1, "urn:knx:if.c");
}

// ----------------------------------------------------------------------------

#ifdef OC_PUBLISHER_TABLE

int
oc_core_find_publisher_table_index(int group_address)
{
  int i, j;
  for (i = 0; i < GAMT_MAX_ENTRIES; i++) {

    if (g_gpt[i].ga_len != 0) {
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
  PRINT("oc_core_fp_p_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /* example entry: </fp/p/1>;ct=60 */
  for (i = 0; i < GPT_MAX_ENTRIES; i++) {

    if (g_gpt[i].ga_len != 0) {
      // index  in use

      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("<fp/p/");
      response_length += length;
      char string[10];
      sprintf((char *)&string, "%d>", g_gpt[i].id);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=60");
      response_length += length;
    }
  }

  if (response_length > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_p_get_handler - end\n");
}

static void
oc_core_fp_p_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;

  PRINT("oc_core_fp_p_post_handler\n");

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  char buffer[200];
  memset(buffer, 200, 1);
  oc_rep_to_json(request->request_payload, (char *)&buffer, 200, true);
  PRINT("%s", buffer);

  int index = -1;
  int id;
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      /* find the storage index, e.g. for this object */
      oc_rep_t *object = rep->value.object;
      id = table_find_id_from_rep(object);
      index = find_empty_slot_in_rp_table(id, g_gpt, GPT_MAX_ENTRIES);
      if (index == -1) {
        PRINT("  ERROR index %d\n", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      g_gpt[index].id = id;

      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {
        case OC_REP_INT: {
          if (object->iname == 12) {
            g_gpt[index].ia = object->value.integer;
          }
        } break;
        case OC_REP_STRING: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 4 &&
              memcmp(oc_string(object->name), "path", 4) == 0) {
            oc_free_string(&g_gpt[index].path);
            oc_new_string(&g_gpt[index].path, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (oc_string_len(object->name) == 3 &&
              memcmp(oc_string(object->name), "url", 3) == 0) {
            oc_free_string(&g_gpt[index].url);
            oc_new_string(&g_gpt[index].url, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
#endif
          if (object->iname == 112) {
            oc_free_string(&g_gpt[index].path);
            oc_new_string(&g_gpt[index].path, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (object->iname == 10) {
            oc_free_string(&g_gpt[index].url);
            oc_new_string(&g_gpt[index].url, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;
        case OC_REP_INT_ARRAY: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 2 &&
              memcmp(oc_string(object->name), "ga", 2) == 0) {
            // g_got[index].id = object->value.integer;
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            int *new_array = (int *)malloc(array_size * sizeof(int));

            for (int i = 0; i < array_size; i++) {
              new_array[i] = arr[i];
            }
            if (g_gpt[index].ga != 0) {
              free(g_gpt[index].ga);
            }
            g_gpt[index].ga_len = array_size;
            g_gpt[index].ga = new_array;
          }
#endif
          if (object->iname == 7) {
            // g_got[index].id = object->value.integer;
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            int *new_array = (int *)malloc(array_size * sizeof(int));

            for (int i = 0; i < array_size; i++) {
              new_array[i] = arr[i];
            }
            if (g_gpt[index].ga != 0) {
              free(g_gpt[index].ga);
            }
            g_gpt[index].ga_len = array_size;
            g_gpt[index].ga = new_array;
          }
        } break;
        case OC_REP_NIL:
          break;
        default:
          break;
        }
        object = object->next;
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    }

    oc_print_group_rp_table_entry(index, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
    bool do_save = true;
    if (oc_string_len(g_gpt[index].url) > OC_MAX_URL_LENGTH) {
      // do_save = false;
      OC_ERR("  url is longer than %d \n", (int)OC_MAX_URL_LENGTH);
    }
    if (oc_string_len(g_gpt[index].path) > OC_MAX_URL_LENGTH) {
      // do_save = false;
      OC_ERR("  path is longer than %d \n", (int)OC_MAX_URL_LENGTH);
    }

    oc_print_group_rp_table_entry(index, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
    if (do_save) {
      oc_dump_group_rp_table_entry(index, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
    }

    rep = rep->next;
  };

  oc_knx_increase_fingerprint();
  PRINT("oc_core_fp_p_post_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_p_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/p", OC_IF_C | OC_IF_B,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_p_get_handler, 0,
                            oc_core_fp_p_post_handler, 0, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_p_x_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_p_x_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index =
    oc_core_find_index_in_rp_table_from_id(id, g_gpt, GPT_MAX_ENTRIES);
  PRINT("  id:%d index = %d\n", id, index);

  if (index >= GPT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  if (g_gpt[index].ga_len == 0) {
    /* it is empty */
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  /* id 0 */
  oc_rep_i_set_int(root, 0, g_gpt[index].id);
  /* ia - 12 */
  if (g_gpt[index].ia > -1) {
    // oc_rep_i_set_text_string(root, 11, oc_string(g_gpt[index].ia));
    oc_rep_i_set_int(root, 11, g_gpt[index].ia);
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

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index =
    oc_core_find_index_in_rp_table_from_id(id, g_gpt, GPT_MAX_ENTRIES);

  if (index >= GPT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  g_gpt[index].id = 0;
  oc_free_string(&g_gpt[index].url);
  oc_new_string(&g_gpt[index].url, "", 0);
  // oc_free_int_array(g_gpt[index].ga);
  free(g_gpt[index].ga);
  g_gpt[index].ga = NULL;
  g_gpt[index].ga_len = 0;

  // make the change persistent
  oc_dump_group_rp_table_entry(index, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);

  oc_knx_increase_fingerprint();
  PRINT("oc_core_fp_p_x_del_handler - end\n");

  oc_send_cbor_response(request, OC_STATUS_DELETED);
}

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

// -----------------------------------------------------------------------------

//#ifdef OC_RECIPIENT_TABLE

static void
oc_core_fp_r_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_fp_r_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /* example entry: </fp/r/1>;ct=60 (cbor) */
  for (i = 0; i < GRT_MAX_ENTRIES; i++) {

    if (g_grt[i].ga_len != 0) {
      // index  in use

      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("<fp/r/");
      response_length += length;
      char string[10];
      sprintf((char *)&string, "%d>", g_grt[i].id);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=60");
      response_length += length;
    }
  }

  if (response_length > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_r_get_handler - end\n");
}

static void
oc_core_fp_r_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
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

  char buffer[200];
  memset(buffer, 200, 1);
  oc_rep_to_json(request->request_payload, (char *)&buffer, 200, true);
  PRINT("%s", buffer);

  int index = -1;
  int id = -1;
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;
      id = table_find_id_from_rep(object);
      if (id == -1) {
        OC_ERR("  ERROR id %d", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      index = find_empty_slot_in_rp_table(id, g_grt, GRT_MAX_ENTRIES);
      if (index == -1) {
        OC_ERR("  ERROR index %d", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      PRINT("  storing at %d\n", index);
      g_grt[index].id = id;

      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {

        case OC_REP_INT: {
          if (object->iname == 12) {
            g_grt[index].ia = object->value.integer;
          }
        } break;

        case OC_REP_STRING: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 4 &&
              memcmp(oc_string(object->name), "path", 4) == 0) {
            oc_free_string(&g_grt[index].path);
            oc_new_string(&g_grt[index].path, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (oc_string_len(object->name) == 3 &&
              memcmp(oc_string(object->name), "url", 3) == 0) {
            oc_free_string(&g_grt[index].url);
            oc_new_string(&g_grt[index].url, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
#endif
          if (object->iname == 112) {
            oc_free_string(&g_grt[index].path);
            oc_new_string(&g_grt[index].path, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
          if (object->iname == 10) {
            oc_free_string(&g_grt[index].url);
            oc_new_string(&g_grt[index].url, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;
        case OC_REP_INT_ARRAY: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 2 &&
              memcmp(oc_string(object->name), "ga", 2) == 0) {
            // g_got[index].id = object->value.integer;
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            int *new_array = (int *)malloc(array_size * sizeof(int));

            for (int i = 0; i < array_size; i++) {
              new_array[i] = arr[i];
            }
            if (g_grt[index].ga != 0) {
              free(g_grt[index].ga);
            }
            g_grt[index].ga_len = array_size;
            g_grt[index].ga = new_array;
          }
#endif
          if (object->iname == 7) {
            // g_got[index].id = object->value.integer;
            int64_t *arr = oc_int_array(object->value.array);
            int array_size = (int)oc_int_array_size(object->value.array);
            int *new_array = (int *)malloc(array_size * sizeof(int));

            for (int i = 0; i < array_size; i++) {
              new_array[i] = arr[i];
            }
            if (g_grt[index].ga != 0) {
              free(g_grt[index].ga);
            }
            g_grt[index].ga_len = array_size;
            g_grt[index].ga = new_array;
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
      oc_dump_group_rp_table_entry(index, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
    }
    rep = rep->next;
  };

  oc_knx_increase_fingerprint();

  PRINT("oc_core_fp_r_post_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_r_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/r", OC_IF_C | OC_IF_B,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_r_get_handler, 0,
                            oc_core_fp_r_post_handler, 0, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_r_x_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_r_x_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_CBOR) {
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

  if (index >= GRT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  if (g_grt[index].ga_len == 0) {
    // it is empty
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_grt[index].id);
  // ia - 12
  oc_rep_i_set_int(root, 11, g_grt[index].ia);
  // path- 112
  oc_rep_i_set_text_string(root, 112, oc_string(g_grt[index].path));
  // url- 10
  oc_rep_i_set_text_string(root, 10, oc_string(g_grt[index].url));
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

  int id = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  int index =
    oc_core_find_index_in_rp_table_from_id(id, g_grt, GRT_MAX_ENTRIES);

  if (index >= GRT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  PRINT("oc_core_fp_r_x_del_handler: deleting id %d at index %d\n", id, index);

  g_grt[index].id = 0;
  oc_free_string(&g_grt[index].url);
  oc_new_string(&g_grt[index].url, "", 0);
  // oc_free_int_array(g_grt[index].ga);
  free(g_grt[index].ga);
  g_grt[index].ga = NULL;
  g_grt[index].ga_len = 0;

  // make the change persistent
  oc_dump_group_rp_table_entry(index, GRT_STORE, g_grt, GRT_MAX_ENTRIES);

  oc_knx_increase_fingerprint();

  PRINT("oc_core_fp_r_x_del_handler - end\n");

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_r_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_x_resource\n");
  oc_core_populate_resource(resource_idx, device, "/fp/r/*", OC_IF_D | OC_IF_C,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_fp_r_x_get_handler, 0, 0,
                            oc_core_fp_r_x_del_handler, 0, 1, "urn:knx:if.c");
}

bool
oc_core_check_recipient_index_on_group_address(int index, int group_address)
{
  if (index >= GRT_MAX_ENTRIES) {
    return -1;
  }
  for (int i = 0; g_grt[index].ga_len; i++) {
    if (g_grt[index].ga[i] == group_address) {
      return true;
    }
  }
  return false;
}

int
oc_core_get_recipient_ia(int index)
{
  if (index >= GRT_MAX_ENTRIES) {
    return -1;
  }

  return g_grt[index].ia;
}

char *
oc_core_get_recipient_index_url_or_path(int index)
{
  if (index >= GRT_MAX_ENTRIES) {
    return NULL;
  }

  if (g_grt[index].ia > -1) {
    PRINT("oc_core_get_recipient_index_url_or_path: ia %d\n", g_grt[index].ia);
    if (oc_string_len(g_grt[index].path) > 0) {
      PRINT("      oc_core_get_recipient_index_url_or_path path %s\n",
            oc_string(g_grt[index].path));
      return oc_string(g_grt[index].path);

    } else {
      // do .knx
      PRINT("      oc_core_get_recipient_index_url_or_path (default) %s\n",
            ".knx");
      return ".knx";
    }

  } else {
    if (oc_string_len(g_grt[index].url) > 0) {
      //
      PRINT("      oc_core_get_recipient_index_url_or_path url %s\n",
            oc_string(g_grt[index].url));
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
  // PRINT("  GOT [%d] -> %d\n", entry, g_got[entry].ga_len);
  if (g_got[entry].ga_len == 0) {
    return;
  }

  PRINT("    id (0)     : %d\n", g_got[entry].id);
  PRINT("    href (11)  : %s\n", oc_string(g_got[entry].href));
  PRINT("    cflags (8) : %d string: ", g_got[entry].cflags);
  oc_print_cflags(g_got[entry].cflags);
  PRINT("    ga (7)     : [");
  for (int i = 0; i < g_got[entry].ga_len; i++) {
    PRINT(" %d", g_got[entry].ga[i]);
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

void
oc_load_group_object_table_entry(int entry)
{
  long ret = 0;
  char filename[20];
  snprintf(filename, 20, "%s_%d", GOT_STORE, entry);

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
            int *new_array = (int *)malloc(array_size * sizeof(int));

            // TODO check  if new array is not NULL
            for (int i = 0; i < array_size; i++) {
              new_array[i] = arr[i];
            }
            if (g_got[entry].ga != 0) {
              free(g_got[entry].ga);
            }
            PRINT("  ga size %d\n", array_size);
            g_got[entry].ga_len = array_size;
            g_got[entry].ga = new_array;
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
oc_free_group_object_table_entry(int entry)
{
  g_got[entry].id = -1;
  oc_free_string(&g_got[entry].href);
  // oc_new_string(&g_got[entry].href, "", 0);
  free(g_got[entry].ga);
  g_got[entry].ga = NULL;
  // oc_free_int_array(g_got[value].ga);
  g_got[entry].ga_len = 0;
  g_got[entry].cflags = 0;
}

void
oc_delete_group_object_table_entry(int entry)
{
  oc_free_group_object_table_entry(entry);
}

void
oc_delete_group_object_table()
{
  PRINT("Deleting Group Object Table from Persistent storage\n");
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    oc_delete_group_object_table_entry(i);
    oc_print_group_object_table_entry(i);
    oc_dump_group_object_table_entry(i);
  }
}

void
oc_free_group_object_table()
{
  PRINT("Free Group Object Table\n");
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    oc_free_group_object_table_entry(i);
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
  if (rp_table[entry].ga_len == 0) {
    return;
  }
  PRINT("  %s [%d] --> [%d]\n", Store, entry, rp_table[entry].ga_len);
  PRINT("    id (0)     : %d\n", rp_table[entry].id);
  PRINT("    ia (12)    : %d\n", rp_table[entry].ia);
  PRINT("    path (112) : '%s'\n", oc_string(rp_table[entry].path));
  PRINT("    url (10)   : '%s'\n", oc_string(rp_table[entry].url));
  // PRINT("    cflags  %d\n", rp_table[entry].cflags);
  PRINT("    ga (7)     : [");
  for (int i = 0; i < rp_table[entry].ga_len; i++) {
    PRINT(" %d", rp_table[entry].ga[i]);
  }
  PRINT(" ]\n");
}

static void
oc_dump_group_rp_table_entry(int entry, char *Store,
                             oc_group_rp_table_t *rp_table, int max_size)
{
  (void)max_size;
  char filename[20];
  snprintf(filename, 20, "%s_%d", Store, entry);
  // PRINT("oc_dump_group_rp_table_entry %s, fname=%s\n", Store, filename);

  uint8_t *buf = malloc(OC_MAX_APP_DATA_SIZE);
  if (!buf) {
    PRINT("oc_dump_group_rp_table_entry: no allocated mem\n");
    return;
  }

  oc_rep_new(buf, OC_MAX_APP_DATA_SIZE);
  // write the data
  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, rp_table[entry].id);
  // href- 11
  oc_rep_i_set_int(root, 12, rp_table[entry].ia);
  // href- 11
  oc_rep_i_set_text_string(root, 112, oc_string(rp_table[entry].path));
  // href- 11
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
          break;
        case OC_REP_INT_ARRAY:
          if (rep->iname == 7) {
            int64_t *arr = oc_int_array(rep->value.array);
            int array_size = (int)oc_int_array_size(rep->value.array);
            int *new_array = (int *)malloc(array_size * sizeof(int));
            if (new_array != 0) {
              for (int i = 0; i < array_size; i++) {
                new_array[i] = arr[i];
              }
            }
            if (rp_table[entry].ga != 0) {
              free(rp_table[entry].ga);
            }
            PRINT("  ga size %d\n", array_size);
            rp_table[entry].ga_len = array_size;
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
  for (int i = 0; i < GPT_MAX_ENTRIES; i++) {
    oc_load_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
    oc_print_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
  }
#endif /* OC_PUBLISHER_TABLE */
}

static void
oc_free_group_rp_table_entry(int entry, char *Store,
                             oc_group_rp_table_t *rp_table, int max_size)
{
  (void)max_size;
  (void)Store;
  rp_table[entry].id = -1;
  rp_table[entry].ia = -1;
  oc_free_string(&rp_table[entry].path);
  oc_free_string(&rp_table[entry].url);
  free(rp_table[entry].ga);
  rp_table[entry].ga = NULL;

  rp_table[entry].ga_len = 0;
  // rp_table[entry].cflags = 0;
}

static void
oc_delete_group_rp_table_entry(int entry, char *Store,
                               oc_group_rp_table_t *rp_table, int max_size)
{
  oc_free_group_rp_table_entry(entry, Store, rp_table, max_size);
}

void
oc_delete_group_rp_table()
{
  PRINT("Deleting Group Recipient Table from Persistent storage\n");
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    oc_delete_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
    oc_print_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
    oc_dump_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
  }

#ifdef OC_PUBLISHER_TABLE
  PRINT("Deleting Group Publisher Table from Persistent storage\n");
  for (int i = 0; i < GPT_MAX_ENTRIES; i++) {
    oc_delete_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
    oc_print_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
    oc_dump_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
  }
#endif /*  OC_PUBLISHER_TABLE */
}
void
oc_free_group_rp_table()
{

  PRINT("FreeGroup Recipient Table from Persistent storage\n");
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    oc_free_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
  }

#ifdef OC_PUBLISHER_TABLE
  PRINT("Deleting Group Publisher Table from Persistent storage\n");
  for (int i = 0; i < GPT_MAX_ENTRIES; i++) {
    oc_free_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
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
    if (rp_table[i].ga_len == 0) {
      return i;
    }
  }
  return -1;
}

int
oc_core_get_recipient_table_size()
{
  return GRT_MAX_ENTRIES;
}

#ifdef OC_PUBLISHER_TABLE
int
oc_core_get_publisher_table_size()
{
  return GPT_MAX_ENTRIES;
}
#endif /* OC_PUBLISHER_TABLE */

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void
oc_init_tables()
{
#ifdef OC_PUBLISHER_TABLE
  for (int i = 0; i < GPT_MAX_ENTRIES; i++) {
    oc_delete_group_rp_table_entry(i, GPT_STORE, g_gpt, GPT_MAX_ENTRIES);
  }
#endif /* OC_PUBLISHER_TABLE */
  for (int i = 0; i < GRT_MAX_ENTRIES; i++) {
    oc_delete_group_rp_table_entry(i, GRT_STORE, g_grt, GRT_MAX_ENTRIES);
  }
  for (int i = 0; i < GOT_MAX_ENTRIES; i++) {
    oc_delete_group_object_table_entry(i);
  }
}

// -----------------------------------------------------------------------------

void
oc_create_knx_fp_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_fp_resources");

  oc_create_fp_g_resource(OC_KNX_FP_G, device_index);
  oc_create_fp_g_x_resource(OC_KNX_FP_G_X, device_index);

#ifdef OC_PUBLISHER_TABLE
  oc_create_fp_p_resource(OC_KNX_FP_P, device_index);
  oc_create_fp_p_x_resource(OC_KNX_FP_P_X, device_index);
#endif /* OC_PUBLISHER_TABLE */

  oc_create_fp_r_resource(OC_KNX_FP_R, device_index);
  oc_create_fp_r_x_resource(OC_KNX_FP_R_X, device_index);

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
is_in_array(int value, int *array, int array_size)
{
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
                                                int group_address,
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
    if (g_got[index].ga_len > 0) {
      if (is_in_array(group_address, g_got[index].ga, g_got[index].ga_len)) {
        // add the resource
        // note, not checked if the resource is already there...
        PRINT("oc_add_points_in_group_object_table_to_response [%d] %s\n",
              index, oc_string(g_got[index].href));
        oc_add_resource_to_wk(oc_ri_get_app_resource_by_uri(
                                oc_string(g_got[index].href),
                                oc_string_len(g_got[index].href), device_index),
                              request, device_index, response_length, matches);
        return_value = true;
      }
    }
  }
  return return_value;
}

oc_endpoint_t
oc_create_multicast_group_address(oc_endpoint_t in, int group_nr, int iid,
                                  int scope)
{
  // create the multicast address from group and scope
  // FF35::30: <ULA-routing-prefix>::<group id>
  //    | 5 == scope
  //   | 3 == multicast

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

  int my_transport_flags = 0;
  my_transport_flags += IPV6;
  my_transport_flags += MULTICAST;
  my_transport_flags += DISCOVERY;
#ifdef OC_OSCORE
  my_transport_flags += OSCORE;
#endif

  oc_make_ipv6_endpoint(group_mcast, my_transport_flags, 5683, 0xff,
                        0x30 + scope, ula_4, ula_3, ula_2, ula_1, 0, 0, 0, 0, 0,
                        0, byte_4, byte_3, byte_2, byte_1);
  PRINT("  oc_create_multicast_group_address S=%d U=%d G=%d B4=%d B3=%d B2=%d "
        "B1=%d\n",
        scope, iid, group_nr, byte_4, byte_3, byte_2, byte_1);
  PRINT("  ");
  PRINTipaddr(group_mcast);
  PRINT("\n");
  memcpy(&in, &group_mcast, sizeof(oc_endpoint_t));

  return in;
}

void
subscribe_group_to_multicast(int group_nr, int iid, int scope)
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
oc_register_group_multicasts()
{
  // installation id will be used as ULA prefix
  oc_device_info_t *device = oc_core_get_device_info(0);
  uint32_t installation_id = device->iid;

  PRINT("oc_register_group_multicasts\n");

  int index;
  for (index = 0; index < GOT_MAX_ENTRIES; index++) {
    int nr_entries = g_got[index].ga_len;

    oc_cflag_mask_t cflags = g_got[index].cflags;
    // check if the group address is used for receiving.
    // e.g. WRITE or UPDATE
    if (((cflags & OC_CFLAG_WRITE) > 0) || ((cflags & OC_CFLAG_UPDATE) > 0) ||
        ((cflags & OC_CFLAG_READ) > 0)) {

      for (int i = 0; i < nr_entries; i++) {
        PRINT(" oc_register_group_multicasts index=%d i=%d group: %d  cflags=",
              index, i, g_got[index].ga[i]);
        oc_print_cflags(cflags);
        subscribe_group_to_multicast(g_got[index].ga[i], installation_id, 2);
        subscribe_group_to_multicast(g_got[index].ga[i], installation_id, 5);
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
        oc_do_s_mode_read(g_got[index].ga[0]);
      }
    }
  }
}
