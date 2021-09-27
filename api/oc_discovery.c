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

#ifdef OC_RES_BATCH_SUPPORT
#ifdef OC_SECURITY
#include "security/oc_acl_internal.h"
#endif /* OC_SECURITY */
#endif /* OC_RES_BATCH_SUPPORT */

#if defined(OC_COLLECTIONS) && defined(OC_SERVER)
#include "oc_collection.h"
#endif /* OC_COLLECTIONS  && OC_SERVER */

#include "oc_core_res.h"
#include "oc_endpoint.h"

#ifdef OC_SECURITY
#include "security/oc_pstat.h"
#include "security/oc_sdi.h"
#include "security/oc_tls.h"
#endif

static int
clf_add_line_to_buffer(const char *line)
{
  int len = (int)strlen(line);
  oc_rep_encode_raw((uint8_t *)line, len);
  return len;
}

static int
clf_add_line_size_to_buffer(const char *line, int len)
{
  oc_rep_encode_raw((uint8_t *)line, len);
  return len;
}

static bool
oc_filter_resource(oc_resource_t *resource, oc_request_t *request,
                   size_t device_index, size_t *response_length, int matches)
{
  (void)device_index; /* variable not used */

  int length;
  if (!oc_filter_resource_by_rt(resource, request)) {
    return false;
  }

  if (!oc_filter_resource_by_if(resource, request)) {
    return false;
  }

  if (!(resource->properties & OC_DISCOVERABLE)) {
    return false;
  }

  if (matches > 0) {
    length = clf_add_line_to_buffer(",\n");
    *response_length += length;
  }

  length = clf_add_line_to_buffer("<");
  *response_length += length;

  oc_endpoint_t *eps = oc_connectivity_get_endpoints(request->resource->device);
  oc_string_t ep, uri;
  memset(&uri, 0, sizeof(oc_string_t));
  while (eps != NULL) {
    if (eps->flags & SECURED) {
      if (oc_endpoint_to_string(eps, &ep) == 0) {
        length = clf_add_line_to_buffer(oc_string(ep));
        *response_length += length;
        oc_free_string(&ep);
        break;
      }
    }
    eps = eps->next;
  }

  length = clf_add_line_to_buffer(oc_string(resource->uri));
  *response_length += length;

  length = clf_add_line_to_buffer(">;");
  *response_length += length;
  length = clf_add_line_to_buffer("rt=");
  *response_length += length;
  // length = clf_add_line_to_buffer(oc_string(resource->types));

  int i;
  int numberofresourcetypes =
    (int)oc_string_array_get_allocated_size(resource->types);

  for (i = 0; i < numberofresourcetypes; i++) {
    size_t size = oc_string_array_get_item_size(resource->types, i);
    const char *t = (const char *)oc_string_array_get_item(resource->types, i);
    if (size > 0) {
      length = clf_add_line_size_to_buffer(t, size);
      *response_length += length;
    }
  }

  length = clf_add_line_to_buffer(";");
  *response_length += length;

  length = clf_add_line_to_buffer("if=");
  *response_length += length;
  length = oc_get_interfaces_mask(resource->interfaces);
  *response_length += length;

  length = clf_add_line_to_buffer(";");
  *response_length += length;

  length = clf_add_line_to_buffer("ct=50");
  *response_length += length;

  return true;
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

  // tag-pos-desc
  if (resource->tag_pos_desc > 0) {
    const char *desc = oc_enum_pos_desc_to_str(resource->tag_pos_desc);
    if (desc) {
      // clang-format off
      oc_rep_set_text_string(link, tag-pos-desc, desc);
      // clang-format on
    }
  }

  // tag-func-desc
  if (resource->tag_func_desc > 0) {
    const char *func = oc_enum_to_str(resource->tag_func_desc);
    if (func) {
      // clang-format off
      oc_rep_set_text_string(link, tag-func-desc, func);
      // clang-format on
    }
  }

  // tag-locn
  if (resource->tag_locn > 0) {
    const char *locn = oc_enum_locn_to_str(resource->tag_locn);
    if (locn) {
      // clang-format off
      oc_rep_set_text_string(link, tag-locn, locn);
      // clang-format on
    }
  }

  // tag-pos-rel
  double *pos = resource->tag_pos_rel;
  if (pos[0] != 0 || pos[1] != 0 || pos[2] != 0) {
    oc_rep_set_key(oc_rep_object(link), "tag-pos-rel");
    oc_rep_start_array(oc_rep_object(link), tag_pos_rel);
    oc_rep_add_double(tag_pos_rel, pos[0]);
    oc_rep_add_double(tag_pos_rel, pos[1]);
    oc_rep_add_double(tag_pos_rel, pos[2]);
    oc_rep_end_array(oc_rep_object(link), tag_pos_rel);
  }

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

  if (filter_resource(
        oc_core_get_resource_by_index(OCF_INTROSPECTION_WK, device_index),
        request, oc_string(anchor), links, device_index))
    matches++;

  if (oc_get_con_res_announced() &&
      filter_resource(oc_core_get_resource_by_index(OCF_CON, device_index),
                      request, oc_string(anchor), links, device_index))
    matches++;

#ifdef OC_SOFTWARE_UPDATE
  if (filter_resource(
        oc_core_get_resource_by_index(OCF_SW_UPDATE, device_index), request,
        oc_string(anchor), links, device_index))
    matches++;
#endif /* OC_SOFTWARE_UPDATE */

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

#if defined(OC_COLLECTIONS)
  oc_resource_t *collection = (oc_resource_t *)oc_collection_get_all();
  for (; collection; collection = collection->next) {
    if (collection->device != device_index ||
        !(collection->properties & OC_DISCOVERABLE))
      continue;

    if (filter_resource(collection, request, oc_string(anchor), links,
                        device_index))
      matches++;
  }
#endif /* OC_COLLECTIONS */
#endif /* OC_SERVER */

  oc_free_string(&anchor);

  return matches;
}

static void
oc_core_discovery_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;

#ifdef OC_SPEC_VER_OIC
  if (request->origin && request->origin->version == OIC_VER_1_1_0) {
    oc_core_1_1_discovery_handler(request, iface_mask, data);
    return;
  }
#endif /* OC_SPEC_VER_OIC */

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
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
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

  value_len = -1;
  oc_init_query_iterator();
  while (oc_iterate_query(request, &key, &key_len, &value, &value_len) > 0) {
    if (strncmp(key, "rt", key_len) == 0) {
      rt_request = value;
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

  if (ep_request != 0 && ep_len > 11 && strncmp(ep_request, "urn:knx:sn:", 11) == 0) {
    /* request for all devices via serial number wildcard*/
    char *ep_serialnumber = ep_request + 11;
    bool frame_ep = false;

    size_t device_index = request->resource->device;
    oc_device_info_t *device = oc_core_get_device_info(device_index);

    if (strncmp (ep_serialnumber, "*", 1) ==0) {
      /* matches wild card*/
      frame_ep = true;
    }
    if (strncmp(oc_string(device->serialnumber), ep_serialnumber,
                strlen(oc_string(device->serialnumber))) == 0) {
      frame_ep = true;
    }
    if (frame_ep) {
      /* return <>; ep=”urn:knx:sn.<serial-number>”*/
      int size = clf_add_line_to_buffer("<>;ep=urn:knx:sn.");
      response_length = response_length + size;
      size = clf_add_line_to_buffer(oc_string(device->serialnumber));
      response_length = response_length + size;
      matches = 1;
    }
  }

  if (rt_len > 0 || if_len > 0) {
    size_t device = request->resource->device;
    matches = oc_process_resources(request, device, &response_length);
  }
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

    oc_core_populate_resource(resource_idx, device, "/.well-known/core", 0, 0,
                              OC_DISCOVERABLE, oc_wkcore_discovery_handler, 0,
                              0, 0, 1, "wk");

    return;
  }
#endif /* OC_SERVER*/

  oc_core_populate_resource(resource_idx, device, "oic/res",
#ifdef OC_RES_BATCH_SUPPORT
                            OC_IF_B |
#endif /* OC_RES_BATCH_SUPPORT */
                              OC_IF_LL | OC_IF_BASELINE,
                            OC_IF_LL, OC_DISCOVERABLE,
                            oc_core_discovery_handler, 0, 0, 0, 1,
                            "oic.wk.res");
}

#ifdef OC_CLIENT
oc_discovery_flags_t
oc_ri_process_discovery_payload(uint8_t *payload, int len,
                                oc_client_handler_t client_handler,
                                oc_endpoint_t *endpoint, void *user_data)
{
  oc_discovery_handler_t handler = client_handler.discovery;
  oc_discovery_all_handler_t all_handler = client_handler.discovery_all;
  bool all = false;
  if (all_handler) {
    all = true;
  }
  oc_discovery_flags_t ret = OC_CONTINUE_DISCOVERY;
  oc_string_t *uri = NULL;
  oc_string_t *anchor = NULL;
  oc_string_array_t *types = NULL;
  oc_interface_mask_t iface_mask = 0;

#ifndef OC_DYNAMIC_ALLOCATION
  char rep_objects_alloc[OC_MAX_NUM_REP_OBJECTS];
  oc_rep_t rep_objects_pool[OC_MAX_NUM_REP_OBJECTS];
  memset(rep_objects_alloc, 0, OC_MAX_NUM_REP_OBJECTS * sizeof(char));
  memset(rep_objects_pool, 0, OC_MAX_NUM_REP_OBJECTS * sizeof(oc_rep_t));
  struct oc_memb rep_objects = { sizeof(oc_rep_t), OC_MAX_NUM_REP_OBJECTS,
                                 rep_objects_alloc, (void *)rep_objects_pool,
                                 0 };
#else  /* !OC_DYNAMIC_ALLOCATION */
  struct oc_memb rep_objects = { sizeof(oc_rep_t), 0, 0, 0, 0 };
#endif /* OC_DYNAMIC_ALLOCATION */
  oc_rep_set_pool(&rep_objects);

  oc_rep_t *links = 0, *rep, *p;
  int s = oc_parse_rep(payload, len, &p);
  if (s != 0) {
    OC_WRN("error parsing discovery response");
  }
  links = rep = p;
  /*  While the oic.wk.res schema over the baseline interface provides for an
   *  array of objects, only one object is present and used in practice.
   *
   *  If rep->value.object != NULL, it means the response was from the baseline
   *  interface, and in that case make rep point to the properties of its first
   *  object. It is traversed in the following loop to obtain a handle to its
   *  array of links.
   */
  if (rep != NULL && rep->value.object) {
    rep = rep->value.object;
  }

  while (rep != NULL) {
    switch (rep->type) {
    /*  Ignore other oic.wk.res properties over here as they're known
     *  and fixed. Only process the "links" property.
     */
    case OC_REP_OBJECT_ARRAY: {
      if (oc_string_len(rep->name) == 5 &&
          memcmp(oc_string(rep->name), "links", 5) == 0) {
        links = rep->value.object_array;
      }
    } break;
    default:
      break;
    }
    rep = rep->next;
  }

  while (links != NULL) {
    /* Reset bm in every round as this can be omitted if 0. */
    oc_uuid_t di;
    oc_resource_properties_t bm = 0;
    oc_endpoint_t *eps_list = NULL;
    oc_rep_t *link = links->value.object;

    while (link != NULL) {
      switch (link->type) {
      case OC_REP_STRING: {
        if (oc_string_len(link->name) == 6 &&
            memcmp(oc_string(link->name), "anchor", 6) == 0) {
          anchor = &link->value.string;
          oc_str_to_uuid(oc_string(*anchor) + 6, &di);
        } else if (oc_string_len(link->name) == 4 &&
                   memcmp(oc_string(link->name), "href", 4) == 0) {
          uri = &link->value.string;
        }
      } break;
      case OC_REP_STRING_ARRAY: {
        size_t i;
        if (oc_string_len(link->name) == 2 &&
            strncmp(oc_string(link->name), "rt", 2) == 0) {
          types = &link->value.array;
        } else {
          iface_mask = 0;
          for (i = 0; i < oc_string_array_get_allocated_size(link->value.array);
               i++) {
            iface_mask |= oc_ri_get_interface_mask(
              oc_string_array_get_item(link->value.array, i),
              oc_string_array_get_item_size(link->value.array, i));
          }
        }
      } break;
      case OC_REP_OBJECT_ARRAY: {
        oc_rep_t *eps = link->value.object_array;
        oc_endpoint_t *eps_cur = NULL;
        oc_endpoint_t temp_ep;
        while (eps != NULL) {
          oc_rep_t *ep = eps->value.object;
          while (ep != NULL) {
            switch (ep->type) {
            case OC_REP_STRING: {
              if (oc_string_len(ep->name) == 2 &&
                  memcmp(oc_string(ep->name), "ep", 2) == 0) {
                if (oc_string_to_endpoint(&ep->value.string, &temp_ep, NULL) ==
                    0) {
                  if (!((temp_ep.flags & IPV6) &&
                        (temp_ep.addr.ipv6.port == 5683)) &&
#ifdef OC_IPV4
                      !((temp_ep.flags & IPV4) &&
                        (temp_ep.addr.ipv4.port == 5683)) &&
#endif /* OC_IPV4 */
                      !(temp_ep.flags & TCP) &&
                      (((endpoint->flags & IPV4) && (temp_ep.flags & IPV6)) ||
                       ((endpoint->flags & IPV6) && (temp_ep.flags & IPV4)))) {
                    goto next_ep;
                  }
                  if (eps_cur) {
                    eps_cur->next = oc_new_endpoint();
                    eps_cur = eps_cur->next;
                  } else {
                    eps_cur = eps_list = oc_new_endpoint();
                  }

                  if (eps_cur) {
                    memcpy(eps_cur, &temp_ep, sizeof(oc_endpoint_t));
                    eps_cur->next = NULL;
                    eps_cur->device = endpoint->device;
                    memcpy(eps_cur->di.id, di.id, 16);
                    eps_cur->interface_index = endpoint->interface_index;
                    oc_endpoint_set_local_address(eps_cur,
                                                  endpoint->interface_index);
                    if (oc_ipv6_endpoint_is_link_local(eps_cur) == 0 &&
                        oc_ipv6_endpoint_is_link_local(endpoint) == 0) {
                      eps_cur->addr.ipv6.scope = endpoint->addr.ipv6.scope;
                    }
                    eps_cur->version = endpoint->version;
                  }
                }
              }
            } break;
            default:
              break;
            }
            ep = ep->next;
          }
        next_ep:
          eps = eps->next;
        }
      } break;
      case OC_REP_OBJECT: {
        oc_rep_t *policy = link->value.object;
        if (policy != NULL && oc_string_len(link->name) == 1 &&
            *(oc_string(link->name)) == 'p' && policy->type == OC_REP_INT &&
            oc_string_len(policy->name) == 2 &&
            memcmp(oc_string(policy->name), "bm", 2) == 0) {
          bm = policy->value.integer;
        }
      } break;
      default:
        break;
      }
      link = link->next;
    }

    if (eps_list &&
        (all ? all_handler(oc_string(*anchor), oc_string(*uri), *types,
                           iface_mask, eps_list, bm,
                           (links->next ? true : false), user_data)
             : handler(oc_string(*anchor), oc_string(*uri), *types, iface_mask,
                       eps_list, bm, user_data)) == OC_STOP_DISCOVERY) {
      oc_free_server_endpoints(eps_list);
      ret = OC_STOP_DISCOVERY;
      goto done;
    }
    oc_free_server_endpoints(eps_list);
    links = links->next;
  }

done:
  oc_free_rep(p);
#ifdef OC_DNS_CACHE
  oc_dns_clear_cache();
#endif /* OC_DNS_CACHE */
  return ret;
}
#endif /* OC_CLIENT */
