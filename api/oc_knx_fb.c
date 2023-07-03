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
#include "api/oc_knx_fb.h"
#include "api/oc_knx_fp.h"
#include "api/oc_knx_gm.h"
#include "oc_knx_helpers.h"

#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>

// -----------------------------------------------------------------------------
// note this can be optimized.
#define ARRAY_SIZE 50 // upto 50 data points in a functional block
int g_int_array[2][ARRAY_SIZE];
int g_array_size = 0;
static int g_nr_functional_blocks = 0;

int
get_fp_from_dp(char *dpt)
{
  // char *dpt = oc_string(oc_dpt);
  char *dot;

  // dpa.352.51
  // urn:knx:dpa.352
  dot = strchr(dpt, '.');
  if (dot) {
    return atoi(dot + 1);
  }

  return -1;
}

bool
is_in_g_array(int value, int instance)
{
  int i;
  for (i = 0; i < g_array_size; i++) {
    if (value == g_int_array[0][i] && instance == g_int_array[1][i]) {
      return true;
    }
  }
  return false;
}

void
store_in_array(int value, int instance)
{
  if (value == -1) {
    return;
  }
  g_int_array[0][g_array_size] = value;    // functional block number
  g_int_array[1][g_array_size] = instance; // instance of the functional block
  g_array_size++;
  // assert(g_array_size == ARRAY_SIZE);
}

// -----------------------------------------------------------------------------
static int
oc_core_count_dp_in_fb(size_t device_index, int instance, int fb_value)
{
  int counter = 0;
  int i;

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
    }
    int instance_resource = resource->fb_instance;
    oc_string_array_t types = resource->types;
    for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
      char *t = oc_string_array_get_item(types, i);
      if ((strncmp(t, ":dpa", 4) == 0) ||
          (strncmp(t, "urn:knx:dpa", 11) == 0)) {
        int fp_int = get_fp_from_dp(t);
        if (fp_int == fb_value && instance_resource == instance) {
          counter++;
        }
      }
    }
  }
  return counter;
}

static void
oc_core_fb_x_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int matches = 0;
  int length;
  bool ps_exists;
  bool total_exists;
  PRINT("oc_core_fb_x_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

#ifdef OC_IOT_ROUTER
  const char *value;
  int value_len = oc_uri_get_wildcard_value_as_string(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len, &value);
  if ((value_len == 5) && (strncmp(value, "netip", 5) == 0)) {
    oc_core_f_netip_get_handler(request, iface_mask, data);
    return;
  }
#endif /* OC_IOT_ROUTER */

  // if instance is not set, it is instance 0
  int instance = 0;
  int fb_value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  PRINT("  fb_value: %d\n", fb_value);
  PRINT("  resource url: %s\n", oc_string(request->resource->uri));
  PRINT("  request url: %.*s", (int)request->uri_path_len, request->uri_path);
  bool has_instance = oc_uri_contains_wildcard_value_underscore(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);
  if (has_instance) {
    instance = oc_uri_get_wildcard_value_as_int_after_underscore(
      oc_string(request->resource->uri), oc_string_len(request->resource->uri),
      request->uri_path, request->uri_path_len);
  }
  PRINT("  instance: %d\n", instance);
  size_t device_index = request->resource->device;

  // handle query parameters: l=ps l=total
  if (check_if_query_l_exist(request, &ps_exists, &total_exists)) {
    // example : < / fp / r / ? l = total>; total = 22; ps = 5

    length = oc_frame_query_l("/fp/gm", ps_exists, total_exists);
    response_length += length;
    if (ps_exists) {
      length = oc_rep_add_line_to_buffer(";ps=");
      response_length += length;
      length = oc_frame_integer(
        oc_core_count_dp_in_fb(device_index, instance, fb_value));
      response_length += length;
    }
    if (total_exists) {
      length = oc_rep_add_line_to_buffer(";total=");
      response_length += length;
      length = oc_frame_integer(
        oc_core_count_dp_in_fb(device_index, instance, fb_value));
      response_length += length;
    }

    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }

  // do the actual creation of the payload, e.g. the data points per functional
  // block instance
  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
    }

    bool frame_resource = false;
    int instance_resource = resource->fb_instance;

    oc_string_array_t types = resource->types;
    for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
      char *t = oc_string_array_get_item(types, i);
      if ((strncmp(t, ":dpa", 4) == 0) ||
          (strncmp(t, "urn:knx:dpa", 11) == 0)) {
        int fp_int = get_fp_from_dp(t);
        if (fp_int == fb_value && instance_resource == instance) {
          frame_resource = true;
        }
      }
    }
    if (frame_resource) {
      oc_add_resource_to_wk(resource, request, device_index, &response_length,
                            matches, 1);
      matches++;
    }
  }

  if (matches > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fb_x_get_handler - end\n");
}

void
oc_create_fb_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fb_x_resource\n");
  // note that this resource is listed in /.well-known/core so it should have
  // the full rt with urn:knx prefix
  oc_core_populate_resource(
    resource_idx, device, "/f/*", OC_IF_LI, APPLICATION_LINK_FORMAT, 0,
    oc_core_fb_x_get_handler, 0, 0, 0, 1, "urn:knx:fb.0");
}

// -----------------------------------------------------------------------------

int
oc_count_functional_blocks(size_t device_index)
{
  int counter = 0;
  int i;
  bool netip_added = false;

  if (g_nr_functional_blocks > 0) {
    return g_nr_functional_blocks;
  }

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
    }

    oc_string_array_t types = resource->types;
    for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
      char *t = oc_string_array_get_item(types, i);
      if ((strncmp(t, ":dpa.11.", 8) == 0) ||
          (strncmp(t, "urn:knx:dpa.11.", 15) == 0)) {
        /* specific functional block iot_router : /f/netip */
        if (netip_added == false) {
          /* add only once */
          counter++;
          netip_added = true;
        }
      } else {
        /* regular functional block, framing by functional block numbers &
         * instances*/
        if ((strncmp(t, ":dpa", 4) == 0) ||
            (strncmp(t, "urn:knx:dpa", 11) == 0)) {
          int fp_int = get_fp_from_dp(t);
          int instance = resource->fb_instance;
          if ((fp_int > 0) && (is_in_g_array(fp_int, instance) == false)) {
            counter++;
          }
        }
      }
    }
  }
  g_nr_functional_blocks = counter;
  return g_nr_functional_blocks;
}

bool
oc_add_function_blocks_to_response(oc_request_t *request, size_t device_index,
                                   size_t *response_length, int matches)
{
  (void)request;
  int length = 0;
  char number[24];
  int i;
  int counter = 0;

  // use global variable
  g_array_size = 0;
  bool netip_added = false;

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
    }

    oc_string_array_t types = resource->types;
    for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
      char *t = oc_string_array_get_item(types, i);
      if ((strncmp(t, ":dpa.11.", 8) == 0) ||
          (strncmp(t, "urn:knx:dpa.11.", 15) == 0)) {
        /* specific functional block iot_router : /f/netip */
        if (netip_added == false) {
          /* add only once */
          length = oc_rep_add_line_to_buffer("</f/netip>;rt=\":fb.11\";ct=40");
          *response_length += length;
          matches++;
          netip_added = true;
          counter++;
        }
      } else {
        /* regular functional block, framing by functional block numbers &
         * instances*/
        if ((strncmp(t, ":dpa", 4) == 0) ||
            (strncmp(t, "urn:knx:dpa", 11) == 0)) {
          int fp_int = get_fp_from_dp(t);
          int instance = resource->fb_instance;
          if ((fp_int > 0) && (is_in_g_array(fp_int, instance) == false)) {
            store_in_array(fp_int, instance);
            counter++;
          }
        }
      }
    }
  }

  for (i = 0; i < g_array_size; i++) {

    if (matches > 0) {
      length = oc_rep_add_line_to_buffer(",\n");
      *response_length += length;
    }

    length = oc_rep_add_line_to_buffer("</f/");
    *response_length += length;
    if (g_int_array[1][i] > 0) {
      // functional block with instance with 2 numbers, e.g. <functional
      // block>_<instance>
      snprintf(number, 23, "%05d_%02d", g_int_array[0][i], g_int_array[1][i]);
      // snprintf(number, 23, "%d_%d", g_int_array[0][i], g_int_array[1][i]);
    } else {
      // functional block with no instance, e.g. defaulting to instance 0.
      snprintf(number, 5, "%d", g_int_array[0][i]);
    }
    length = oc_rep_add_line_to_buffer(number);
    *response_length += length;
    length = oc_rep_add_line_to_buffer(">;");
    *response_length += length;
    matches++;

    length = oc_rep_add_line_to_buffer("rt=\"");
    *response_length += length;
    length = oc_rep_add_line_to_buffer(":fb.");
    *response_length += length;
    // e.g. max functional block is 12345
    snprintf(number, 6, "%d", g_int_array[0][i]);
    length = oc_rep_add_line_to_buffer(number);
    *response_length += length;
    length = oc_rep_add_line_to_buffer("\";");
    *response_length += length;
    /* content type application link format*/
    length = oc_rep_add_line_to_buffer("ct=40");
    *response_length += length;
  }

  if (matches > 0) {
    return true;

    if (g_nr_functional_blocks == 0) {
      // store the counter so that we only have to do this once
      g_nr_functional_blocks = counter;
    }
  }

  return false;
}

/*
 * return list of function blocks
 */
static void
oc_core_fb_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                       void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int length;
  int matches = 0;
  bool ps_exists;
  bool total_exists;

  PRINT("oc_core_fb_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  // handle query parameters: l=ps l=total
  if (check_if_query_l_exist(request, &ps_exists, &total_exists)) {
    // example : < / fp / r / ? l = total>; total = 22; ps = 5

    length = oc_frame_query_l("/auth/at", ps_exists, total_exists);
    response_length += length;
    if (ps_exists) {
      length = oc_rep_add_line_to_buffer(";ps=");
      response_length += length;
      length = oc_frame_integer(g_nr_functional_blocks);
      response_length += length;
    }
    if (total_exists) {
      length = oc_rep_add_line_to_buffer(";total=");
      response_length += length;
      length = oc_frame_integer(g_nr_functional_blocks);
      response_length += length;
    }

    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }

  bool added = oc_add_function_blocks_to_response(request, device_index,
                                                  &response_length, matches);

  if (added) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fb_get_handler - end\n");
}

void
oc_create_fb_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fb_resource\n");
  // note that this resource is listed in /.well-known/core so it should have
  // the full rt with urn:knx prefix
  oc_core_populate_resource(resource_idx, device, "/f", OC_IF_LI,
                            APPLICATION_LINK_FORMAT, 0, oc_core_fb_get_handler,
                            0, 0, 0, 1, "urn:knx:fb.0");
}

void
oc_create_knx_fb_resources(size_t device_index)
{

  oc_create_fb_x_resource(OC_KNX_F_X, device_index);

  // should be last of the dev/xxx resources, it will list those.
  oc_create_fb_resource(OC_KNX_F, device_index);
}
