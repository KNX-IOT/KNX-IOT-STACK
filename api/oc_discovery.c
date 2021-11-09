/*
// Copyright (c) 2016 Intel Corporation
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

#ifdef OC_CLIENT
#include "oc_client_state.h"
#endif /* OC_CLIENT */

#include "messaging/coap/oc_coap.h"
#include "oc_api.h"
#include "oc_discovery.h"
#include "oc_enums.h"

#include "oc_core_res.h"
#include "oc_endpoint.h"

#ifdef OC_SECURITY
#include "security/oc_pstat.h"
#include "security/oc_sdi.h"
#include "security/oc_tls.h"
#endif

bool
oc_add_resource_to_wk(oc_resource_t *resource, oc_request_t *request,
                      size_t device_index, size_t *response_length, int matches)
{
  (void)device_index; /* variable not used */
  int length;

  if (matches > 0) {
    length = oc_rep_add_line_to_buffer(",\n");
    *response_length += length;
  }

  length = oc_rep_add_line_to_buffer("<");
  *response_length += length;

  oc_endpoint_t *eps = oc_connectivity_get_endpoints(request->resource->device);
  oc_string_t ep, uri;
  memset(&uri, 0, sizeof(oc_string_t));
  while (eps != NULL) {
#ifdef OC_SECURITY
    if (eps->flags & SECURED) {
#else
    if ((eps->flags & SECURED) == 0) {
#endif /* OC_SECURITY */
      if (oc_endpoint_to_string(eps, &ep) == 0) {
        length = oc_rep_add_line_to_buffer(oc_string(ep));
        *response_length += length;
        oc_free_string(&ep);
        break;
      }
    }
    eps = eps->next;
  }

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
        length = oc_rep_add_line_size_to_buffer(t, size);
        *response_length += length;
      }
    }

    length = oc_rep_add_line_to_buffer("\";");
    *response_length += length;
  }

  if (resource->interfaces > 0) {
    length = oc_rep_add_line_to_buffer("if=");
    *response_length += length;
    length = oc_get_interfaces_mask(resource->interfaces);
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
oc_filter_resource(oc_resource_t *resource, oc_request_t *request,
                   size_t device_index, size_t *response_length, int matches)
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

  return oc_add_resource_to_wk(resource, request, device_index, response_length,
                               matches);
}

static bool
filter_resource(oc_resource_t *resource, oc_request_t *request,
                const char *anchor, CborEncoder *links, size_t device_index)
{
  if (!oc_filter_resource_by_rt(resource, request)) {
    return false;
  }

  if (!(resource->properties & OC_DISCOVERABLE)) {
    return false;
  }

#ifdef OC_SECURITY
  bool owned_for_SVRs =
    (oc_core_is_SVR(resource, device_index) &&
     (((oc_sec_get_pstat(device_index))->s != OC_DOS_RFOTM) ||
      oc_tls_num_peers(device_index) != 0));
#endif /* OC_SECURITY */

  oc_rep_start_object(links, link);

  // rel
  if (oc_core_get_resource_by_index(OCF_RES, resource->device) == resource) {
    oc_rep_set_array(link, rel);
    oc_rep_add_text_string(rel, "self");
    oc_rep_close_array(link, rel);
  }

  // anchor
  oc_rep_set_text_string(link, anchor, anchor);

  // uri
  oc_rep_set_text_string(link, href, oc_string(resource->uri));

  // rt
  oc_rep_set_array(link, rt);
  int i;
  for (i = 0; i < (int)oc_string_array_get_allocated_size(resource->types);
       i++) {
    size_t size = oc_string_array_get_item_size(resource->types, i);
    const char *t = (const char *)oc_string_array_get_item(resource->types, i);
    if (size > 0)
      oc_rep_add_text_string(rt, t);
  }
  oc_rep_close_array(link, rt);

  // if
  oc_core_encode_interfaces_mask(oc_rep_object(link), resource->interfaces);

  // p
  oc_rep_set_object(link, p);
  oc_rep_set_uint(p, bm,
                  (uint8_t)(resource->properties & ~(OC_PERIODIC | OC_SECURE)));
  oc_rep_close_object(link, p);

  // eps
  oc_rep_set_array(link, eps);
  oc_endpoint_t *eps = oc_connectivity_get_endpoints(device_index);
  while (eps != NULL) {
    /*  If this resource has been explicitly tagged as SECURE on the
     *  application layer, skip all coap:// endpoints, and only include
     *  coaps:// endpoints.
     *  Also, exclude all endpoints that are not associated with the interface
     *  through which this request arrived. This is achieved by checking if the
     *  interface index matches.
     */
    if (((resource->properties & OC_SECURE
#ifdef OC_SECURITY
          || owned_for_SVRs
#endif /* OC_SECURITY */
          ) &&
         !(eps->flags & SECURED)) ||
        (request->origin && request->origin->interface_index != -1 &&
         request->origin->interface_index != eps->interface_index)) {
      goto next_eps;
    }
    if (request->origin &&
        (((request->origin->flags & IPV4) && (eps->flags & IPV6)) ||
         ((request->origin->flags & IPV6) && (eps->flags & IPV4)))) {
      goto next_eps;
    }
    oc_rep_object_array_start_item(eps);
    oc_string_t ep;
    if (oc_endpoint_to_string(eps, &ep) == 0) {
      oc_rep_set_text_string(eps, ep, oc_string(ep));
      oc_free_string(&ep);
    }
    if (oc_core_get_latency() > 0)
      oc_rep_set_uint(eps, lat, oc_core_get_latency());
    oc_rep_object_array_end_item(eps);
  next_eps:
    eps = eps->next;
  }
#ifdef OC_OSCORE
  if (resource->properties & OC_SECURE_MCAST) {
    oc_rep_object_array_start_item(eps);
#ifdef OC_IPV4
    oc_rep_set_text_string(eps, ep, "coap://224.0.1.187:5683");
#endif /* OC_IPV4 */
    oc_rep_set_text_string(eps, ep, "coap://[ff02::158]:5683");
    oc_rep_object_array_end_item(eps);
  }
#endif /* OC_OSCORE */
  oc_rep_close_array(link, eps);

  oc_rep_end_object(links, link);

  return true;
}

int
oc_process_resources(oc_request_t *request, size_t device_index,
                     size_t *response_length)
{
  int matches = 0;

  if (*response_length > 0) {
    /* frame the trailing comma */
    matches = 1;
  }

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE))
      continue;

    if (oc_filter_resource(resource, request, device_index, response_length,
                           matches)) {
      matches++;
    }
  }

  return matches;
}

int
process_device_resources(CborEncoder *links, oc_request_t *request,
                         size_t device_index)
{
  int matches = 0;
  char uuid[OC_UUID_LEN];
  oc_uuid_to_str(oc_core_get_device_id(device_index), uuid, OC_UUID_LEN);
  oc_string_t anchor;
  oc_concat_strings(&anchor, "ocf://", uuid);

  if (filter_resource(oc_core_get_resource_by_index(OCF_P, 0), request,
                      oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(oc_core_get_resource_by_index(OCF_RES, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(oc_core_get_resource_by_index(OCF_D, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

#ifdef OC_SECURITY
  if (filter_resource(oc_core_get_resource_by_index(OCF_SEC_DOXM, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(
        oc_core_get_resource_by_index(OCF_SEC_PSTAT, device_index), request,
        oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(oc_core_get_resource_by_index(OCF_SEC_ACL, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(oc_core_get_resource_by_index(OCF_SEC_AEL, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(oc_core_get_resource_by_index(OCF_SEC_CRED, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(oc_core_get_resource_by_index(OCF_SEC_SP, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

#ifdef OC_PKI
  if (filter_resource(oc_core_get_resource_by_index(OCF_SEC_CSR, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

  if (filter_resource(
        oc_core_get_resource_by_index(OCF_SEC_ROLES, device_index), request,
        oc_string(anchor), links, device_index))
    matches++;
#endif /* OC_PKI */

  if (filter_resource(oc_core_get_resource_by_index(OCF_SEC_SDI, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

#endif /* OC_SECURITY */

#ifdef OC_SERVER
  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE))
      continue;

    if (filter_resource(resource, request, oc_string(anchor), links,
                        device_index))
      matches++;
  }
#endif /* OC_SERVER */

  oc_free_string(&anchor);

  return matches;
}

static void
oc_core_discovery_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  int matches = 0;
  size_t device = request->resource->device;

  // for dev without SVRs, ignore queries for backward compatibility
#ifdef OC_SECURITY
  char *q;
  int ql = oc_get_query_value(request, "sduuid", &q);
  if (ql > 0) {
    oc_sec_sdi_t *s = oc_sec_get_sdi(device);
    if (s->priv) {
      oc_ignore_request(request);
      OC_DBG("private sdi");
      return;
    } else {
      char uuid[OC_UUID_LEN];
      oc_uuid_to_str(&s->uuid, uuid, OC_UUID_LEN);
      if (ql != (OC_UUID_LEN - 1)) {
        oc_ignore_request(request);
        OC_DBG("uuid mismatch: ql %d", ql);
        return;
      }
      if (strncasecmp(q, uuid, OC_UUID_LEN - 1) != 0) {
        oc_ignore_request(request);
        OC_DBG("uuid mismatch: %s", uuid);
        return;
      }
    }
  }
#endif

  switch (iface_mask) {
  case OC_IF_LL: {
    oc_rep_start_links_array();
    matches += process_device_resources(oc_rep_array(links), request, device);
    oc_rep_end_links_array();
  } break;
  case OC_IF_BASELINE: {
    oc_rep_start_links_array();
    oc_rep_start_object(oc_rep_array(links), props);
    memcpy(&root_map, &props_map, sizeof(CborEncoder));
    oc_process_baseline_interface(
      oc_core_get_resource_by_index(OCF_RES, device));
#ifdef OC_SECURITY
    oc_sec_sdi_t *s = oc_sec_get_sdi(device);
    if (!s->priv) {
      char uuid[OC_UUID_LEN];
      oc_uuid_to_str(&s->uuid, uuid, OC_UUID_LEN);
      oc_rep_set_text_string(root, sduuid, uuid);
      oc_rep_set_text_string(root, sdname, oc_string(s->name));
    }
#endif
    oc_rep_set_array(root, links);
    matches += process_device_resources(oc_rep_array(links), request, device);
    oc_rep_close_array(root, links);
    memcpy(&props_map, &root_map, sizeof(CborEncoder));
    oc_rep_end_object(oc_rep_array(links), props);
    oc_rep_end_links_array();
  } break;
  default:
    break;
  }
  int response_length = oc_rep_get_encoded_payload_size();
  request->response->response_buffer->content_format = APPLICATION_VND_OCF_CBOR;
  if (matches && response_length > 0) {
    request->response->response_buffer->response_length = response_length;
    request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  } else if (request->origin && (request->origin->flags & MULTICAST) == 0) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
  } else {
    request->response->response_buffer->code = OC_IGNORE;
  }
}

#ifdef OC_SERVER

static void
oc_wkcore_discovery_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int matches = 0;

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT &&
      request->accept != APPLICATION_JSON) {
    /* handle bad request..
    note below layer ignores this message if it is a multi cast request */
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  char *value = NULL;
  size_t value_len;
  char *key;
  size_t key_len;
  // char *rt_request = 0;
  int rt_len = 0;
  char *ep_request = 0;
  int ep_len = 0;
  char *if_request = 0;
  int if_len = 0;

  value_len = -1;
  oc_init_query_iterator();
  while (oc_iterate_query(request, &key, &key_len, &value, &value_len) > 0) {
    if (strncmp(key, "rt", key_len) == 0) {
      // rt_request = value;
      rt_len = (int)value_len;
    }
    if (strncmp(key, "ep", key_len) == 0) {
      ep_request = value;
      ep_len = (int)value_len;
    }
    if (strncmp(key, "if", key_len) == 0) {
      if_request = value;
      if_len = (int)value_len;
    }
  }
  /* d */

  // get the device structure from the request.
  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);

  if (oc_is_device_mode_in_programming(device_index)) {
    // add only the serial number when the interface is if.pm && device is in
    // programming mode
    if (if_len == 13 && strncmp(if_request, "urn:knx:if.pm", 13) == 0) {
      int size = oc_rep_add_line_to_buffer("<>;ep=urn:knx:sn.");
      response_length = response_length + size;
      size = oc_rep_add_line_to_buffer(oc_string(device->serialnumber));
      response_length = response_length + size;
      matches = 1;
    }
  }

  /* handle individual address */
  if (if_request != 0 && if_len > 11 &&
      strncmp(if_request, "urn:knx:ia.", 11) == 0) {
    char *if_ia_s = if_request + 11;
    int if_ia_i = atoi(if_ia_s);
    if (if_ia_i == device->ia) {
      /* return the ll entry: </dev/ia>;rt="dpt.value2Ucount";ct=50 */
      int size =
        oc_rep_add_line_to_buffer("</dev/ia>;rt=\"dpt.value2Ucount\";ct=50");
      response_length = response_length + size;

      request->response->response_buffer->response_length = response_length;
      request->response->response_buffer->content_format =
        APPLICATION_LINK_FORMAT;
      request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
      ;
      PRINT(" oc_wkcore_discovery_handler IA HANDLING: OK\n");
      return;
    } else {
      /* should ignore this request*/
      PRINT(" oc_wkcore_discovery_handler IA HANDLING: IGNORE\n");
      request->response->response_buffer->code = OC_IGNORE;
      return;
    }
  }

  /* handle serial number*/
  if (ep_request != 0 && ep_len > 11 &&
      strncmp(ep_request, "urn:knx:sn.", 11) == 0) {
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
      /* return <>; ep=”urn:knx:sn.<serial-number>”*/
      int size = oc_rep_add_line_to_buffer("<>;ep=urn:knx:sn.");
      response_length = response_length + size;
      size = oc_rep_add_line_to_buffer(oc_string(device->serialnumber));
      response_length = response_length + size;
      matches = 1;
    }
  }

  if (rt_len > 0 || if_len > 0) {
    size_t device = request->resource->device;
    matches = oc_process_resources(request, device, &response_length);
  }

  oc_add_resource_to_wk(oc_core_get_resource_by_index(OC_DEV, device_index),
                        request, device_index, &response_length, matches);

  // oc_add_resource_to_wk(oc_core_get_resource_by_index(OC_AUTH, device_index),
  //                      request, device_index, &response_length, matches);

  oc_add_resource_to_wk(oc_core_get_resource_by_index(OC_KNX_SWU, device_index),
                        request, device_index, &response_length, matches);

  // oc_add_resource_to_wk(oc_core_get_resource_by_index(OC_SUB, device_index),
  //                      request, device_index, &response_length, matches);

  request->response->response_buffer->content_format = APPLICATION_LINK_FORMAT;

  if (matches > 0 && response_length > 0) {
    request->response->response_buffer->response_length = response_length;
    request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  } else if (request->origin && (request->origin->flags & MULTICAST) == 0) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
  } else {
    request->response->response_buffer->code = OC_IGNORE;
  }
}

#endif /* OC_SERVER */

void
oc_create_discovery_resource(int resource_idx, size_t device)
{

#ifdef OC_SERVER
  if (resource_idx == WELLKNOWNCORE) {

    oc_core_lf_populate_resource(resource_idx, device, "/.well-known/core", 0,
                                 APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                                 oc_wkcore_discovery_handler, 0, 0, 0, 1, "wk");

    return;
  }
#endif /* OC_SERVER*/

  oc_core_populate_resource(
    resource_idx, device, "oic/res", OC_IF_LL | OC_IF_BASELINE, OC_IF_LL,
    OC_DISCOVERABLE, oc_core_discovery_handler, 0, 0, 0, 1, "oic.wk.res");
}

#ifdef OC_CLIENT
oc_discovery_flags_t
oc_ri_process_discovery_payload(uint8_t *payload, int len,
                                oc_client_handler_t client_handler,
                                oc_endpoint_t *endpoint,
                                oc_content_format_t content, void *user_data)
{

  // oc_discovery_handler_t handler = client_handler.discovery;
  oc_discovery_all_handler_t all_handler = client_handler.discovery_all;

  oc_discovery_flags_t ret = OC_CONTINUE_DISCOVERY;

  if (content == APPLICATION_LINK_FORMAT) {

    PRINT("calling handler all\n");
    if (all_handler) {
      all_handler((const char *)payload, len, endpoint, user_data);
    }
  }

  return ret;
}
#endif /* OC_CLIENT */
