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
#include "api/oc_knx_dev.h"
#include "api/oc_knx_fp.h"
#include "api/oc_knx_gm.h"
#include "api/oc_knx_sec.h"
#include "api/oc_knx_helpers.h"
#include "api/oc_main.h"
#include "port/dns-sd.h"

#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define KNX_STORAGE_HOSTNAME "dev_knx_hostname"
#define KNX_STORAGE_PM "dev_knx_pm"
#define KNX_STORAGE_PORT "dev_knx_port"
#define KNX_STORAGE_MPORT "dev_knx_mport"
#define KNX_STORAGE_AP_MAJOR "knx_ap_maj"
#define KNX_STORAGE_AP_MINOR "knx_ap_min"
#define KNX_STORAGE_AP_PATCH "knx_ap_p"

static void
oc_core_dev_sn_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // Content-Format: "application/cbor"
    // Payload: "123ABC"
    oc_rep_begin_root_object();
    oc_rep_i_set_text_string(root, 1, oc_string(device->serialnumber));
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_sn, dev_hwv, 0, "/dev/sn", OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_sn_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.serNum", OC_SIZE_MANY(1),
                                     "urn:knx:dpa:0.11");

void
oc_create_dev_sn_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_sn_resource\n");
  // rt :dpa:0.11
  // rt :dpt.serNum
  oc_core_populate_resource(
    resource_idx, device, "/dev/sn", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_sn_get_handler, 0, 0, 0, 1, "urn:knx:dpa:0.11");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.serNum");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_hwv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }
  PRINT("oc_core_dev_hwv_get_handler\n");

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // Content-Format: "application/cbor"
    // Payload: [ 1, 2, 3 ]
    uint64_t array[3];
    array[0] = device->hwv.major;
    array[1] = device->hwv.minor;
    array[2] = device->hwv.patch;
    oc_rep_begin_root_object();
    oc_rep_i_set_int_array(root, 1, array, 3);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }
  oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_hwv, dev_fwv, 0, "/dev/hwv", OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_hwv_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.version", OC_SIZE_ZERO());

void
oc_create_dev_hwv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hwv_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/hwv", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_hwv_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.version");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_fwv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  PRINT("oc_core_dev_fwv_get_handler\n");

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // Content-Format: "application/cbor"
    // Payload: [ 1, 2, 3 ]
    uint64_t array[3];
    array[0] = device->fwv.major;
    array[1] = device->fwv.minor;
    array[2] = device->fwv.patch;
    oc_rep_begin_root_object();
    oc_rep_i_set_int_array(root, 1, array, 3);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_fwv, dev_hwt, 0, "/dev/fwv", OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_fwv_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.version", OC_SIZE_MANY(1),
                                     "urn:knx:dpa.0.25");

void
oc_create_dev_fwv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_fwv_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/fwv", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_fwv_get_handler, 0, 0, 0, 1,
                            "urn:knx:dpa.0.25");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.version");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_hwt_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL && oc_string(device->hwt) != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_text_string(root, 1, oc_string(device->hwt));
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }
  oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_hwt, dev_model, 0, "/dev/hwt", OC_IF_D,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_hwt_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.varString8859_1",
                                     OC_SIZE_ZERO());

void
oc_create_dev_hwt_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hwt_resource\n");
  // cbor rt :dpt.varString8859_1
  oc_core_populate_resource(resource_idx, device, "/dev/hwt", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_hwt_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device,
                            "urn:knx:dpt.varString8859_1");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_model_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL && oc_string(device->model) != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_text_string(root, 1, oc_string(device->model));
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }
  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_model, dev_hostname, 0, "/dev/model",
                                     OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_model_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.utf8", OC_SIZE_MANY(1),
                                     "urn:knx:dpa.0.15");

void
oc_create_dev_model_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_model_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/model", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_model_get_handler, 0, 0, 0, 1,
                            "urn:knx:dpa.0.15");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.utf8");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_hostname_put_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_STRING) {
      if (rep->iname == 1) {
        PRINT("  oc_core_dev_hostname_put_handler received : %s\n",
              oc_string_checked(rep->value.string));
        oc_core_set_device_hostname(device_index, oc_string(rep->value.string));

        oc_storage_write(KNX_STORAGE_HOSTNAME,
                         (uint8_t *)oc_string(rep->value.string),
                         oc_string_len(rep->value.string));

        oc_hostname_t *my_hostname = oc_get_hostname_cb();
        if (my_hostname && my_hostname->cb) {
          my_hostname->cb(device_index, rep->value.string, my_hostname->data);
        }

        oc_send_response_no_format(request, OC_STATUS_CHANGED);
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_hostname_get_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_text_string(root, 1, oc_string(device->hostname));
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }
  oc_send_cbor_response(request, OC_STATUS_OK);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_hostname, dev_iid, 0, "/dev/hname",
                                     OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_hostname_get_handler,
                                     oc_core_dev_hostname_put_handler, 0, 0,
                                     "urn:knx:dpt.varString8859_1",
                                     OC_SIZE_ZERO());

void
oc_create_dev_hostname_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hostname_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/hname", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_hostname_get_handler,
                            oc_core_dev_hostname_put_handler, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device,
                            "urn:knx:dpt.varString8859_1");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_iid_put_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_dev_iid_put_handler received : %" PRIu64 "\n",
              rep->value.integer);
        oc_core_set_device_iid(device_index, rep->value.integer);
        // make the value persistent
        oc_storage_write(KNX_STORAGE_IID, (uint8_t *)&rep->value.integer,
                         sizeof(uint64_t));
        oc_send_response_no_format(request, OC_STATUS_CHANGED);

        // do the run time installation
        if (oc_is_device_in_runtime(device_index)) {
          oc_register_group_multicasts();
          oc_init_datapoints_at_initialization();
          oc_device_info_t *device = oc_core_get_device_info(device_index);
          knx_publish_service(oc_string(device->serialnumber), device->iid,
                              device->ia, device->pm);
        }
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_iid_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->iid);
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_iid, dev_pm, 0, "/dev/iid", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_iid_get_handler,
                                     oc_core_dev_iid_put_handler, 0, 0,
                                     "urn:knx:dpt.value8Ucount",
                                     OC_SIZE_ZERO());

void
oc_create_dev_iid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_iid_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/iid", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_iid_get_handler,
                            oc_core_dev_iid_put_handler, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value8Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_ipv6_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  int i;

  int ps = 1;
  bool ps_exists = false;
  bool total_exists = false;
  int total = 0;
  int first_entry = 0; // inclusive
  int last_entry = 0;  // exclusive
  // int query_ps = -1;
  int query_pn = -1;

  PRINT("oc_core_dev_ipv6_get_handler\n");

  // get the device
  size_t device_index = request->resource->device;

  oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(device_index);
  // Calculate total endpoints
  while (my_ep != NULL) {
    my_ep = my_ep->next;
    total++;
  }
  last_entry = total;

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /dev/ipv6 > l = total>;total=22;ps=5
    size_t response_length = oc_frame_query_l(
      oc_string(request->resource->uri), ps_exists, ps, total_exists, total);
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
    return;
  }
  if (l_exist == -1) {
    oc_send_response_no_format(request, OC_STATUS_NOT_FOUND);
    return;
  }

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  my_ep = oc_connectivity_get_endpoints(device_index);
  // handle query with page number (pn)
  if (check_if_query_pn_exist(request, &query_pn, NULL)) {
    first_entry = query_pn * ps;
    if (first_entry >= last_entry) {
      oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
      return;
    }

    // skip endpoints and return the next one
    for (i = 0; i < first_entry; i++) {
      my_ep = my_ep->next;
    }
  }

  // return the single entry.
  oc_rep_begin_root_object();
  oc_rep_i_set_byte_string(root, 1, my_ep->addr.ipv6.address,
                           sizeof(my_ep->addr.ipv6.address));
  oc_rep_end_root_object();

  oc_send_cbor_response(request, OC_STATUS_OK);

  PRINT("oc_core_dev_ipv6_get_handler - end\n");
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_ipv6, dev_sa, 0, "/dev/ipv6", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_ipv6_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.ipv6", OC_SIZE_ZERO());

void
oc_create_dev_ipv6_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_ipv6_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/ipv6", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_ipv6_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.ipv6");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_pm_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  PRINT("  oc_core_dev_pm_get_handler\n");
  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // cbor_encode_boolean(&g_encoder, device->pm);
    oc_rep_begin_root_object();
    oc_rep_i_set_boolean(root, 1, device->pm);
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_pm_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  oc_rep_t *rep = request->request_payload;
  oc_programming_mode_t *my_cb = oc_get_programming_mode_cb();

  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  oc_core_dev_pm_put_handler received : %d\n",
              (int)rep->value.boolean);

        // NOTE: It is the responsibility of the callback (if it exists)
        // to set the programming mode, in this case
        if (my_cb && my_cb->cb)
          my_cb->cb(device_index, rep->value.boolean, my_cb->data);
        else
          device->pm = rep->value.boolean;

        oc_send_response_no_format(request, OC_STATUS_CHANGED);

        knx_publish_service(oc_string(device->serialnumber), device->iid,
                            device->ia, device->pm);
        oc_storage_write(KNX_STORAGE_PM, (uint8_t *)&(rep->value.boolean), 1);
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_pm, dev_ipv6, 0, "/dev/pm", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_pm_get_handler,
                                     oc_core_dev_pm_put_handler, 0, 0,
                                     "urn:knx:dpt.binaryValue", OC_SIZE_MANY(1),
                                     "urn:knx:dpa.0.54");

void
oc_create_dev_pm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_pm_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/pm", OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_pm_get_handler, oc_core_dev_pm_put_handler, 0, 0, 1,
    "urn:knx:dpa.0.54");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.binaryValue");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_dev_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int matches = 0;

  bool ps_exists = false;
  bool total_exists = false;
  int total = (int)OC_DEV - (int)OC_DEV_SN;
  int first_entry = (int)OC_DEV_SN; // inclusive
  int last_entry = (int)OC_DEV;     // exclusive
  // int query_ps = -1;
  int query_pn = -1;
  bool more_request_needed =
    false; // If more requests (pages) are needed to get the full list

  PRINT("oc_core_dev_dev_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /dev > l = total>;total=22;ps=5
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

  for (i = first_entry; i < last_entry; i++) {
    const oc_resource_t *resource =
      oc_core_get_resource_by_index(i, device_index);
    if (oc_filter_resource(resource, request, device_index, &response_length,
                           &i, i)) {
      matches++;
    }
  }

  if (matches > 0) {
    if (more_request_needed) {
      int next_page_num = query_pn > -1 ? query_pn + 1 : 1;
      response_length += add_next_page_indicator(
        oc_string(request->resource->uri), next_page_num);
    }
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  }

  PRINT("oc_core_dev_dev_get_handler - end\n");
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev, app, 0, "/dev", OC_IF_LI,
                                     APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                                     oc_core_dev_dev_get_handler, 0, 0, 0, NULL,
                                     OC_SIZE_MANY(1), "urn:knx:fb.0");

void
oc_create_dev_dev_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_dev_resource\n");
  // note that this resource is listed in /.well-known/core so it should have
  // the full rt with urn:knx prefix
  oc_core_populate_resource(
    resource_idx, device, "/dev", OC_IF_LI, APPLICATION_LINK_FORMAT,
    OC_DISCOVERABLE, oc_core_dev_dev_get_handler, 0, 0, 0, 1, "urn:knx:fb.0");
}

// -----------------------------------------------------------------------------
/*
  Each KNX IoT device, i.e.a Router or an end device,
  SHALL have a unique KNX Individual Address in a network.The KNX Individual
  Address is a 2 octet value that consist of an 8 bit Subnetwork
  Address(see �dev / sa�) and
  an 8 bit Device Address(see �dev / da�)
  octet 0 == subnetwork
  octet 1 == device address
  Example: Subnetwork Address: 0, Device Address: 1 = 0x0001
*/

static void
oc_core_dev_sa_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    uint8_t sa = (uint8_t)((device->ia) >> 8);
    oc_rep_i_set_int(root, 1, sa);
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_sa, dev_da, 0, "/dev/sna", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_sa_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.value1Ucount",
                                     OC_SIZE_MANY(1), "urn:knx:dpa.0.57");

void
oc_create_dev_sa_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_sa_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/sna", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_sa_get_handler, 0, 0, 0, 1,
                            "urn:knx:dpa.0.57");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value1Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_da_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();

    uint8_t da = (uint8_t)(device->ia);
    oc_rep_i_set_int(root, 1, da);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_da, dev_fid, 0, "/dev/da", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_da_get_handler, 0, 0, 0,
                                     "urn:knx:dpa.0.58", OC_SIZE_MANY(1),
                                     "urn:knx:dpa.0.58");

void
oc_create_dev_da_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_da_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/da", OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_da_get_handler, 0, 0, 0, 1, "urn:knx:dpa.0.58");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value1Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_fid_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->fid);
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_fid_put_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_dev_fid_put_handler received : %" PRIu64 "\n",
              rep->value.integer);
        oc_core_set_device_fid(device_index, (uint64_t)rep->value.integer);
        uint64_t temp = (uint64_t)rep->value.integer;
        oc_storage_write(KNX_STORAGE_FID, (uint8_t *)&temp, sizeof(temp));
        oc_send_response_no_format(request, OC_STATUS_CHANGED);
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_fid, dev_port, 0, "/dev/fid", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_fid_get_handler,
                                     oc_core_dev_fid_put_handler, 0, 0,
                                     "urn:knx:dpt.value8Ucount",
                                     OC_SIZE_ZERO());

void
oc_create_dev_fid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_fid_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/fid", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_fid_get_handler,
                            oc_core_dev_fid_put_handler, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value8Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_port_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->port);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_port, dev_mport, 0, "/dev/port",
                                     OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_port_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.value2Ucount",
                                     OC_SIZE_ZERO());

void
oc_create_dev_port_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_port_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/port", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_port_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value2Ucount");
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

static void
oc_core_dev_mport_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->mport);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_mport_put_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  oc_rep_t *rep = request->request_payload;
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_dev_mport_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_core_dev_mport_put_handler received : %d\n",
          (int)rep->value.integer);
    device->mport = (uint32_t)rep->value.integer;
    oc_send_response_no_format(request, OC_STATUS_CHANGED);
    oc_storage_write(KNX_STORAGE_MPORT, (uint8_t *)&(rep->value.integer), 1);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_mport, dev_mid, 0, "/dev/mport",
                                     OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_mport_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.value2Ucount",
                                     OC_SIZE_ZERO());
void
oc_create_dev_mport_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_mport_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/mport", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_mport_get_handler, 0, 0, 0, 0);

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value2Ucount");
}

// -----------------------------------------------------------------------------
static int
oc_core_dump_ap(int device_index)
{
  // KNX_STORAGE_AP
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    int32_t value = device->ap.major;
    oc_storage_write(KNX_STORAGE_AP_MAJOR, (uint8_t *)&value, sizeof(value));
    value = device->ap.minor;
    oc_storage_write(KNX_STORAGE_AP_MINOR, (uint8_t *)&value, sizeof(value));
    value = device->ap.patch;
    oc_storage_write(KNX_STORAGE_AP_PATCH, (uint8_t *)&value, sizeof(value));
    return 0;
  }
  return -1;
}

static int
oc_core_read_ap(int device_index)
{
  // KNX_STORAGE_AP
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    int32_t value;
    long temp_size;

    temp_size =
      oc_storage_read(KNX_STORAGE_AP_MAJOR, (uint8_t *)&value, sizeof(value));
    if (temp_size > 0) {
      device->ap.major = value;
    }
    temp_size =
      oc_storage_read(KNX_STORAGE_AP_MINOR, (uint8_t *)&value, sizeof(value));
    if (temp_size > 0) {
      device->ap.minor = value;
    }
    temp_size =
      oc_storage_read(KNX_STORAGE_AP_PATCH, (uint8_t *)&value, sizeof(value));
    if (temp_size > 0) {
      device->ap.patch = value;
    }
    return 0;
  }
  return -1;
}

static void
oc_core_ap_x_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // Content-Format: "application/cbor"
    // Payload: [ 1, 2, 3 ]
    uint64_t array[3];
    array[0] = device->ap.major;
    array[1] = device->ap.minor;
    array[2] = device->ap.patch;
    oc_rep_begin_root_object();
    oc_rep_i_set_int_array(root, 1, array, 3);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_ap_x_put_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  oc_rep_t *rep = request->request_payload;
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_ap_x_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_INT_ARRAY)) {
    int64_t *arr = oc_int_array(rep->value.array);
    int array_size = (int)oc_int_array_size(rep->value.array);
    if (array_size != 3) {
      oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
      return;
    }
    device->ap.major = (int)arr[0];
    device->ap.minor = (int)arr[1];
    device->ap.patch = (int)arr[2];

    // write to persistent storage
    oc_core_dump_ap(device_index);

    oc_send_response_no_format(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(app_x, a_lsm, 0, "/ap/pv", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_ap_x_get_handler,
                                     oc_core_ap_x_put_handler, 0, 0,
                                     "urn:knx:dpt.programVersion",
                                     OC_SIZE_MANY(1), "urn:knx:dpa.3.13");

void
oc_create_ap_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_ap_x_resource\n");
  oc_core_populate_resource(resource_idx, device, "/ap/pv", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_ap_x_get_handler, oc_core_ap_x_put_handler,
                            0, 0, 1, "urn:knx:dpa.3.13");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.programVersion");
}
// -----------------------------------------------------------------------------

static void
oc_core_ap_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                       void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int matches = 0;

  bool ps_exists = false;
  bool total_exists = false;
  int total = (int)OC_KNX_SPAKE - (int)OC_APP_X;
  int first_entry = (int)OC_APP_X;    // inclusive
  int last_entry = (int)OC_KNX_SPAKE; // exclusive
  // int query_ps = -1;
  int query_pn = -1;
  bool more_request_needed =
    false; // If more requests (pages) are needed to get the full list

  PRINT("oc_core_ap_get_handler\n");

  /* check if the accept header is link-format */
  if (oc_check_accept_header(request, APPLICATION_LINK_FORMAT) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  // handle query parameters: l=ps l=total
  int l_exist = check_if_query_l_exist(request, &ps_exists, &total_exists);
  if (l_exist == 1) {
    // example : < /ap > l = total>;total=22;ps=5
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

  for (i = first_entry; i < last_entry; i++) {
    const oc_resource_t *resource =
      oc_core_get_resource_by_index(i, device_index);
    if (oc_filter_resource(resource, request, device_index, &response_length,
                           &i, i)) {
      matches++;
    }
  }

  if (matches > 0) {
    if (more_request_needed) {
      int next_page_num = query_pn > -1 ? query_pn + 1 : 1;
      response_length += add_next_page_indicator(
        oc_string(request->resource->uri), next_page_num);
    }
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_response_no_format(request, OC_STATUS_INTERNAL_SERVER_ERROR);
  }

  PRINT("oc_core_ap_get_handler - end\n");
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(app, app_x, 0, "/ap", OC_IF_P,
                                     APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                                     oc_core_ap_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.value2Ucount",
                                     OC_SIZE_MANY(1), "urn:knx:fb.3");

void
oc_create_ap_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_ap_resource\n");
  oc_core_populate_resource(resource_idx, device, "/ap", OC_IF_P,
                            APPLICATION_LINK_FORMAT, OC_DISCOVERABLE,
                            oc_core_ap_get_handler, 0, 0, 0, 1, "urn:knx:fb.3");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value2Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_mid_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->mid);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
}

OC_CORE_CREATE_CONST_RESOURCE_LINKED(dev_mid, dev, 0, "/dev/mid", OC_IF_P,
                                     APPLICATION_CBOR, OC_DISCOVERABLE,
                                     oc_core_dev_mid_get_handler, 0, 0, 0,
                                     "urn:knx:dpt.value2Ucount",
                                     OC_SIZE_MANY(1), "urn:knx:dpa.0.12");
void
oc_create_dev_mid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_mid_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/mid", OC_IF_P,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_mid_get_handler, 0, 0, 0, 1,
                            "urn:knx:dpa.0.12");

  oc_core_bind_dpt_resource(resource_idx, device, "urn:knx:dpt.value2Ucount");
}

// -----------------------------------------------------------------------------
void
oc_knx_device_storage_read(size_t device_index)
{

  uint32_t ia;
  int temp_size;
  char tempstring[255];
  bool pm;

  PRINT("Loading Device Config from Persistent storage\n");

  if (device_index >= oc_core_get_num_devices()) {
    PRINT("device_index %d too large\n", (int)device_index);
    return;
  }

  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR(" could not get device %d\n", (int)device_index);
  }

  /* IA */
  temp_size = oc_storage_read(KNX_STORAGE_IA, (uint8_t *)&ia, sizeof(ia));
  if (temp_size > 0) {
    device->ia = ia;
    PRINT("  ia (storage) %d\n", ia);
  }

  /* HOST NAME */
  temp_size =
    oc_storage_read(KNX_STORAGE_HOSTNAME, (uint8_t *)&tempstring, 255);
  if (temp_size > 1) {
    tempstring[temp_size] = 0;
    oc_core_set_device_hostname(device_index, tempstring);
    PRINT("  hostname (storage) %s\n", oc_string_checked(device->hostname));
  }

  /* KNX_STORAGE_IID */
  temp_size =
    oc_storage_read(KNX_STORAGE_IID, (uint8_t *)&device->iid, sizeof(int64_t));
  PRINT("  idd (storage) %" PRIu64 "\n", device->iid);

  /* KNX_STORAGE_PM */
  temp_size = oc_storage_read(KNX_STORAGE_PM, (uint8_t *)&pm, 1);
  if (temp_size > 0) {
    device->pm = pm;
    PRINT("  pm (storage) %d\n", device->pm);
  }

  oc_core_read_ap(device_index);
}

void
oc_knx_device_storage_reset(size_t device_index, int reset_mode)
{
  (void)device_index;
  (void)reset_mode;

  oc_factory_presets_t *presets = oc_get_factory_presets_cb();
  if (presets && presets->cb) {
    presets->cb(0, presets->data);
  }

  char buf[2] = "";
  int zero = 0;
  uint32_t ffff = 0xffff;

  if (device_index >= oc_core_get_num_devices()) {
    PRINT("oc_knx_device_storage_reset: device_index %d too large\n",
          (int)device_index);
    return;
  }

  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR("oc_knx_device_storage_reset: device is NULL\n");
    return;
  }

  if (reset_mode == 2) {
    /* With erase code 2 (Factory Reset to default state),
     all addressing information and security configuration data SHALL be reset
     to default ex-factory state. */
    // writing the empty values
    oc_storage_erase(KNX_STORAGE_IA);
    oc_storage_erase(KNX_STORAGE_IID);
    oc_storage_erase(KNX_STORAGE_PM);
    uint32_t port = 5683;  // unicast communication
    uint32_t mport = 5683; // multicast communication
    oc_storage_write(KNX_STORAGE_PORT, (char *)&port, sizeof(uint32_t));
    oc_storage_write(KNX_STORAGE_MPORT, (char *)&mport, sizeof(uint32_t));
    oc_storage_erase(KNX_STORAGE_HOSTNAME);
    // load state: unloaded, and programming mode is true
    oc_a_lsm_set_state(device_index, LSM_S_UNLOADED);
    // set the other data to KNX defaults
    device->ia = ffff;
    device->iid = zero;
    device->port = port;
    device->mport = mport;
    oc_free_string(&device->hostname);
    oc_new_string(&device->hostname, "", strlen(""));

    oc_delete_group_object_table();
    oc_delete_group_rp_table();
    oc_delete_group_mapping_table();
    oc_delete_at_table(device_index);
    oc_knx_device_set_programming_mode(device_index, false);

  } else if (reset_mode == 3) {
    /*  ResetIA The IA shall be reset to the medium
      specific default IA when this A_Restart is executed.Channel Number
      : Fixed : 00h */
    oc_storage_erase(KNX_STORAGE_IA);
    // set the ia to KNX defaults
    device->ia = ffff;
    // not sure if the programming mode needs to be reset
    oc_knx_device_set_programming_mode(device_index, false);
    oc_device_info_t *device = oc_core_get_device_info(device_index);
    device->ia = ffff;

  } else if (reset_mode == 7) {
    /*
      With erase code 7 (Factory Reset to default without IA),
      all configuration data SHALL be reset to ex -factory default state
      except addressing information( IA, Device IP Address) and
      security configuration data(credentials)
      that are needed after the reset to access the device without need
      to discover the device again and /
      or renew addressing information and security credentials.
        */
    oc_delete_group_object_table();
    oc_delete_group_rp_table();
    oc_delete_group_mapping_table();
    oc_reset_at_table(device_index, reset_mode);

    oc_knx_device_set_programming_mode(device_index, false);
    // load state: unloaded
    oc_a_lsm_set_state(device_index, LSM_S_UNLOADED);
  }

  oc_reset_t *my_reset_cb = oc_get_reset_cb();
  if (my_reset_cb && my_reset_cb->cb) {
    my_reset_cb->cb(device_index, reset_mode, my_reset_cb->data);
  }
}

bool
oc_knx_device_in_programming_mode(size_t device_index)
{

  if (device_index >= oc_core_get_num_devices()) {
    PRINT("device_index %d too large\n", (int)device_index);
    return false;
  }

  oc_device_info_t *device = oc_core_get_device_info(device_index);
  return device->pm;
}

void
oc_knx_device_set_programming_mode(size_t device_index, bool programming_mode)
{

  if (device_index >= oc_core_get_num_devices()) {
    PRINT("device_index %d too large\n", (int)device_index);
    return;
  }

  oc_device_info_t *device = oc_core_get_device_info(device_index);
  device->pm = programming_mode;
}

void
oc_create_knx_device_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_device_resources");

  if (device_index == 0) {
    OC_DBG("resources for dev 0 created statically");
    return;
  }

  oc_create_dev_sn_resource(OC_DEV_SN, device_index);
  oc_create_dev_hwv_resource(OC_DEV_HWV, device_index);
  oc_create_dev_fwv_resource(OC_DEV_FWV, device_index);
  oc_create_dev_hwt_resource(OC_DEV_HWT, device_index);
  oc_create_dev_model_resource(OC_DEV_MODEL, device_index);
  oc_create_dev_hostname_resource(OC_DEV_HOSTNAME, device_index);
  oc_create_dev_iid_resource(OC_DEV_IID, device_index);
  oc_create_dev_pm_resource(OC_DEV_PM, device_index);
  oc_create_dev_ipv6_resource(OC_DEV_IPV6, device_index);
  oc_create_dev_sa_resource(OC_DEV_SA, device_index);
  oc_create_dev_da_resource(OC_DEV_DA, device_index);
  oc_create_dev_fid_resource(OC_DEV_FID, device_index);
  oc_create_dev_port_resource(OC_DEV_PORT, device_index);
  oc_create_dev_mport_resource(OC_DEV_MPORT, device_index);
  oc_create_dev_mid_resource(OC_DEV_MID, device_index);
  oc_create_ap_resource(OC_APP, device_index);
  oc_create_ap_x_resource(OC_APP_X, device_index);
  // should be last of the dev/xxx resources, it will list those.
  oc_create_dev_dev_resource(OC_DEV, device_index);
}
