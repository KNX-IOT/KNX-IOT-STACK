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
#include "api/oc_knx_p.h"
#include "api/oc_knx_fp.h"

#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>

// -----------------------------------------------------------------------------

bool
oc_add_data_points_to_response(oc_request_t *request, size_t device_index,
                               size_t *response_length, int matches)
{
  (void)request;
  int length = 0;

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
    }
    oc_add_resource_to_wk(resource, request, device_index, response_length,
                          matches);
    matches++;
  }

  if (matches > 0) {
    return true;
  }

  return false;
}

/*
 * return list of function blocks
 */
static void
oc_core_p_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                      void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int matches = 0;

  PRINT("oc_core_p_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  bool added = oc_add_data_points_to_response(request, device_index,
                                              &response_length, matches);

  if (added) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_p_get_handler - end\n");
}

/*
 * return list of function blocks
 */
static void
oc_core_p_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                       void *data)
{
  (void)data;
  oc_rep_t *rep = NULL;
  bool error = false;

  PRINT("oc_core_p_post_handler\n");

  /* check if the accept header is cbor */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  size_t device_index = request->resource->device;

  /*  check if the url are implemented on the device*/
  rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_OBJECT) {
      oc_rep_t *entry = rep->value.object;
      while (entry != NULL) {
        // href == 11
        if ((entry->iname == 11) && (entry->type == OC_REP_STRING)) {

          if (oc_belongs_href_to_resource(entry->value.string, device_index) ==
              false) {
            error = true;
            OC_ERR("href '%.*s' does not belong to device",
                   (int)oc_string_len(entry->value.string),
                   oc_string(entry->value.string));
          }
        }
        entry = entry->next;
      }
    }
    rep = rep->next;
  }
  if (error) {
    PRINT("oc_core_p_post_handler - end\n");
    oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  oc_string_t *myurl;
  oc_rep_t *value;
  rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_OBJECT) {
      oc_rep_t *entry = rep->value.object;
      myurl = NULL;
      value = NULL;
      while (entry != NULL) {
        // href == 11
        if ((entry->iname == 11) && (entry->type == OC_REP_STRING)) {
          myurl = &entry->value.string;
        }
        if (entry->iname == 1) {
          value = entry;
        }
        if (value && myurl) {
          // do the post..
          oc_request_t new_request;
          memcpy(&new_request, request, sizeof(request));
          new_request.request_payload = value;
          new_request.response = NULL;

          oc_response_buffer_t response_buffer;
          oc_response_t response_obj;

          /* Postpone allocating response_state right after calling
           * oc_parse_rep()
           *  in order to reducing peak memory in OC_BLOCK_WISE &
           * OC_DYNAMIC_ALLOCATION
           */
          response_buffer.code = 0;
          response_buffer.response_length = 0;
          response_buffer.content_format = 0;
          response_buffer.max_age = 0;

          response_obj.separate_response = NULL;
          response_obj.response_buffer = &response_buffer;
          new_request.response = &response_obj;
          new_request.uri_path = ".knx";
          new_request.uri_path_len = 4;

          oc_resource_t *my_resource = oc_ri_get_app_resource_by_uri(
            oc_string(*myurl), oc_string_len(*myurl), device_index);
          if (my_resource) {
            // this should not be the request..
            if (my_resource->post_handler.cb) {
              my_resource->post_handler.cb(&new_request, iface_mask, NULL);
            }
          }
        }
        entry = entry->next;
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_OK);
  PRINT("oc_core_p_post_handler - end\n");
}

void
oc_create_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_p_resource\n");
  // note that this resource is listed in /.well-known/core so it should have
  // the full rt with urn:knx prefix
  oc_core_populate_resource(resource_idx, device, "/p", OC_IF_LI,
                            APPLICATION_LINK_FORMAT, 0, oc_core_p_get_handler,
                            0, oc_core_p_post_handler, 0, 1, "urn:knx:fb.0");
}

void
oc_create_knx_p_resources(size_t device_index)
{

  oc_create_p_resource(OC_KNX_P, device_index);
}
