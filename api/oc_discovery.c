/*
// Copyright (c) 2016 Intel Corporation
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

#include "oc_client_state.h"

#include "messaging/coap/oc_coap.h"
#include "oc_api.h"
#include "oc_discovery.h"
#include "oc_knx_fb.h"
#include "oc_knx_fp.h"

#include "oc_core_res.h"
#include "oc_endpoint.h"
#include "oc_knx_helpers.h"

#ifdef OC_SECURITY
#include "security/oc_pstat.h"
#include "security/oc_sdi.h"
#include "security/oc_tls.h"
#endif
#ifdef OC_OSCORE
#include "security/oc_tls.h"
#endif
#include <inttypes.h>

int basic_resources[] = {
  OC_DEV, OC_KNX_K, OC_KNX_SWU, OC_KNX_SUB, OC_KNX_AUTH
}; // must be in response if implemented and passed filtering

bool
oc_add_resource_to_wk(const oc_resource_t *resource, oc_request_t *request,
                      size_t device_index, size_t *response_length,
                      int truncate)
{
  (void)device_index; /* variable not used */
  int length;

  if (resource == NULL) {
    return false;
  }

  if ((oc_string_len(resource->uri) == 0)) {
    return false;
  }

  if (*response_length > 0) {
    /* frame the trailing comma */
    length = oc_rep_add_line_to_buffer(",\n");
    *response_length += length;
  }

  length = oc_rep_add_line_to_buffer("<");
  *response_length += length;
  /*
  // code to frame the used IP address
  oc_endpoint_t *eps = oc_connectivity_get_endpoints(request->resource->device);
  oc_string_t ep, uri;
  memset(&uri, 0, sizeof(oc_string_t));
  while (eps != NULL) {
//#ifdef OC_SECURITY
#ifdef OC_OSCORE
    if (eps->flags & SECURED) {
#else
    if ((eps->flags & SECURED) == 0) {
#endif // OC_SECURITY
      if (oc_endpoint_to_string(eps, &ep) == 0) {
        length = oc_rep_add_line_to_buffer(oc_string(ep));
        *response_length += length;
        oc_free_string(&ep);
        break;
      }
    }
    eps = eps->next;
  }
 */

  length = oc_rep_add_line_to_buffer(oc_string(resource->uri));
  *response_length += length;

  length = oc_rep_add_line_to_buffer(">;");
  *response_length += length;

  int i;
  int numberofresourcetypes =
    (int)oc_string_array_get_allocated_size(resource->types);

  if (numberofresourcetypes > 0) {
    length = oc_rep_add_line_to_buffer("rt=\"");
    *response_length += length;

    for (i = 0; i < numberofresourcetypes; i++) {
      size_t size = oc_string_array_get_item_size(resource->types, i);
      const char *t =
        (const char *)oc_string_array_get_item(resource->types, i);
      if (size > 0) {

        if (i > 0) {
          // white space as separator of the rt values
          length = oc_rep_add_line_to_buffer(" ");
          *response_length += length;
        }
        if (truncate == 0) {
          // full rt with urn
          length = oc_rep_add_line_size_to_buffer(t, (int)size);
          *response_length += length;
        } else if (truncate == 1) {
          // rt with urn removed
          if (strncmp(t, "urn:knx", 7) == 0) {
            length = oc_rep_add_line_size_to_buffer(&t[7], (int)size - 7);
            *response_length += length;
          } else {
            // does not start with urn, so frame it all
            length = oc_rep_add_line_size_to_buffer(t, (int)size);
            *response_length += length;
          }
        }
      }
    }

    length = oc_rep_add_line_to_buffer("\";");
    *response_length += length;
  }

  if (resource->interfaces > 0) {
    length = oc_rep_add_line_to_buffer("if=");
    *response_length += length;
    length =
      oc_frame_interfaces_mask_in_response(resource->interfaces, truncate);
    *response_length += length;

    length = oc_rep_add_line_to_buffer(";");
    *response_length += length;
  }

  if (resource->content_type) {
    length = oc_rep_add_line_to_buffer("ct=");
    *response_length += length;
    char my_ct_value[5];
    sprintf((char *)&my_ct_value, "%d", resource->content_type);
    length = oc_rep_add_line_to_buffer(my_ct_value);
    *response_length += length;
  }

  return true;
}

bool
oc_filter_resource(const oc_resource_t *resource, oc_request_t *request,
                   size_t device_index, size_t *response_length, int *skipped,
                   int first_entry)
{
  (void)device_index; /* variable not used */

  if (!oc_filter_resource_by_rt(resource, request)) {
    return false;
  }

  if (!oc_filter_resource_by_if(resource, request)) {
    return false;
  }

  if (!(resource->properties & OC_DISCOVERABLE)) {
    return false;
  }

  if (*skipped < first_entry) {
    (*skipped)++;
    return false;
  }

  int truncate = oc_filter_resource_by_urn(resource, request);

  return oc_add_resource_to_wk(resource, request, device_index, response_length,
                               truncate);
}

bool
oc_process_resources(oc_request_t *request, size_t device_index,
                     size_t *response_length, int *matches, int *skipped,
                     int first_entry, int last_entry)
{

  const oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE))
      continue;

    if (oc_filter_resource(resource, request, device_index, response_length,
                           skipped, first_entry)) {
      (*matches)++;
      if (first_entry + (*matches) >= last_entry) {
        return true;
      }
    }
  }

  return false;
}

bool
oc_process_basic_resources(oc_request_t *request, size_t device_index,
                           size_t *response_length, int *matches, int *skipped,
                           int first_entry, int last_entry)
{
  for (int i = 0; i < sizeof(basic_resources) / sizeof(basic_resources[0]);
       i++) {
    if (oc_filter_resource(
          oc_core_get_resource_by_index(basic_resources[i], device_index),
          request, device_index, response_length, skipped, first_entry)) {
      (*matches)++;
      if (first_entry + (*matches) >= last_entry) {
        return true;
      }
    }
  }
  return false;
}

static int
frame_sn(char *serial_number, uint64_t iid, uint32_t ia)
{
  int framed_bytes;
  int response_length = 0;

  framed_bytes = oc_rep_add_line_to_buffer("<>;ep=\"knx://sn.");
  response_length = response_length + framed_bytes;
  framed_bytes = oc_rep_add_line_to_buffer(serial_number);
  response_length = response_length + framed_bytes;

  framed_bytes = oc_rep_add_line_to_buffer(" knx://ia.");
  response_length = response_length + framed_bytes;
  char text_hex[20];
  oc_conv_uint64_to_hex_string(text_hex, iid);
  framed_bytes = oc_rep_add_line_to_buffer(text_hex);
  response_length = response_length + framed_bytes;

  snprintf(text_hex, 19, ".%x", ia);
  framed_bytes = oc_rep_add_line_to_buffer(text_hex);
  response_length = response_length + framed_bytes;
  framed_bytes = oc_rep_add_line_to_buffer("\"");
  response_length = response_length + framed_bytes;
  return response_length;
}

static void
oc_wkcore_discovery_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int matches = 0;
  int skipped = 0;
  bool finished = false;
  bool query_match = false;
  int framed_bytes;
  bool more_request_needed =
    false; // If more requests (pages) are needed to get the full list

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT &&
      request->accept != APPLICATION_JSON && request->accept != CONTENT_NONE) {
    /* handle bad request..
    note below layer ignores this message if it is a multi cast request */
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  char *value = NULL;
  size_t value_len;
  char *key;
  size_t key_len;
  char *rt_request = 0;
  int rt_len = 0;
  char *ep_request = 0;
  int ep_len = 0;
  char *if_request = 0;
  int if_len = 0;
  char *d_request = 0;
  int d_len = 0;

  bool ps_exists = false;
  bool total_exists = false;
  int total = 0;
  int first_entry = 0; // inclusive
  int last_entry = 0;  // exclusive
  // int query_ps = -1;
  int query_pn = -1;

  value_len = -1;
  oc_init_query_iterator();
  while (oc_iterate_query(request, &key, &key_len, &value, &value_len) > 0) {
    if (strncmp(key, "rt", key_len) == 0) {
      rt_request = value;
      rt_len = (int)value_len;
      query_match = true;
    }
    if (strncmp(key, "ep", key_len) == 0) {
      ep_request = value;
      ep_len = (int)value_len;
      query_match = true;
    }
    if (strncmp(key, "if", key_len) == 0) {
      if_request = value;
      if_len = (int)value_len;
      query_match = true;
    }
    if (strncmp(key, "d", key_len) == 0) {
      d_request = value;
      d_len = (int)value_len;
      query_match = true;
    }
  }

  // get the device structure from the request.
  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);

  /* handle multicast with no queries */
  if (request->query_len == 0 && request->origin &&
      (request->origin->flags & MULTICAST) != 0) {
    response_length =
      frame_sn(oc_string(device->serialnumber), device->iid, device->ia);
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }

  if (rt_len > 0 || if_len > 0) {
    const oc_resource_t *my_resource = oc_ri_get_app_resources();
    // Calculate total properties
    for (; my_resource; my_resource = my_resource->next) {
      if (my_resource->device != device_index ||
          !(my_resource->properties & OC_DISCOVERABLE)) {
        continue;
      }
      if (oc_string(my_resource->uri) != NULL) {
        total++;
      }
    }
  }
  total += sizeof(basic_resources) / sizeof(basic_resources[0]);
  total += oc_count_functional_blocks(device_index);
  last_entry = total;

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /.well-known/core > l = total>;total=22;ps=5
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
    query_match = true;
    first_entry += query_pn * PAGE_SIZE;
    if (first_entry >= last_entry) {
      oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
      return;
    }
  }

  if (last_entry > first_entry + PAGE_SIZE) {
    last_entry = first_entry + PAGE_SIZE;
    more_request_needed = true;
  }

  if (request->query_len > 0 && !query_match) {
    if (request->origin && (request->origin->flags & MULTICAST) == 0) {
      // for unicast
      oc_send_linkformat_response(request, OC_STATUS_OK, 0);
    } else {
      oc_ignore_request(request);
    }
    return;
  }

  /* if device is belongs to a group address
     ?d=urn:knx:g.s.[ga]
     list the data points to which the group address applies to
  */
  if (d_len > 12 && strncmp(d_request, "urn:knx:g.s.", 12) == 0) {
    int group_address = atoi(&d_request[12]);
    PRINT(" group address: %d\n", group_address);
    // if not loaded the we can just return
    oc_lsm_state_t lsm = oc_a_lsm_state(device_index);
    if (lsm != LSM_S_LOADED) {
      /* handle bad request..
      note below layer ignores this message if it is a multi cast request */
      PRINT(" not loaded!");
      oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
      return;
    }
    if (strncmp(d_request, "urn:knx:g.s.*", 13) == 0) {
      // Quote from EITT test 5.1.1.8: "Must fail since the response would
      // likely be excessively large"
      oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
      return;
    }
    // create the response
    bool ret = oc_add_points_in_group_object_table_to_response(
      request, device_index, group_address, &response_length, matches);
    if (ret) {
      oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    } else if (request->origin && (request->origin->flags & MULTICAST) == 0) {
      oc_send_linkformat_response(request, OC_STATUS_OK, 0);
    } else {
      oc_ignore_request(request);
    }
    return;
  }

  if (if_len == 13 && strncmp(if_request, "urn:knx:if.pm", 13) == 0) {

    if (oc_is_device_mode_in_programming(device_index)) {
      /* device is in programming mode so create the response */
      /* add only the serial number when the interface is if.pm && device is
         in programming mode return <>; ep="urn:knx:sn.<serial-number>" return
         immediately so we do not process any other query params note: we ignore
         the ep=urn:knx:sn.* and if=urn:knx:if.pm concatenation. since that only
         needs to respond when the device is in programming mode
      */
      // if unicast & serial number is wrong, return error
      if (ep_request != 0 && ep_len > 9 &&
          strncmp(ep_request, "knx://sn.", 9) == 0) {
        char *ep_serialnumber = ep_request + 9;

        if (strncmp(oc_string(device->serialnumber), ep_serialnumber,
                    strlen(oc_string(device->serialnumber))) != 0) {
          if (request->origin && (request->origin->flags & MULTICAST) == 0) {
            oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
          } else {
            oc_ignore_request(request);
          }
          return;
        }
      } else {
        if (skipped < first_entry) {
          skipped++;
        } else {
          response_length =
            frame_sn(oc_string(device->serialnumber), device->iid, device->ia);
          matches++;
        }

        PRINT(" oc_wkcore_discovery_handler PM HANDLING: OK\n");
      }
    } else {
      /* device is not in programming mode so ignore this request*/
      if (request->origin && (request->origin->flags & MULTICAST) == 0) {
        oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
      } else {
        oc_ignore_request(request);
      }
      return;
    }
  }

  /* handle individual address, spec 1.0 */
  if (if_request != 0 && if_len > 11 &&
      strncmp(if_request, "urn:knx:ia.", 11) == 0) {
    char *if_ia_s = if_request + 11;
    int if_ia_i = atoi(if_ia_s);
    if (if_ia_i == device->ia) {
      framed_bytes =
        oc_rep_add_line_to_buffer("</dev/sna>;rt=\"dpa.0.57\";ct=50,");
      response_length = response_length + framed_bytes;
      framed_bytes =
        oc_rep_add_line_to_buffer("</dev/da>;rt=\"dpa.0.58\";ct=50,");
      response_length = response_length + framed_bytes;

      oc_send_linkformat_response(request, OC_STATUS_OK, response_length);

      PRINT(" oc_wkcore_discovery_handler IA HANDLING: OK\n");
    } else {
      /* should ignore this request*/
      // PRINT(" oc_wkcore_discovery_handler IA HANDLING: IGNORE\n");
      oc_ignore_request(request);
    }
    return;
  }

  /* handle individual address, spec 1.1 */
  if (ep_request != 0 && ep_len > 9 &&
      strncmp(ep_request, "knx://ia.", 9) == 0) {
    bool frame_ep = false;
    /* new style release 1.1 */
    /* request for all devices via serial number wild card*/
    /* example:     knx://ia.d773e094b6.1101 */
    char *ep_ia = ep_request + 20; // at the end, after the second dot
    uint32_t ia = strtol(ep_ia, NULL, 16);
    if (ia == device->ia) {
      // ia is the same
      // now do the extra work to check the iid
      // do this with string compare, otherwise we have to create an uint64 from
      // string
      int iid_str_len = 10;
      char *iid_str = oc_strnchr(ep_request, '.', iid_str_len);
      if (iid_str) {
        char iid_dev[20];
        oc_conv_uint64_to_hex_string(iid_dev, device->iid);
        if (strncmp(iid_dev, iid_str + 1, iid_str_len) == 0) {
          response_length =
            frame_sn(oc_string(device->serialnumber), device->iid, device->ia);
          oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
          return;
        }
      }
    }
    /* should ignore this request*/
    // PRINT(" oc_wkcore_discovery_handler IA HANDLING: IGNORE\n");
    oc_ignore_request(request);
    return;
  }

  /* handle serial number spec 1.0 */
  if (ep_request != 0 && ep_len > 11 &&
      strncmp(ep_request, "urn:knx:sn.", 11) == 0) {
    /* old style can be removed later*/
    /* request for all devices via serial number wild card*/
    char *ep_serialnumber = ep_request + 11;
    bool frame_ep = false;

    if (strncmp(ep_serialnumber, "*", 1) == 0) {
      /* matches wild card*/
      frame_ep = true;
    }
    if (strncmp(oc_string(device->serialnumber), ep_serialnumber,
                strlen(oc_string(device->serialnumber))) == 0) {
      frame_ep = true;
    }
    if (frame_ep) {
      if (skipped < first_entry) {
        skipped++;
      } else {
        /* return <>; ep="urn:knx:sn.<serial-number>"*/
        framed_bytes = oc_rep_add_line_to_buffer("<>;ep=\"urn:knx:sn.");
        response_length = response_length + framed_bytes;
        framed_bytes =
          oc_rep_add_line_to_buffer(oc_string(device->serialnumber));
        response_length = response_length + framed_bytes;
        framed_bytes = oc_rep_add_line_to_buffer("\"");
        response_length = response_length + framed_bytes;
        matches++;
      }
    }
  }
  /* handle serial number spec 1.1 */
  if (ep_request != 0 && ep_len > 9 &&
      strncmp(ep_request, "knx://sn.", 9) == 0) {
    /* new style release 1.1 */
    /* request for all devices via serial number wild card*/
    char *ep_serialnumber = ep_request + 9;

    if (strncmp(ep_serialnumber, "*", 1) == 0 ||
        strncmp(oc_string(device->serialnumber), ep_serialnumber,
                strlen(oc_string(device->serialnumber))) == 0) {
      response_length =
        frame_sn(oc_string(device->serialnumber), device->iid, device->ia);
      oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    } else {
      oc_ignore_request(request);
    }
    return;
  }

  if (rt_len > 0 || if_len > 0) {
    size_t device = request->resource->device;
    PRINT("  oc_wkcore_discovery_handler rt='%.*s'\n", rt_len, rt_request);
    PRINT("  oc_wkcore_discovery_handler if='%.*s'\n", if_len, if_request);
    if (!finished) {
      finished =
        oc_process_resources(request, device, &response_length, &matches,
                             &skipped, first_entry, last_entry);
    }
  }

  if (!finished) {
    finished =
      oc_process_basic_resources(request, device_index, &response_length,
                                 &matches, &skipped, first_entry, last_entry);
  }

  if (!finished && request->origin &&
      (request->origin->flags & MULTICAST) == 0) {
    // only for unicast
    if (oc_filter_functional_blocks(request)) {
      oc_add_function_blocks_to_response(request, device_index,
                                         &response_length, &matches, &skipped,
                                         first_entry, last_entry);
    }
  }

  if (matches > 0 && response_length > 0) {
    if (more_request_needed) {
      int next_page_num = query_pn > -1 ? query_pn + 1 : 1;
      response_length += add_next_page_indicator(
        oc_string(request->resource->uri), next_page_num);
    }
    PRINT("  oc_wkcore_discovery_handler response_length %d'\n",
          (int)response_length);
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else if (request->origin && (request->origin->flags & MULTICAST) == 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, 0);
  } else {
    oc_ignore_request(request);
  }
}

OC_CORE_CREATE_CONST_RESOURCE_FINAL(well_known_core, 0, "/.well-known/core",
                                    OC_IF_NONE, APPLICATION_LINK_FORMAT,
                                    OC_DISCOVERABLE,
                                    oc_wkcore_discovery_handler, 0, 0, 0, NULL,
                                    OC_SIZE_MANY(1), "wk");

void
oc_create_discovery_resource(int resource_idx, size_t device)
{
  if (resource_idx == WELLKNOWNCORE && device > 0) {
    oc_core_populate_resource(resource_idx, device, "/.well-known/core",
                              OC_IF_NONE, APPLICATION_LINK_FORMAT,
                              OC_DISCOVERABLE, oc_wkcore_discovery_handler, 0,
                              0, 0, 1, "wk");
  } else if (device == 0) {
    OC_DBG("resources for dev 0 created statically");
  }
}

oc_discovery_flags_t
oc_ri_process_discovery_payload(uint8_t *payload, int len,
                                oc_client_handler_t client_handler,
                                oc_endpoint_t *endpoint,
                                oc_content_format_t content, void *user_data)
{
  oc_discovery_all_handler_t all_handler = client_handler.discovery_all;

  oc_discovery_flags_t ret = OC_CONTINUE_DISCOVERY;

  if (content == APPLICATION_LINK_FORMAT) {

    PRINT("oc_ri_process_discovery_payload: calling handler all\n");
    if (all_handler) {
      all_handler((const char *)payload, len, endpoint, user_data);
    }
  }

  return ret;
}
