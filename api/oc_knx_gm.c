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
#include "api/oc_knx_gm.h"
#include "api/oc_knx_fp.h"
#include "oc_discovery.h"
#include "oc_core_res.h"
#include <stdio.h>

//#define OC_GM_TABLE

#ifndef G_GM_MAX_ENTRIES
#define G_GM_MAX_ENTRIES 20
#endif

int oc_core_get_gm_table_size()
{

#ifdef OC_GM_TABLE
  return 0;
#endif 
  return G_GM_MAX_ENTRIES;
}

#ifdef OC_GM_TABLE

/** the list of group mappings */
#define GM_STORE "gm_store"


oc_group_mapping_table_t g_gm_entries[G_GM_MAX_ENTRIES];


// ----------------------------------------------------------------------------

static int
find_empty_gm_index()
{
  for (int i = 0; i < oc_core_get_gm_table_size(); i++) {
    if (g_gm_entries[i].ga_len == 0) {
      return i;
    }
  }
  return -1;
}

static int
find_gm_index(int id)
{
  int len;
  for (int i = 0; i < oc_core_get_gm_table_size(); i++) {
    if (g_gm_entries[i].id == id) {
      return i;
    }
  }
  return -1;
}

int oc_core_set_gm_table(size_t device_index, int index,
                         oc_group_mapping_table_t entry, bool store)
{
  if (index > oc_core_get_gm_table_size()) {
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
  /* example entry: </fp/gm/1>;ct=60 (cbor)*/
  for (i = 0; i < oc_core_get_gm_table_size(); i++) {
    if (i > 0) {
      length = oc_rep_add_line_to_buffer(",\n");
      response_length += length;
    }
    if (g_gm_entries[i].ga_len == 0) {
      // index not in use
      break;
    }

    length = oc_rep_add_line_to_buffer("<fp/gm/");
    response_length += length;
    char string[10];
    sprintf((char *)&string, "%d", i + 1);
    length = oc_rep_add_line_to_buffer(string);
    response_length += length;

    length = oc_rep_add_line_to_buffer(";ct=60");
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
  oc_rep_t *object = NULL;
  oc_rep_t *s_object = NULL;
  oc_rep_t *sec_object = NULL;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
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
  int id = oc_table_find_id_from_rep(rep);
  if (id < 0) {
    OC_ERR(" not a valid id\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  // entry storage
  oc_group_mapping_table_t my_entry;

  int index = find_gm_index(id);
  if (index != -1) {
    PRINT("   entry already exist! \n");
  } else {
    index = find_empty_gm_index();
    if (index == -1) {
      PRINT("  no space left!\n");
      oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
      return;
    } 
 }
 PRINT("  storage index: %d (%d)\n", index, id);

 // parse the response
 object = rep->value.object;
 while (object != NULL) {
   if (object->type == OC_REP_INT_ARRAY) {
     // ga
     if (object->iname == 7) {
       int64_t *array = 0;
       size_t array_size = 0;
       // not making a deep copy
       oc_rep_i_get_int_array(object, 9, &array, &array_size);
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
         int64_t *new_array = (int64_t *)malloc(array_size * sizeof(uint64_t));
         if (new_array) {
           for (size_t i = 0; i < array_size; i++) {
             new_array[i] = array[i];
           }
           g_gm_entries[index].ga = new_array;
         } else {
           OC_ERR("out of memory");
         }
       }
     }
   } else if (object->type == OC_REP_INT) {
     if (object->iname == 116) {
       // dataType (116)
       PRINT("   profile %d\n", (int)object->value.integer);
       g_gm_entries[index].dataType = (int)object->value.integer;
     }
   } else if (object->type == OC_REP_OBJECT) {
     // level of s
     s_object = object->value.object;
     int s_object_nr = object->iname;
     PRINT("  s_object_nr %d\n", s_object_nr);
     while (s_object) {
       if (s_object->type == OC_REP_STRING) {
         if (s_object->iname == 107 && s_object_nr == 115) {
           // groupkey (115(s)::107) 
           oc_free_string(&(g_gm_entries[index].groupKey));
           oc_new_string(&g_gm_entries[index].groupKey,
                         oc_string(s_object->value.string),
                         oc_string_len(s_object->value.string));
         }
       } else if (s_object->type == OC_REP_OBJECT) {
         sec_object = s_object->value.object;
         int sec_object_nr = sec_object->iname;
         while (sec_object) {
           if (sec_object->type == OC_REP_BOOL) {
             if (sec_object->iname == 97 && s_object_nr == 115 &&
                 sec_object_nr == 28) {
               // 115:28:97
               g_gm_entries[index].authentication = s_object->value.boolean;
             }
             if (sec_object->iname == 99 && s_object_nr == 115 &&
                 sec_object_nr == 28) {
               // 115:28:97
               g_gm_entries[index].confidentiality = s_object->value.boolean;
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

  request->response->response_buffer->content_format = APPLICATION_CBOR;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_fp_gm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_populate_resource(
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
  if (value >= oc_core_get_gm_table_size()) {
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
  //id (0)
  oc_rep_i_set_int(root, 0, g_gm_entries[index].id);
  // ga (7) taking input of int64 array
  oc_rep_i_set_int_array(root, 7, g_gm_entries[index].ga,
                         g_gm_entries[index].ga_len);
  if (oc_string_len(g_gm_entries[index].groupKey) > 0) {
    // create s map (s)
    oc_rep_set_key(&root_map, "s");
    CborEncoder s_map;
    cbor_encoder_create_map(&root_map, &s_map, CborIndefiniteLength);
    // set groupKey (107)
    oc_rep_i_set_byte_string(s, 107, oc_string(g_gm_entries[index].groupKey),
      oc_string_len(g_gm_entries[index].groupKey));

    // secSetting map (28)
    oc_rep_i_set_key(&root_map, 28);
    CborEncoder secSettings_map;
    cbor_encoder_create_map(&s_map, &secSettings_map, CborIndefiniteLength);
    // add a
    oc_rep_i_set_boolean(secSettings, 0,
                         g_gm_entries[index].authentication);
    // add c
    oc_rep_i_set_boolean(secSettings, 0,
                         g_gm_entries[index].confidentiality);
    cbor_encoder_close_container_checked(&s_map, &secSettings_map);
    cbor_encoder_close_container_checked(&root_map, &s_map);
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

  if (value >= oc_core_get_gm_table_size()) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  // todo 
  // free the entries
  g_gm_entries[value - 1].ga_len = 0;

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

#endif /* OC_GM_TABLE */

// -----------------------------------------------------------------------------

void
oc_create_knx_gm_resources(size_t device_index)
{
  (void)device_index;
#ifdef OC_GM_TABLE
  OC_DBG("oc_create_knx_gm_resources");

  oc_create_fp_gm_resource(OC_KNX_FP_GM, device_index);
  oc_create_fp_gm_x_resource(OC_KNX_FP_GM_X, device_index);

#endif /* OC_GM_TABLE */
}

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