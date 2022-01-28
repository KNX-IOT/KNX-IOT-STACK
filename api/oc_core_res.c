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

#include "oc_core_res.h"
#include "oc_api.h"
#include "messaging/coap/oc_coap.h"
#include "oc_discovery.h"
#include "oc_rep.h"

#include "oc_knx.h"
#include "oc_knx_dev.h"
#include "oc_knx_fb.h"
#include "oc_knx_fp.h"
#include "oc_knx_swu.h"
#include "oc_knx_sec.h"

#ifdef OC_SECURITY
#include "security/oc_doxm.h"
#include "security/oc_pstat.h"
#include "security/oc_tls.h"
#endif /* OC_SECURITY */

#include "port/oc_assert.h"
#include <stdarg.h>

#ifdef OC_DYNAMIC_ALLOCATION
#include "oc_endpoint.h"
#include <stdlib.h>
static oc_resource_t *core_resources = NULL;
static oc_device_info_t *oc_device_info = NULL;
#else  /* OC_DYNAMIC_ALLOCATION */
static oc_resource_t core_resources[1 + OCF_D * OC_MAX_NUM_DEVICES];
static oc_device_info_t oc_device_info[OC_MAX_NUM_DEVICES];
#endif /* !OC_DYNAMIC_ALLOCATION */
static oc_platform_info_t oc_platform_info;

static int res_latency = 0;
static size_t device_count = 0;

void
oc_core_init(void)
{
  oc_core_shutdown();

#ifdef OC_DYNAMIC_ALLOCATION
  core_resources = (oc_resource_t *)calloc(1, sizeof(oc_resource_t));
  if (!core_resources) {
    printf("COULD NOT ALLOCATE CORE RESOURCE\n\n\n\n\n");
    oc_abort("Insufficient memory");
  }

  oc_device_info = NULL;
#endif /* OC_DYNAMIC_ALLOCATION */
}

static void
oc_core_free_device_info_properties(oc_device_info_t *oc_device_info_item)
{

  if (oc_device_info_item) {
    oc_free_string(&(oc_device_info_item->name));
    oc_free_string(&(oc_device_info_item->icv));
    oc_free_string(&(oc_device_info_item->dmv));
    // KNX
    oc_free_string(&(oc_device_info_item->serialnumber));
    oc_free_string(&(oc_device_info_item->hwt));
    oc_free_string(&(oc_device_info_item->model));
    oc_free_string(&(oc_device_info_item->hostname));
    oc_free_string(&(oc_device_info_item->iid));
  }
}

void
oc_core_shutdown(void)
{
  size_t i;
  oc_free_string(&(oc_platform_info.mfg_name));

#ifdef OC_DYNAMIC_ALLOCATION
  if (oc_device_info) {
#endif /* OC_DYNAMIC_ALLOCATION */
    for (i = 0; i < device_count; ++i) {
      oc_device_info_t *oc_device_info_item = &oc_device_info[i];
      oc_core_free_device_info_properties(oc_device_info_item);
    }
#ifdef OC_DYNAMIC_ALLOCATION
    free(oc_device_info);
    oc_device_info = NULL;
  }
#endif /* OC_DYNAMIC_ALLOCATION */

#ifdef OC_DYNAMIC_ALLOCATION
  if (core_resources) {
#endif /* OC_DYNAMIC_ALLOCATION */
    for (i = 0; i < 1 + (WELLKNOWNCORE * device_count); ++i) {
      oc_resource_t *core_resource = &core_resources[i];
      oc_ri_free_resource_properties(core_resource);
    }
#ifdef OC_DYNAMIC_ALLOCATION
    free(core_resources);
    core_resources = NULL;
  }
#endif /* OC_DYNAMIC_ALLOCATION */
  device_count = 0;
}

void
oc_core_encode_interfaces_mask(CborEncoder *parent,
                               oc_interface_mask_t iface_mask)
{
  oc_rep_set_key((parent), "if");
  oc_rep_start_array((parent), if);

  if (iface_mask & OC_IF_I) {
    oc_rep_add_text_string(if, "if.i");
  }
  if (iface_mask & OC_IF_O) {
    oc_rep_add_text_string(if, "if.o");
  }
  if (iface_mask & OC_IF_G) {
    oc_rep_add_text_string(if, "if.g.s");
    // TODO note: this must be extended with a number...
  }
  if (iface_mask & OC_IF_C) {
    oc_rep_add_text_string(if, "if.c");
  }
  if (iface_mask & OC_IF_P) {
    oc_rep_add_text_string(if, "if.p");
  }
  if (iface_mask & OC_IF_D) {
    oc_rep_add_text_string(if, "if.d");
  }
  if (iface_mask & OC_IF_A) {
    oc_rep_add_text_string(if, "if.a");
  }
  if (iface_mask & OC_IF_S) {
    oc_rep_add_text_string(if, "if.s");
  }
  if (iface_mask & OC_IF_LI) {
    oc_rep_add_text_string(if, "if.ll");
  }
  if (iface_mask & OC_IF_B) {
    oc_rep_add_text_string(if, "if.b");
  }
  if (iface_mask & OC_IF_SEC) {
    oc_rep_add_text_string(if, "if.sec");
  }
  if (iface_mask & OC_IF_SWU) {
    oc_rep_add_text_string(if, "if.swu");
  }
  if (iface_mask & OC_IF_PM) {
    oc_rep_add_text_string(if, "if.pm");
  }

  oc_rep_end_array((parent), if);
}

int
oc_frame_interfaces_mask_in_response(oc_interface_mask_t iface_mask)
{
  int total_size = 0;

  if (iface_mask & OC_IF_I) {
    oc_rep_encode_raw((uint8_t *)"if.i", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_O) {
    oc_rep_encode_raw((uint8_t *)"if.o", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_G) {
    oc_rep_encode_raw((uint8_t *)"if.g.s", 5);
    // TODO note: this must be extended with a number...
    total_size += 5;
  }
  if (iface_mask & OC_IF_C) {
    oc_rep_encode_raw((uint8_t *)"if.c", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_P) {
    oc_rep_encode_raw((uint8_t *)"if.p", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_D) {
    oc_rep_encode_raw((uint8_t *)"if.d", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_A) {
    oc_rep_encode_raw((uint8_t *)"if.a", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_S) {
    oc_rep_encode_raw((uint8_t *)"if.s", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_LI) {
    oc_rep_encode_raw((uint8_t *)"if.ll", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_B) {
    oc_rep_encode_raw((uint8_t *)"if.b", 4);
    total_size += 4;
  }
  if (iface_mask & OC_IF_SEC) {
    oc_rep_encode_raw((uint8_t *)"if.sec", 6);
    total_size += 6;
  }
  if (iface_mask & OC_IF_SWU) {
    oc_rep_encode_raw((uint8_t *)"if.swu", 6);
    total_size += 6;
  }
  if (iface_mask & OC_IF_PM) {
    oc_rep_encode_raw((uint8_t *)"if.pm", 5);
    total_size += 5;
  }
  return total_size;
}

size_t
oc_core_get_num_devices(void)
{
  return device_count;
}

void
oc_core_set_latency(int latency)
{
  res_latency = latency;
}

int
oc_core_get_latency(void)
{
  return res_latency;
}

int
oc_core_set_device_fwv(int device_index, int major, int minor, int minor2)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("device_index %d to large\n", device_index);
    return -1;
  }
  oc_device_info[device_index].fwv.major = major;
  oc_device_info[device_index].fwv.minor = minor;
  oc_device_info[device_index].fwv.third = minor2;
  return 0;
}

int
oc_core_set_device_hwv(int device_index, int major, int minor, int minor2)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("device_index %d to large\n", device_index);
    return -1;
  }

  oc_device_info[device_index].hwv.major = major;
  oc_device_info[device_index].hwv.minor = minor;
  oc_device_info[device_index].hwv.third = minor2;
  return 0;
}

int
oc_core_set_device_ia(int device_index, int ia)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("device_index %d to large\n", device_index);
    return -1;
  }
  oc_device_info[device_index].ia = ia;
  return 0;
}

int
oc_core_set_device_hwt(int device_index, const char *hardwaretype)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("device_index %d to large\n", device_index);
    return -1;
  }

  oc_free_string(&oc_device_info[device_index].hwt);
  oc_new_string(&oc_device_info[device_index].hwt, hardwaretype,
                strlen(hardwaretype));

  return 0;
}

int
oc_core_set_device_pm(int device_index, bool pm)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("  device_index %d to large\n", device_index);
    return -1;
  }

  oc_device_info[device_index].pm = pm;

  return 0;
}

int
oc_core_set_device_model(int device_index, const char *model)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("  device_index %d to large\n", device_index);
    return -1;
  }
  oc_free_string(&oc_device_info[device_index].model);
  oc_new_string(&oc_device_info[device_index].model, model, strlen(model));

  return 0;
}

int
oc_core_set_device_hostname(int device_index, const char *hostname)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("  device_index %d to large\n", device_index);
    return -1;
  }
  oc_free_string(&oc_device_info[device_index].hostname);
  oc_new_string(&oc_device_info[device_index].hostname, hostname,
                strlen(hostname));

  return 0;
}

int
oc_core_set_device_iid(int device_index, const char *iid)
{
  if (device_index >= (int)oc_number_of_devices()) {
    PRINT("  device_index %d to large\n", device_index);
    return -1;
  }
  oc_free_string(&oc_device_info[device_index].iid);
  oc_new_string(&oc_device_info[device_index].iid, iid, strlen(iid));

  return 0;
}

/*
oc_device_info_t *
oc_core_add_new_device(const char *uri, const char *rt, const char *name,
                       const char *spec_version, const char *data_model_version,
                       oc_core_add_device_cb_t add_device_cb, void *data)
{
  (void)data;
  (void)uri;
  (void)rt;
#ifndef OC_DYNAMIC_ALLOCATION
  if (device_count == OC_MAX_NUM_DEVICES) {
    OC_ERR("device limit reached");
    return NULL;
  }
#else // !OC_DYNAMIC_ALLOCATION
  size_t new_num = 1 + OCF_D * (device_count + 1);
  core_resources =
    (oc_resource_t *)realloc(core_resources, new_num * sizeof(oc_resource_t));

  if (!core_resources) {
    oc_abort("Insufficient memory");
  }
  oc_resource_t *device = &core_resources[new_num - OCF_D];
  memset(device, 0, OCF_D * sizeof(oc_resource_t));

  oc_device_info = (oc_device_info_t *)realloc(
    oc_device_info, (device_count + 1) * sizeof(oc_device_info_t));

  if (!oc_device_info) {
    oc_abort("Insufficient memory");
  }
  memset(&oc_device_info[device_count], 0, sizeof(oc_device_info_t));

#endif // OC_DYNAMIC_ALLOCATION

  oc_gen_uuid(&oc_device_info[device_count].di);

  // Construct device resource
  //int properties = OC_DISCOVERABLE;

 // if (strlen(rt) == 8 && strncmp(rt, "oic.wk.d", 8) == 0) {
 //   oc_core_populate_resource(OCF_D, device_count, uri,
 //                             OC_IF_R | OC_IF_BASELINE, OC_IF_R, properties,
//                              oc_core_device_handler, 0, 0, 0, 1, rt);
//  } else {
//    oc_core_populate_resource(
//      OCF_D, device_count, uri, OC_IF_R | OC_IF_BASELINE, OC_IF_R, properties,
 //     oc_core_device_handler, 0, 0, 0, 2, rt, "oic.wk.d");
//  }

  oc_gen_uuid(&oc_device_info[device_count].piid);

  oc_new_string(&oc_device_info[device_count].name, name, strlen(name));
  oc_new_string(&oc_device_info[device_count].icv, spec_version,
                strlen(spec_version));
  oc_new_string(&oc_device_info[device_count].dmv, data_model_version,
                strlen(data_model_version));
  oc_device_info[device_count].add_device_cb = add_device_cb;

  oc_create_discovery_resource(OCF_RES, device_count);

  oc_create_discovery_resource(WELLKNOWNCORE, device_count);

  oc_device_info[device_count].data = data;

  if (oc_connectivity_init(device_count) < 0) {
    oc_abort("error initializing connectivity for device");
  }

  device_count++;

  return &oc_device_info[device_count - 1];
}
*/

oc_device_info_t *
oc_core_add_device(const char *name, const char *version, const char *base,
                   const char *serialnumber,
                   oc_core_add_device_cb_t add_device_cb, void *data)
{
  (void)data;
#ifndef OC_DYNAMIC_ALLOCATION
  if (device_count == OC_MAX_NUM_DEVICES) {
    OC_ERR("device limit reached");
    return NULL;
  }
#else /* !OC_DYNAMIC_ALLOCATION */
  size_t new_num = 1 + WELLKNOWNCORE * (device_count + 1);
  core_resources =
    (oc_resource_t *)realloc(core_resources, new_num * sizeof(oc_resource_t));

  if (!core_resources) {
    oc_abort("Insufficient memory");
  }
  oc_resource_t *device = &core_resources[new_num - WELLKNOWNCORE];
  memset(device, 0, WELLKNOWNCORE * sizeof(oc_resource_t));

  oc_device_info = (oc_device_info_t *)realloc(
    oc_device_info, (device_count + 1) * sizeof(oc_device_info_t));

  if (!oc_device_info) {
    oc_abort("Insufficient memory");
  }
  memset(&oc_device_info[device_count], 0, sizeof(oc_device_info_t));

#endif /* OC_DYNAMIC_ALLOCATION */

  oc_gen_uuid(&oc_device_info[device_count].di);

  /* Construct device resource */
  // int properties = OC_DISCOVERABLE;

  oc_gen_uuid(&oc_device_info[device_count].piid);

  oc_new_string(&oc_device_info[device_count].name, name, strlen(name));
  oc_new_string(&oc_device_info[device_count].icv, version, strlen(version));
  oc_new_string(&oc_device_info[device_count].dmv, base, strlen(base));
  oc_new_string(&oc_device_info[device_count].serialnumber, serialnumber,
                strlen(serialnumber));
  oc_device_info[device_count].add_device_cb = add_device_cb;

  oc_create_discovery_resource(WELLKNOWNCORE, device_count);

  oc_create_knx_device_resources(device_count);
  oc_create_knx_resources(device_count);
  oc_create_knx_fp_resources(device_count);
  oc_create_knx_sec_resources(device_count);
  oc_create_knx_swu_resources(device_count);
  oc_create_knx_fb_resources(device_count);

  oc_device_info[device_count].data = data;

  if (oc_connectivity_init(device_count) < 0) {
    oc_abort("error initializing connectivity for device");
  }

  device_count++;

  return &oc_device_info[device_count - 1];
}

static void
oc_device_bind_rt(size_t device_index, const char *rt)
{
  oc_resource_t *r = oc_core_get_resource_by_index(WELLKNOWNCORE, device_index);
  oc_string_array_t types;

  memcpy(&types, &r->types, sizeof(oc_string_array_t));

  size_t num_types = oc_string_array_get_allocated_size(types);
  num_types++;

  memset(&r->types, 0, sizeof(oc_string_array_t));
  oc_new_string_array(&r->types, num_types);
  size_t i;
  for (i = 0; i < num_types; i++) {
    if (i == 0) {
      oc_string_array_add_item(r->types, rt);
    } else {
      oc_string_array_add_item(r->types,
                               oc_string_array_get_item(types, (i - 1)));
    }
  }
  oc_free_string_array(&types);
}

void
oc_device_bind_resource_type(size_t device, const char *type)
{
  oc_device_bind_rt(device, type);
}

oc_platform_info_t *
oc_core_init_platform(const char *mfg_name, oc_core_init_platform_cb_t init_cb,
                      void *data)
{
  if (oc_platform_info.mfg_name.size > 0) {
    return &oc_platform_info;
  }

  /* Populating resource object */
  // int properties = OC_DISCOVERABLE;

  // oc_core_populate_resource(OCF_P, 0, "oic/p", OC_IF_R | OC_IF_BASELINE,
  //                          OC_IF_R, properties, oc_core_platform_handler, 0,
  //                          0, 0, 1, "oic.wk.p");

  oc_gen_uuid(&oc_platform_info.pi);

  oc_new_string(&oc_platform_info.mfg_name, mfg_name, strlen(mfg_name));
  oc_platform_info.init_platform_cb = init_cb;
  oc_platform_info.data = data;

  return &oc_platform_info;
}

void
oc_store_uri(const char *s_uri, oc_string_t *d_uri)
{
  if (s_uri[0] != '/') {
    size_t s_len = strlen(s_uri);
    oc_alloc_string(d_uri, s_len + 2);
    memcpy((char *)oc_string(*d_uri) + 1, s_uri, s_len);
    ((char *)oc_string(*d_uri))[0] = '/';
    ((char *)oc_string(*d_uri))[s_len + 1] = '\0';
  } else {
    oc_new_string(d_uri, s_uri, strlen(s_uri));
  }
}

void
oc_core_populate_resource(int core_resource, size_t device_index,
                          const char *uri, oc_interface_mask_t iface_mask,
                          oc_interface_mask_t default_interface, int properties,
                          oc_request_callback_t get, oc_request_callback_t put,
                          oc_request_callback_t post,
                          oc_request_callback_t delete, int num_resource_types,
                          ...)
{
  oc_resource_t *r = oc_core_get_resource_by_index(core_resource, device_index);
  if (!r) {
    return;
  }
  r->device = device_index;
  oc_store_uri(uri, &r->uri);
  r->properties = properties;
  va_list rt_list;
  int i;
  va_start(rt_list, num_resource_types);
  if (num_resource_types > 0) {
    oc_new_string_array(&r->types, num_resource_types);
    for (i = 0; i < num_resource_types; i++) {
      oc_string_array_add_item(r->types, va_arg(rt_list, const char *));
    }
  }
  va_end(rt_list);
  r->interfaces = iface_mask;
  r->default_interface = default_interface;
  r->content_type = APPLICATION_VND_OCF_CBOR;
  r->get_handler.cb = get;
  r->put_handler.cb = put;
  r->post_handler.cb = post;
  r->delete_handler.cb = delete;
}

void
oc_core_lf_populate_resource(int core_resource, size_t device_index,
                             const char *uri, oc_interface_mask_t iface_mask,
                             oc_content_format_t content_type, int properties,
                             oc_request_callback_t get,
                             oc_request_callback_t put,
                             oc_request_callback_t post,
                             oc_request_callback_t delete,
                             int num_resource_types, ...)
{
  oc_resource_t *r = oc_core_get_resource_by_index(core_resource, device_index);
  if (!r) {
    return;
  }
  r->device = device_index;
  oc_store_uri(uri, &r->uri);
  r->properties = properties;
  va_list rt_list;
  int i;
  va_start(rt_list, num_resource_types);
  if (num_resource_types > 0) {
    oc_new_string_array(&r->types, num_resource_types);
    for (i = 0; i < num_resource_types; i++) {
      oc_string_array_add_item(r->types, va_arg(rt_list, const char *));
    }
  }
  va_end(rt_list);
  r->interfaces = iface_mask;
  r->default_interface = iface_mask;
  r->content_type = content_type;
  r->get_handler.cb = get;
  r->put_handler.cb = put;
  r->post_handler.cb = post;
  r->delete_handler.cb = delete;
}

oc_uuid_t *
oc_core_get_device_id(size_t device)
{
  if (device >= device_count) {
    return NULL;
  }
  return &oc_device_info[device].di;
}

oc_device_info_t *
oc_core_get_device_info(size_t device)
{
  if (device >= device_count) {
    return NULL;
  }
  return &oc_device_info[device];
}

oc_platform_info_t *
oc_core_get_platform_info(void)
{
  return &oc_platform_info;
}

oc_resource_t *
oc_core_get_resource_by_index(int type, size_t device)
{
  if (type == OC_DEV_SN) {
    return &core_resources[0];
  }
  return &core_resources[WELLKNOWNCORE * device + type];
}

#ifdef OC_SECURITY
bool
oc_core_is_SVR(oc_resource_t *resource, size_t device)
{
  size_t device_svrs = WELLKNOWNCORE * device + OCF_SEC_DOXM;

  size_t SVRs_end = (device + 1) * WELLKNOWNCORE - 1, i;
  for (i = device_svrs; i <= SVRs_end; i++) {
    if (resource == &core_resources[i]) {
      return true;
    }
  }

  return false;
}
#endif /* OC_SECURITY */

bool
oc_core_is_vertical_resource(oc_resource_t *resource, size_t device)
{
  if (resource == &core_resources[0]) {
    return true;
  }

  size_t device_resources = WELLKNOWNCORE * device;

  size_t DCRs_end = device_resources + WELLKNOWNCORE, i;
  for (i = device_resources + 1; i <= DCRs_end; i++) {
    if (resource == &core_resources[i]) {
      return false;
    }
  }

  return true;
}

bool
oc_core_is_DCR(oc_resource_t *resource, size_t device)
{
  if (resource == &core_resources[0]) {
    return true;
  }

  size_t device_resources = WELLKNOWNCORE * device;

  size_t DCRs_end = device_resources + WELLKNOWNCORE, i;
  for (i = device_resources + 1; i <= DCRs_end; i++) {
    if (resource == &core_resources[i]) {
      if (i == (device_resources + WELLKNOWNCORE)) {
        return false;
      }
      return true;
    }
  }

  return false;
}

oc_resource_t *
oc_core_get_resource_by_uri(const char *uri, size_t device)
{
  int skip = 0, type = 0;
  if (uri[0] == '/')
    skip = 1;

  // TODO: need to add all other KNX resources, not sure though if this function
  // is being used anywhere
  if ((strlen(uri) - skip) == 7 &&
      memcmp(uri + skip, ".well-known/core", 17) == 0) {
    type = WELLKNOWNCORE;
  } else if ((strlen(uri) - skip) == 7 &&
             memcmp(uri + skip, ".well-known/knx", 15) == 0) {
    type = OC_KNX;
  }

#ifdef OC_SECURITY
  else if ((strlen(uri) - skip) == 12) {
    if (memcmp(uri + skip, "oic/sec/doxm", 12) == 0) {
      type = OCF_SEC_DOXM;
    } else if (memcmp(uri + skip, "oic/sec/pstat", 12) == 0) {
      type = OCF_SEC_PSTAT;
    } else if (memcmp(uri + skip, "oic/sec/acl2", 12) == 0) {
      type = OCF_SEC_ACL;
    } else if (memcmp(uri + skip, "oic/sec/ael", 11) == 0) {
      type = OCF_SEC_AEL;
    } else if (memcmp(uri + skip, "oic/sec/cred", 12) == 0) {
      type = OCF_SEC_CRED;
    }
  } else if ((strlen(uri) - skip) == 10 &&
             memcmp(uri + skip, "oic/sec/sp", 10) == 0) {
    type = OCF_SEC_SP;
  }
#ifdef OC_PKI
  else if ((strlen(uri) - skip) == 11 &&
           memcmp(uri + skip, "oic/sec/csr", 11) == 0) {
    type = OCF_SEC_CSR;
  } else if ((strlen(uri) - skip) == 13 &&
             memcmp(uri + skip, "oic/sec/roles", 13) == 0) {
    type = OCF_SEC_ROLES;
  }
#endif /* OC_PKI */
  else if ((strlen(uri) - skip) == 11 &&
           memcmp(uri + skip, "oic/sec/sdi", 11) == 0) {
    type = OCF_SEC_SDI;
  }
#endif /* OC_SECURITY */
  else {
    return NULL;
  }
  size_t res = WELLKNOWNCORE * device + type;
  return &core_resources[res];
}

bool
oc_filter_resource_by_rt(oc_resource_t *resource, oc_request_t *request)
{
  bool match = true, more_query_params = false;
  char *rt = NULL;
  int rt_len = -1;
  oc_init_query_iterator();
  do {
    more_query_params =
      oc_iterate_query_get_values(request, "rt", &rt, &rt_len);

    if (rt_len > 0) {

      /* adapt size when a wild card exists */
      char *wildcart = memchr(rt, '*', rt_len);
      if (wildcart != NULL) {
        rt_len = wildcart - rt;
      }

      match = false;
      int i;
      for (i = 0; i < (int)oc_string_array_get_allocated_size(resource->types);
           i++) {
        size_t size = oc_string_array_get_item_size(resource->types, i);
        const char *t =
          (const char *)oc_string_array_get_item(resource->types, i);
        if (wildcart != NULL) {
          if (strncmp(rt, t, rt_len) == 0) {
            return true;
          }
        }
        if (rt_len == (int)size && strncmp(rt, t, rt_len) == 0) {
          return true;
        }
      }
    }
  } while (more_query_params);
  return match;
}

bool
oc_filter_resource_by_if(oc_resource_t *resource, oc_request_t *request)
{
  bool match = true, more_query_params = false;
  char *value = NULL;
  int value_len = -1;
  oc_init_query_iterator();
  do {
    more_query_params =
      oc_iterate_query_get_values(request, "if", &value, &value_len);

    if (value_len > 8) {

      /* adapt size when a wild card exists */
      char *wildcart = memchr(value, '*', value_len);
      if (wildcart != NULL) {
        /* wild card means that everything matches */
        return true;
      }

      match = false;
      const char *resource_interface =
        get_interface_string(resource->interfaces);
      // the value contains urn:knx:if.xxx
      if (strncmp(resource_interface, value + 8, value_len - 8) == 0) {
        return true;
      }
    }
  } while (more_query_params);
  return match;
}

size_t
oc_number_of_devices()
{
  return (size_t)device_count;
}
