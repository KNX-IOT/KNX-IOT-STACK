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
#include "oc_knx_p.h"
#include "oc_knx_swu.h"
#include "oc_knx_sec.h"
#include "oc_knx_sub.h"
#ifdef OC_IOT_ROUTER
#include "oc_knx_gm.h"
#endif /* OC_IOT_ROUTER */

#include "port/oc_assert.h"
#include <stdarg.h>

#ifdef OC_DYNAMIC_ALLOCATION
#include "oc_endpoint.h"
#include <stdlib.h>
OC_LIST(core_resource_list);
static oc_resource_t *core_resources = NULL;
static oc_device_info_t *oc_device_info = NULL;
#else  /* OC_DYNAMIC_ALLOCATION */
// TODO fix this for static allocation, this is not used at the moment..
static oc_resource_t core_resources[1 + OCF_D * (OC_MAX_NUM_DEVICES - 1)];
static oc_device_info_t oc_device_info[OC_MAX_NUM_DEVICES];
#endif /* !OC_DYNAMIC_ALLOCATION */
static oc_platform_info_t oc_platform_info;

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
    // KNX
    oc_free_string(&(oc_device_info_item->serialnumber));
    oc_free_string(&(oc_device_info_item->hwt));
    oc_free_string(&(oc_device_info_item->model));
    oc_free_string(&(oc_device_info_item->hostname));
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
    //
    for (i = 0; i < device_count; ++i) {
      oc_free_knx_fp_resources(i);
    }

#ifdef OC_DYNAMIC_ALLOCATION
    free(oc_device_info);
    oc_device_info = NULL;
  }
#endif /* OC_DYNAMIC_ALLOCATION */

#ifdef OC_DYNAMIC_ALLOCATION
  if (core_resources) {
#endif /* OC_DYNAMIC_ALLOCATION */
    size_t max_resource =
      1 + (WELLKNOWNCORE * (device_count ? device_count - 1 : 0));
    for (i = 0; i < max_resource; ++i) {
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
oc_frame_interfaces_mask_in_response(oc_interface_mask_t iface_mask,
                                     int truncate)
{
  int total_size = 0;
  //  </ point - path - example1>;rt = ":dpa.352.51";if= ":if.i";ct = 50 60,

  // start quote
  oc_rep_encode_raw((uint8_t *)"\"", 1);
  total_size += 1;

  if (iface_mask & OC_IF_I) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.i", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_O) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.o", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_G) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.g.s", 6);
    total_size += 6;
  }
  if (iface_mask & OC_IF_C) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.c", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_P) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.p", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_D) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.d", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_A) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.a", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_S) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    oc_rep_encode_raw((uint8_t *)":if.s", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_LI) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.ll", 6);
    total_size += 6;
  }
  if (iface_mask & OC_IF_B) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.b", 5);
    total_size += 5;
  }
  if (iface_mask & OC_IF_SEC) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.sec", 7);
    total_size += 7;
  }
  if (iface_mask & OC_IF_SWU) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.swu", 7);
    total_size += 7;
  }
  if (iface_mask & OC_IF_PM) {
    if (total_size > 1) {
      oc_rep_encode_raw((uint8_t *)" ", 1);
      total_size += 1;
    }
    if (truncate != 1) {
      oc_rep_encode_raw((uint8_t *)"urn:knx", 7);
      total_size += 7;
    }
    oc_rep_encode_raw((uint8_t *)":if.pm", 6);
    total_size += 6;
  }

  // end quote
  oc_rep_encode_raw((uint8_t *)"\"", 1);
  total_size += 1;
  return total_size;
}

size_t
oc_core_get_num_devices(void)
{
  return device_count;
}

int
oc_core_set_device_fwv(size_t device_index, int major, int minor, int minor2)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("device_index %d too large\n", (int)device_index);
    return -1;
  }
  oc_device_info[device_index].fwv.major = major;
  oc_device_info[device_index].fwv.minor = minor;
  oc_device_info[device_index].fwv.patch = minor2;
  return 0;
}

int
oc_core_set_device_hwv(size_t device_index, int major, int minor, int minor2)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("device_index %d too large\n", (int)device_index);
    return -1;
  }

  oc_device_info[device_index].hwv.major = major;
  oc_device_info[device_index].hwv.minor = minor;
  oc_device_info[device_index].hwv.patch = minor2;
  return 0;
}

int
oc_core_set_device_ap(size_t device_index, int major, int minor, int minor2)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("device_index %d too large\n", (int)device_index);
    return -1;
  }

  oc_device_info[device_index].ap.major = major;
  oc_device_info[device_index].ap.minor = minor;
  oc_device_info[device_index].ap.patch = minor2;
  return 0;
}

int
oc_core_set_device_mid(size_t device_index, uint32_t mid)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("device_index %d too large\n", (int)device_index);
    return -1;
  }
  oc_device_info[device_index].mid = mid;
  return 0;
}

int
oc_core_set_device_ia(size_t device_index, uint32_t ia)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("device_index %d too large\n", (int)device_index);
    return -1;
  }
  oc_device_info[device_index].ia = ia;
  return 0;
}

int
oc_core_set_and_store_device_ia(size_t device_index, uint32_t ia)
{
  const int status = oc_core_set_device_ia(device_index, ia);

  // If successful write to storage
  if (status == 0) {
    oc_storage_write(KNX_STORAGE_IA, (uint8_t *)&ia, sizeof(ia));
  }

  return status;
}

int
oc_core_set_device_hwt(size_t device_index, const char *hardwaretype)
{
  int hwt_len = 0;
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("device_index %d too large\n", (int)device_index);
    return -1;
  }
  hwt_len = strlen(hardwaretype);
  // if (strlen(hardwaretype) > 6) {
  //  truncate the hardware type
  // hwt_len = 6;
  //}

  oc_free_string(&oc_device_info[device_index].hwt);
  oc_new_string(&oc_device_info[device_index].hwt, hardwaretype, hwt_len);

  return 0;
}

int
oc_core_set_device_pm(size_t device_index, bool pm)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("  device_index %d too large\n", (int)device_index);
    return -1;
  }

  oc_device_info[device_index].pm = pm;

  return 0;
}

int
oc_core_set_device_model(size_t device_index, const char *model)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("  device_index %d too large\n", (int)device_index);
    return -1;
  }
  oc_free_string(&oc_device_info[device_index].model);
  oc_new_string(&oc_device_info[device_index].model, model, strlen(model));

  return 0;
}

int
oc_core_set_device_hostname(size_t device_index, const char *host_name)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("  device_index %d too large\n", (int)device_index);
    return -1;
  }
  oc_free_string(&oc_device_info[device_index].hostname);
  oc_new_string(&oc_device_info[device_index].hostname, host_name,
                strlen(host_name));

  return 0;
}

int
oc_core_set_device_iid(size_t device_index, uint64_t iid)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("  device_index %d too large\n", (int)device_index);
    return -1;
  }
  oc_device_info[device_index].iid = iid;

  printf("iid set: ");
  oc_print_uint64_t(iid, DEC_REPRESENTATION);
  printf("\n");

  return 0;
}

uint64_t
oc_core_get_device_iid(size_t device_index)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("  device_index %d too large\n", (int)device_index);
    return -1;
  }

  return oc_device_info[device_index].iid;
}

int
oc_core_set_and_store_device_iid(size_t device_index, uint64_t iid)
{
  const int status = oc_core_set_device_iid(device_index, iid);

  // If successful write to storage
  if (status == 0) {
    oc_storage_write(KNX_STORAGE_IID, (uint8_t *)&iid, sizeof(iid));
  }

  return status;
}

int
oc_core_set_device_fid(size_t device_index, uint64_t fid)
{
  if (device_index >= (int)oc_core_get_num_devices()) {
    OC_ERR("  device_index %d too large\n", (int)device_index);
    return -1;
  }
  oc_device_info[device_index].fid = fid;

  return 0;
}

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
  // note: there is always 1 resource, the initial one in the list
  size_t new_num = 1 + WELLKNOWNCORE * device_count;

  core_resources =
    (oc_resource_t *)realloc(core_resources, new_num * sizeof(oc_resource_t));

  if (!core_resources) {
    oc_abort("Insufficient memory");
  }
  if (device_count > 0) {
    oc_resource_t *device_resources = &core_resources[new_num - WELLKNOWNCORE];
    memset(device_resources, 0, WELLKNOWNCORE * sizeof(oc_resource_t));
  } else {
    // device 0 is compile-time generated
    oc_device_info = (oc_device_info_t *)realloc(
      oc_device_info, (device_count + 1) * sizeof(oc_device_info_t));
    if (!oc_device_info) {
      oc_abort("Insufficient memory");
    }
    memset(&oc_device_info[device_count], 0, sizeof(oc_device_info_t));
    OC_CORE_EXTERN_CONST_RESOURCE(dev_sn);
    oc_list_add_block(core_resource_list,
                      (oc_resource_t *)&OC_CORE_RESOURCE_NAME(dev_sn));
  }

  oc_device_info = (oc_device_info_t *)realloc(
    oc_device_info, (device_count + 1) * sizeof(oc_device_info_t));

  if (!oc_device_info) {
    oc_abort("Insufficient memory");
  }
  memset(&oc_device_info[device_count], 0, sizeof(oc_device_info_t));
  oc_device_info[device_count].ia = 0xffff;

#endif /* OC_DYNAMIC_ALLOCATION */

  /* Construct device resource */
  // int properties = OC_DISCOVERABLE;

  // make sure that the serial number used is in lower case
  char serial_lower[20];
  strncpy(serial_lower, serialnumber, 19);
  oc_char_convert_to_lower(serial_lower);

  oc_new_string(&oc_device_info[device_count].serialnumber, serial_lower,
                strlen(serial_lower));
  oc_device_info[device_count].add_device_cb = add_device_cb;

  oc_create_discovery_resource(WELLKNOWNCORE, device_count);

  oc_create_knx_device_resources(device_count);
  oc_create_knx_resources(device_count);
  oc_create_knx_fb_resources(device_count);
  oc_create_knx_fp_resources(device_count);
  oc_create_knx_p_resources(device_count);
  oc_create_knx_sec_resources(device_count);
  oc_create_knx_swu_resources(device_count);
  oc_create_sub_resource(OC_KNX_SUB, device_count);
#ifdef OC_IOT_ROUTER
  oc_create_knx_iot_router_resources(device_count);
#endif /* OC_IOT_ROUTER */

  oc_device_info[device_count].data = data;

  if (oc_connectivity_init(device_count) < 0) {
    oc_abort("error initializing connectivity for device");
  }

  /* must be before the increase of device_count */
  oc_init_oscore_from_storage(device_count, true);

  device_count++;

  return &oc_device_info[device_count - 1];
}

oc_platform_info_t *
oc_core_init_platform(const char *mfg_name, oc_core_init_platform_cb_t init_cb,
                      void *data)
{
  if (oc_platform_info.mfg_name.size > 0) {
    return &oc_platform_info;
  }

  oc_new_string(&oc_platform_info.mfg_name, mfg_name, strlen(mfg_name));
  oc_platform_info.init_platform_cb = init_cb;
  oc_platform_info.data = data;

  return &oc_platform_info;
}

void
oc_check_uri(const char *uri)
{
  oc_assert(uri[0] == '/');
}

void
oc_core_populate_resource(int core_resource, size_t device_index,
                          const char *uri, oc_interface_mask_t iface_mask,
                          oc_content_format_t content_type, int properties,
                          oc_request_callback_t get, oc_request_callback_t put,
                          oc_request_callback_t post,
                          oc_request_callback_t delete, int num_resource_types,
                          ...)
{
  const oc_resource_t *_r =
    oc_core_get_resource_by_index(core_resource, device_index);
  if (!_r) {
    return;
  }
  if (_r->is_const) {
    OC_ERR("oc_core_populate_resource: resource is const");
    return;
  }
  oc_resource_t *r = (oc_resource_t *)_r;
  r->device = device_index;
  oc_check_uri(uri);
  r->uri.next = NULL;
  r->uri.ptr = (char *)uri;
  r->uri.size = strlen(uri) + 1; // include null terminator in size
  r->properties = properties;
  va_list rt_list;
  int i;
  va_start(rt_list, num_resource_types);
  if (num_resource_types > 0) {
    oc_new_string_array(&r->types, num_resource_types);
    for (i = 0; i < num_resource_types; i++) {
      const char *resource_type = va_arg(rt_list, const char *);
      oc_assert(strlen(resource_type) < STRING_ARRAY_ITEM_MAX_LEN);
      oc_string_array_add_item(r->types, resource_type);
    }
  }
  va_end(rt_list);
  r->interfaces = iface_mask;
  r->content_type = content_type;
  r->get_handler.cb = get;
  r->put_handler.cb = put;
  r->post_handler.cb = post;
  r->delete_handler.cb = delete;
}

void
oc_core_bind_dpt_resource(int core_resource, size_t device_index,
                          const char *dpt)
{
  const oc_resource_t *r =
    oc_core_get_resource_by_index(core_resource, device_index);
  if (!r) {
    return;
  }
  if (r->is_const) {
    OC_ERR("resource is const");
    return;
  }

  oc_resource_bind_dpt((oc_resource_t *)r, dpt);
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

const oc_resource_t *
oc_core_get_resource_by_index(int type, size_t device)
{
#ifndef OC_DYNAMIC_ALLOCATION
  if (type == OC_DEV_SN) {
    return &core_resources[0];
  }
  return &core_resources[WELLKNOWNCORE * device + type];
#else
  if (type == OC_DEV_SN) {
    return oc_list_head(core_resource_list);
  }
  if (device != 0) {
    return &core_resources[WELLKNOWNCORE * (device - 1) + type];
  }
  // traverse the list
  const oc_resource_t *res = oc_list_head(core_resource_list);
  while (type && res) {
    res = oc_list_item_next((void *)res);
    type--;
  }
  return res;
#endif
}

const oc_resource_t *
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

#endif /* OC_SECURITY */
  else {
    return NULL;
  }
  size_t res = WELLKNOWNCORE * device + type;
  return &core_resources[res];
}

int
oc_filter_resource_by_urn(const oc_resource_t *resource, oc_request_t *request)
{
  char *value = NULL;
  size_t value_len;
  char *key;
  size_t key_len;
  int truncate = 0;
  oc_init_query_iterator();
  while (oc_iterate_query(request, &key, &key_len, &value, &value_len) > 0) {
    if (strncmp(value, "urn:knx", 7) == 0) {
      truncate = 1;
      break;
    }
  }
  return truncate;
}

bool
oc_filter_resource_by_rt(const oc_resource_t *resource, oc_request_t *request)
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
        rt_len = (int)(wildcart - rt);
      }

      match = false;
      int i;
      for (i = 0; i < (int)oc_string_array_get_allocated_size(resource->types);
           i++) {
        size_t resource_type_len =
          oc_string_array_get_item_size(resource->types, i);
        const char *resource_type =
          (const char *)oc_string_array_get_item(resource->types, i);
        PRINT("   oc_filter_resource_by_rt '%.*s'\n", (int)resource_type_len,
              resource_type);
        if (wildcart != NULL) {
          if (strncmp(rt, resource_type, rt_len) == 0) {
            return true;
          }
        }
        if (rt_len == (int)resource_type_len &&
            strncmp(rt, resource_type, rt_len) == 0) {
          return true;
        }
      }
    }
  } while (more_query_params);
  return match;
}

bool
oc_filter_resource_by_if(const oc_resource_t *resource, oc_request_t *request)
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
