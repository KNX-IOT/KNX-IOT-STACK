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
#include "oc_core_res.h"
#include <stdio.h>

#define TAGS_AS_STRINGS

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
oc_group_address_mapping_table_t g_groups[GAMT_MAX_ENTRIES];

/**
 * @brief cflag masks
 *
 *
 *
 */
typedef enum {
  OC_CFLAG_NONE = 0,       ///< Communication
  OC_CFLAG_READ = 1 << 1,  ///< false = Group Object value cannot be read.
  OC_CFLAG_WRITE = 1 << 2, ///< false = Group Object value cannot be written
  OC_CFLAG_TRANSMISSION =
    1 << 3,                 ///< false = Group Object value is not transmitted.
  OC_CFLAG_UPDATE = 1 << 4, ///< false = Group Object value is not updated.
  OC_CFLAG_INIT = 1 << 5    ///< false = Disable read after initialization.
} oc_cflag_mask_t;

/**
 * @brief Group Object Table Resource (/fp/g)
 *
 * array of objects (as json)
 * [
 *    {
 *        "id": "1",
 *        "href":"/LDSB1/SOO",
 *        "ga":[2305, 2401],
 *        "cflag":["r","w","t","u"]
 *    },
 *    {
 *        "id": "2",
 *        "href":"/LDSB1/RSC",
 *        "ga":[2306],
 *        "cflag":["t"]
 *     }
 * ]
 *
 * cflag translation
 * | string | Integer Value |
 * | ------ | ------------- |
 * | r      | 1             |
 * | w      | 2             |
 * | t      | 3             |
 * | u      | 4             |
 * | i      | 5             |

 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | id       | 0             |
 * | href     | 11            |
 * | ga       | 7             |
 * | cflag    | 8             |
 *
 */
typedef struct oc_group_object_table_t
{
  int id;                 ///< contents of id
  oc_string_t href;       ///<  contents of href
  int *ga;                ///< array of integers
  int ga_len;             //< length of the array of ga identifiers
  oc_cflag_mask_t cflags; ///< contents of cflags as bitmap
} oc_group_object_table_t;

#define GOT_MAX_ENTRIES 20
oc_group_object_table_t g_got[GOT_MAX_ENTRIES];

/**
 * @brief Function point Recipient - Publisher Table Resource (/fp/r) (/fp/p)
 *
 * the same table is used for recipient and publisher.
 * the only difference is the confirmable/not confirmable sending.
 *
 * array of objects (as json)
 * [
 *    {
 *        "id": "1",
 *        ia": "<knx-installation-id>.<recipientís IA>",
 *        "ga":[2305, 2401],
 *        "path": ".knx",
 *    },
 *    {
 *        "id": "2",
 *        "url": "coap://<IP multicast, unicast address or fqdn>/<path>",
 *        "ga": [2305, 2306, 2307, 2308]
 *     }
 * ]
 *
 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | id       | 0             |
 * | ia       | 12            |
 * | path     | 112           |
 * | url      | 10            |
 * | ga       | 7             |
 * | con      | -             |
 *
 */
typedef struct oc_group_rp_table_t
{
  int id;           ///< contents of id
  oc_string_t ia;   ///< contents of ia
  oc_string_t path; ///< contents of path
  oc_string_t url;  ///< contents of url
  bool con;         ///< confirmed message, default = false
  int *ga;          ///< array of integers
  int ga_len;       //< length of the array of ga identifiers
} oc_group_rp_table_t;

#define GPT_MAX_ENTRIES 20
oc_group_rp_table_t g_gpt[GPT_MAX_ENTRIES];

#define GRT_MAX_ENTRIES 20
oc_group_rp_table_t g_grt[GRT_MAX_ENTRIES];

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
  PRINT("oc_core_fp_gm_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /* example entry: </fp/gm/1>;ct=50 */
  for (i = 0; i < GAMT_MAX_ENTRIES; i++) {
    if (i > 0) {
      length = oc_rep_add_line_to_buffer(",\n");
      response_length += length;
    }
    if (oc_string_len(g_groups[i].dpt) == 0) {
      // index not in use
      break;
    }

    length = oc_rep_add_line_to_buffer("<fp/gm/");
    response_length += length;
    char string[10];
    sprintf((char *)&string, "%d", i + 1);
    length = oc_rep_add_line_to_buffer(string);
    response_length += length;

    length = oc_rep_add_line_to_buffer(";ct=50");
    response_length += length;
  }

  if (response_length > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_gm_get_handler - end\n");
}

static void
oc_core_fp_gm_post_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  request->response->response_buffer->content_format = APPLICATION_CBOR;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_fp_gm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/gm", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_gm_get_handler, oc_core_fp_gm_post_handler, 0, 0, 0, 1,
    "urn:knx:if.c");
}

static void
oc_core_fp_gm_x_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  PRINT("oc_core_fp_gm_x_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  if (value >= GAMT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  // if ( oc_string_len(&g_groups[value - 1].dpt) == 0) {
  // it is empty
  //  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  //  return;
  //}

  oc_rep_begin_root_object();
  // ga- 7
  oc_rep_i_set_int(root, 7, g_groups[value - 1].ga);
  // dpt- 116
  oc_rep_i_set_text_string(root, 116, oc_string(g_groups[value - 1].dpt));
  // note add also classic.

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

  if (value >= GAMT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_free_string(&g_groups[value - 1].dpt);
  oc_new_string(&g_groups[value - 1].dpt, "", 0);

  PRINT("oc_core_fp_gm_x_del_handler - end\n");

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_gm_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/gm/*", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_fp_gm_x_get_handler, 0, 0,
    oc_core_fp_gm_x_del_handler, 0, 1, "urn:knx:if.c");
}

// -----------------------------------------------------------------------------

static int
find_id_storage_index(oc_rep_t *object)
{
  int index = -1;
  while (object != NULL) {
    switch (object->type) {
    case OC_REP_INT: {
#ifdef TAGS_AS_STRINGS
      if ((oc_string_len(object->name) == 2 &&
           memcmp(oc_string(object->name), "id", 2) == 0)) {
        index = object->value.integer;
        PRINT("  find_id_storage_index storing at %d \n", index);
        return index;
      }
#endif
      if (oc_string_len(object->name) == 0 && object->iname == 0) {
        index = object->value.integer;
        PRINT(" find_id_storage_index storing at %d \n", index);
        return index;
      }
    } break;
    case OC_REP_NIL:
      break;
    default:
      break;
    } /* switch */
  }   /* while */
  PRINT("  find_id_storage_index ERR: storing at %d \n", index);
  return index;
}

// -----------------------------------------------------------------------------

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

  /* example entry: </fp/g/1>;ct=50 */
  for (i = 0; i < GAMT_MAX_ENTRIES; i++) {

    if (g_got[i].ga_len != 0) {
      // index  in use

      if (response_length > 0) {
        length = oc_rep_add_line_to_buffer(",\n");
        response_length += length;
      }

      length = oc_rep_add_line_to_buffer("<fp/g/");
      response_length += length;
      char string[10];
      sprintf((char *)&string, "%d", i);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=50");
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

static void
oc_core_fp_g_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;

  PRINT("oc_core_fp_g_post_handler\n");

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
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {

      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;
      index = find_id_storage_index(object);
      if (index == -1) {
        PRINT("  ERROR index %d\n", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }

      object = rep->value.object;

      while (object != NULL) {
        switch (object->type) {
        case OC_REP_STRING: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 4 &&
              memcmp(oc_string(object->name), "href", 4) == 0) {
            oc_free_string(&g_got[index].href);
            oc_new_string(&g_got[index].href, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
#endif
          if (object->iname == 11) {
            oc_free_string(&g_got[index].href);
            oc_new_string(&g_got[index].href, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
        } break;
        case OC_REP_INT: {
#ifdef TAGS_AS_STRINGS
          if ((oc_string_len(object->name) == 2 &&
               memcmp(oc_string(object->name), "id", 2) == 0)) {
            g_got[index].id = object->value.integer;
          }
#endif
          if (oc_string_len(object->name) == 0 && object->iname == 0) {
            {
              g_got[index].id = object->value.integer;
            }
          }
          break;
        case OC_REP_INT_ARRAY: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 5 &&
              memcmp(oc_string(object->name), "cflag", 5) == 0) {
            // g_got[index].id = object->value.integer;
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
          if (oc_string_len(object->name) == 2 &&
              memcmp(oc_string(object->name), "ga", 2) == 0) {
            // g_got[index].id = object->value.integer;
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
#endif
          if (object->iname == 8) {
            // g_got[index].id = object->value.integer;
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
            // g_got[index].id = object->value.integer;
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

  PRINT("oc_core_fp_g_post_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_CHANGED);
}

void
oc_create_fp_g_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_g_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/g", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_g_get_handler, 0, oc_core_fp_g_post_handler, 0, 0, 1,
    "urn:knx:if.c");
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

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  PRINT("  index = %d\n", value);

  if (value >= GOT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  if (&g_got[value].ga_len == 0) {
    // it is empty
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_got[value].id);
  // href- 11
  oc_rep_i_set_text_string(root, 11, oc_string(g_got[value].href));
  // ga - 7
  oc_rep_i_set_int_array(root, 7, g_got[value].ga, g_got[value].ga_len);

  // cflags 8
  oc_rep_i_set_key(&root_map, 8);
  oc_rep_begin_array(&root_map, cflags);
  if (g_got[value].cflags & OC_CFLAG_READ) {
    oc_rep_add_int(cflags, 1);
  }
  if (g_got[value].cflags & OC_CFLAG_WRITE) {
    oc_rep_add_int(cflags, 2);
  }
  if (g_got[value].cflags & OC_CFLAG_TRANSMISSION) {
    oc_rep_add_int(cflags, 3);
  }
  if (g_got[value].cflags & OC_CFLAG_UPDATE) {
    oc_rep_add_int(cflags, 4);
  }
  if (g_got[value].cflags & OC_CFLAG_INIT) {
    oc_rep_add_int(cflags, 5);
  }
  oc_rep_close_array(root, cflags);

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

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  PRINT(" deleting %d\n", value);

  if (value >= GOT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  g_got[value].id = 0;
  oc_free_string(&g_got[value].href);
  oc_new_string(&g_got[value].href, "", 0);
  free(g_got[value].ga);
  g_got[value].ga = NULL;
  // oc_free_int_array(g_got[value].ga);
  g_got[value].ga_len = 0;

  PRINT("oc_core_fp_g_x_del_handler - end\n");

  oc_send_cbor_response(request, OC_STATUS_DELETED);
}

void
oc_create_fp_g_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_g_x_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/g/*", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_g_x_get_handler, 0, 0, oc_core_fp_g_x_del_handler, 0, 1,
    "urn:knx:if.c");
}

// ----------------------------------------------------------------------------

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
  /* example entry: </fp/p/1>;ct=50 */
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
      sprintf((char *)&string, "%d", i);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=50");
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
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;
      index = find_id_storage_index(object);
      if (index == -1) {
        PRINT("  ERROR index %d\n", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      g_gpt[index].id = index;

      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {
        case OC_REP_STRING: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 2 &&
              memcmp(oc_string(object->name), "ia", 2) == 0) {
            oc_free_string(&g_gpt[index].ia);
            oc_new_string(&g_gpt[index].ia, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
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
          if (object->iname == 12) {
            oc_free_string(&g_gpt[index].ia);
            oc_new_string(&g_gpt[index].ia, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
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
    rep = rep->next;
  };

  PRINT("oc_core_fp_p_post_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_p_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/p", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_p_get_handler, 0, oc_core_fp_p_post_handler, 0, 0, 1,
    "urn:knx:if.c");
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

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  PRINT("  index = %d\n", value);

  if (value >= GPT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  if (g_gpt[value].ga_len == 0) {
    // it is empty
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_gpt[value].id);
  // ia - 12
  oc_rep_i_set_text_string(root, 11, oc_string(g_gpt[value].ia));
  // path- 112
  oc_rep_i_set_text_string(root, 112, oc_string(g_gpt[value].path));
  // url- 10
  oc_rep_i_set_text_string(root, 10, oc_string(g_gpt[value].url));
  // ga - 7
  oc_rep_i_set_int_array(root, 7, g_gpt[value].ga, g_gpt[value].ga_len);

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

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  if (value >= GRT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  g_gpt[value].id = 0;
  oc_free_string(&g_gpt[value].url);
  oc_new_string(&g_gpt[value].url, "", 0);
  // oc_free_int_array(g_gpt[value].ga);
  free(g_gpt[value].ga);
  g_gpt[value].ga = NULL;
  g_gpt[value].ga_len = 0;

  PRINT("oc_core_fp_p_x_del_handler - end\n");

  oc_send_cbor_response(request, OC_STATUS_DELETED);
}

void
oc_create_fp_p_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_p_x_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/p/*", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_p_x_get_handler, 0, 0, oc_core_fp_p_x_del_handler, 0, 1,
    "urn:knx:if.c");
}

// -----------------------------------------------------------------------------

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
  /* example entry: </fp/r/1>;ct=50 */
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
      sprintf((char *)&string, "%d", i + 1);
      length = oc_rep_add_line_to_buffer(string);
      response_length += length;

      length = oc_rep_add_line_to_buffer(";ct=50");
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
  oc_rep_t *rep = request->request_payload;

  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;
      index = find_id_storage_index(object);
      if (index == -1) {
        PRINT("  ERROR index %d\n", index);
        oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
        return;
      }
      g_grt[index].id = index;

      object = rep->value.object;
      while (object != NULL) {
        switch (object->type) {
        case OC_REP_STRING: {
#ifdef TAGS_AS_STRINGS
          if (oc_string_len(object->name) == 2 &&
              memcmp(oc_string(object->name), "ia", 2) == 0) {
            oc_free_string(&g_grt[index].ia);
            oc_new_string(&g_grt[index].ia, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
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
          if (object->iname == 12) {
            oc_free_string(&g_grt[index].ia);
            oc_new_string(&g_grt[index].ia, oc_string(object->value.string),
                          oc_string_len(object->value.string));
          }
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
    rep = rep->next;
  };

  PRINT("oc_core_fp_r_post_handler - end\n");
  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_r_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/r", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_r_get_handler, 0, oc_core_fp_r_post_handler, 0, 0, 1,
    "urn:knx:if.c");
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

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  PRINT("  index = %d\n", value);

  if (value >= GRT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  if (g_grt[value].ga_len == 0) {
    // it is empty
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_rep_begin_root_object();
  // id 0
  oc_rep_i_set_int(root, 0, g_grt[value].id);
  // ia - 12
  oc_rep_i_set_text_string(root, 11, oc_string(g_grt[value].ia));
  // path- 112
  oc_rep_i_set_text_string(root, 112, oc_string(g_grt[value].path));
  // url- 10
  oc_rep_i_set_text_string(root, 10, oc_string(g_grt[value].url));
  // ga - 7
  oc_rep_i_set_int_array(root, 7, g_grt[value].ga, g_grt[value].ga_len);

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

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  if (value >= GRT_MAX_ENTRIES) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  g_grt[value].id = 0;
  oc_free_string(&g_grt[value].url);
  oc_new_string(&g_grt[value].url, "", 0);
  // oc_free_int_array(g_grt[value].ga);
  free(g_grt[value].ga);
  g_grt[value].ga = NULL;
  g_grt[value].ga_len = 0;

  PRINT("oc_core_fp_r_x_del_handler - end\n");

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_fp_r_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_x_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/r/*", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_r_x_get_handler, 0, 0, oc_core_fp_r_x_del_handler, 0, 1,
    "urn:knx:if.c");
}

// -----------------------------------------------------------------------------

/*
void
oc_create_fp_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/fp", OC_IF_LL,
APPLICATION_LINK_FORMAT, OC_DISCOVERABLE, oc_core_p_get_handler, 0,
                            oc_core_p_post_handler, 0, 1, "urn:knx:if.c");
}
*/

void
oc_create_knx_fp_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_fp_resources");

  oc_create_fp_gm_resource(OC_KNX_FP_GM, device_index);
  oc_create_fp_gm_x_resource(OC_KNX_FP_GM_X, device_index);

  oc_create_fp_g_resource(OC_KNX_FP_G, device_index);
  oc_create_fp_g_x_resource(OC_KNX_FP_G_X, device_index);

  oc_create_fp_p_resource(OC_KNX_FP_P, device_index);
  oc_create_fp_p_x_resource(OC_KNX_FP_P_X, device_index);

  oc_create_fp_r_resource(OC_KNX_FP_R, device_index);
  oc_create_fp_r_x_resource(OC_KNX_FP_R_X, device_index);

  // note: /fp does not exist..
  // oc_create_fp_resource(OC_KNX_FP, device_index);
}
