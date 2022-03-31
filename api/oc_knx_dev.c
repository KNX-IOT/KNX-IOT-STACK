/*
 // Copyright (c) 2021-2022 Cascoda Ltd
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
#include "api/oc_knx_sec.h"
#include "api/oc_main.h"
#include "port/dns-sd.h"

#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>

#define KNX_STORAGE_IA "dev_knx_ia"
#define KNX_STORAGE_HOSTNAME "dev_knx_hostname"
#define KNX_STORAGE_IID "dev_knx_iid"
#define KNX_STORAGE_FID "dev_knx_fid"
#define KNX_STORAGE_PM "dev_knx_pm"
#define KNX_STORAGE_SA "dev_knx_sa"
#define KNX_STORAGE_DA "dev_knx_da"
#define KNX_STORAGE_PORT "dev_knx_port"

static void
oc_core_dev_sn_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
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

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_sn_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_sn_resource\n");
  // rt :dpa:0.11
  // rt :dpt.serNum
  oc_core_populate_resource(
    resource_idx, device, "/dev/sn", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_sn_get_handler, 0, 0, 0, 2, ":dpa:0.11", "dpt.serNum");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_hwv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
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
  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

void
oc_create_dev_hwv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hwv_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/hwv", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_dev_hwv_get_handler, 0, 0, 0, 1, ":dpt.version");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_fwv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
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

  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

void
oc_create_dev_fwv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_fwv_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/fwv", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_fwv_get_handler, 0, 0, 0, 2,
                            ":dpa.0.25", ":dpt.version");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_hwt_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
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
  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

void
oc_create_dev_hwt_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hwt_resource\n");
  // cbor rt :dpt.varString8859_1
  oc_core_populate_resource(resource_idx, device, "/dev/hwt", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_hwt_get_handler, 0, 0, 0, 1,
                            ":dpt.varString8859_1");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_model_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
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
  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_model_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_model_resource\n");
  oc_core_populate_resource(resource_idx, device, "/dev/model", OC_IF_D,
                            APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_model_get_handler, 0, 0, 0, 2,
                            ":dpa.0.15", ":dpt.utf8");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_ia_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    OC_ERR("invalid request");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 12, (int64_t)device->ia);
    oc_rep_i_set_int(root, 26, (int64_t)device->iid);
    if (device->fid > 0) {
      // only frame it when it is set...
      oc_rep_i_set_int(root, 25, (int64_t)device->fid);
    }
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }
  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_ia_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  bool error = false;
  bool ia_set = false;
  bool iid_set = false;
  bool fid_set = false;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 12) {
        PRINT("  oc_core_dev_ia_put_handler received 12 (ia) : %d\n",
              (int)rep->value.integer);
        oc_core_set_device_ia(device_index, (int)rep->value.integer);
        int temp = (int)rep->value.integer;
        oc_storage_write(KNX_STORAGE_IA, (uint8_t *)&temp, sizeof(temp));
        ia_set = true;
      } else if (rep->iname == 25) {
        PRINT("  oc_core_dev_ia_put_handler received 25 (fid): %d\n",
              (int)rep->value.integer);
        oc_core_set_device_fid(device_index, (int)rep->value.integer);
        int temp = (int)rep->value.integer;
        oc_storage_write(KNX_STORAGE_FID, (uint8_t *)&temp, sizeof(temp));
        fid_set = true;
      } else if (rep->iname == 26) {
        PRINT("  oc_core_dev_ia_put_handler received 26 (iid): %d\n",
              (int)rep->value.integer);
        oc_core_set_device_iid(device_index, (int)rep->value.integer);
        int temp = (int)rep->value.integer;
        oc_storage_write(KNX_STORAGE_IID, (uint8_t *)&temp, sizeof(temp));
        iid_set = true;
      }
    }
    rep = rep->next;
  }
  if (fid_set) {
    OC_ERR("fid set in request: returning error!");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  if (iid_set && ia_set) {
    // do the run time installation
    if (oc_is_device_in_runtime(device_index)) {
      oc_register_group_multicasts();
      oc_init_datapoints_at_initialization();
    }

    oc_send_cbor_response(request, OC_STATUS_CHANGED);
  } else {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
  }
}

void
oc_create_dev_ia_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_ia_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/ia", OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_ia_get_handler, oc_core_dev_ia_put_handler, 0, 0, 1,
    ":dpt.value2Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_hostname_put_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_STRING) {
      if (rep->iname == 1) {
        PRINT("  oc_core_dev_hostname_put_handler received : %s\n",
              oc_string(rep->value.string));
        oc_core_set_device_hostname(device_index, oc_string(rep->value.string));

        oc_storage_write(KNX_STORAGE_HOSTNAME,
                         (uint8_t *)oc_string(rep->value.string),
                         oc_string_len(rep->value.string));

        oc_hostname_t *my_hostname = oc_get_hostname_cb();
        if (my_hostname && my_hostname->cb) {
          my_hostname->cb(device_index, rep->value.string, my_hostname->data);
        }

        oc_send_cbor_response(request, OC_STATUS_OK);
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_hostname_get_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL && oc_string(device->hostname) != NULL) {
    // cbor_encode_text_stringz(&g_encoder, oc_string(device->hostname));
    oc_rep_begin_root_object();
    oc_rep_i_set_text_string(root, 1, oc_string(device->hostname));
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_dev_hostname_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hostname_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/hname", OC_IF_P, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_dev_hostname_get_handler,
    oc_core_dev_hostname_put_handler, 0, 0, 1, ":dpt.varString8859_1");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_iid_put_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_INT) {
      if (rep->iname == 1) {
        PRINT("  oc_core_dev_iid_put_handler received : %d\n",
              (int)rep->value.integer);
        oc_core_set_device_iid(device_index, (int)rep->value.integer);
        // make the value persistent
        oc_storage_write(KNX_STORAGE_IID, (uint8_t *)&rep->value.integer,
                         sizeof(uint32_t));
        oc_send_cbor_response(request, OC_STATUS_CHANGED);

        // do the run time installation
        if (oc_is_device_in_runtime(device_index)) {
          oc_register_group_multicasts();
          oc_init_datapoints_at_initialization();
          oc_device_info_t *device = oc_core_get_device_info(device_index);
          knx_publish_service(oc_string(device->serialnumber), device->iid, device->ia);
        }
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_iid_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // cbor_encode_int(&g_encoder, device->iid);
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->iid);
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_iid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_iid_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/iid", OC_IF_P, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_dev_iid_get_handler, oc_core_dev_iid_put_handler,
    0, 0, 1, ":dpt.value4Ucount ");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_ipv6_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  // frame only the first one...
  oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(device_index);
  if (my_ep != NULL) {
    cbor_encode_byte_string(&g_encoder, my_ep->addr.ipv6.address,
                            sizeof(my_ep->addr.ipv6.address));
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_ipv6_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_ipv6_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/ipv6", OC_IF_P, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_dev_ipv6_get_handler, 0, 0, 0, 1, ":dpt.ipv6");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_pm_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

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

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_pm_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  oc_core_dev_pm_put_handler received : %d\n",
              (int)rep->value.boolean);
        device->pm = rep->value.boolean;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);

        oc_storage_write(KNX_STORAGE_PM, (uint8_t *)&(rep->value.boolean), 1);
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_pm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_pm_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/pm", OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_pm_get_handler, oc_core_dev_pm_put_handler, 0, 0, 2,
    ":dpa.0.54", "dpa.binaryValue");
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
  PRINT("oc_core_dev_dev_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  for (i = (int)OC_DEV_SN; i < (int)OC_DEV; i++) {
    oc_resource_t *resource = oc_core_get_resource_by_index(i, device_index);
    if (oc_filter_resource(resource, request, device_index, &response_length,
                           matches)) {
      matches++;
    }
  }

  if (matches > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_dev_dev_get_handler - end\n");
}

void
oc_create_dev_dev_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_dev_resource\n");
  // note that this resource is listed in /.well-known/core so it should have
  // the full rt with urn:knx prefix
  oc_core_populate_resource(
    resource_idx, device, "/dev", OC_IF_LI, APPLICATION_LINK_FORMAT, 0,
    oc_core_dev_dev_get_handler, 0, 0, 0, 1, "urn:knx:fb.0");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_sa_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // cbor_encode_int(&g_encoder, device->sa);
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->sa);
    oc_rep_end_root_object();

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_sa_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  oc_rep_t *rep = request->request_payload;
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_dev_sa_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_core_dev_sa_put_handler received : %d\n",
          (int)rep->value.integer);
    device->sa = (int)rep->value.integer;
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    oc_storage_write(KNX_STORAGE_SA, (uint8_t *)&(rep->value.integer), 1);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_sa_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_sa_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/sa", OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_sa_get_handler, oc_core_dev_sa_put_handler, 0, 0, 2,
    ":dpa.0.57", ":dpt.value1Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_da_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // cbor_encode_int(&g_encoder, device->da);
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->da);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_da_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  oc_rep_t *rep = request->request_payload;
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_dev_da_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_core_dev_da_put_handler received : %d\n",
          (int)rep->value.integer);
    device->da = (uint32_t)rep->value.integer;
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    oc_storage_write(KNX_STORAGE_DA, (uint8_t *)&(rep->value.integer), 1);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_da_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_da_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/da", OC_IF_P, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_da_get_handler, oc_core_dev_da_put_handler, 0, 0, 2,
    ":dpa.0.58", ":dpt.value1Ucount");
}

// -----------------------------------------------------------------------------

static void
oc_core_dev_port_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    // cbor_encode_int(&g_encoder, device->port);
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 1, device->port);
    oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

static void
oc_core_dev_port_put_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  oc_rep_t *rep = request->request_payload;
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_dev_port_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_core_dev_port_put_handler received : %d\n",
          (int)rep->value.integer);
    device->port = (uint32_t)rep->value.integer;
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    oc_storage_write(KNX_STORAGE_PORT, (uint8_t *)&(rep->value.integer), 1);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_port_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_port_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/port", OC_IF_P, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_dev_port_get_handler, oc_core_dev_port_put_handler,
    0, 0, 1, ":dpt.value2Ucount");
}

// -----------------------------------------------------------------------------

void
oc_knx_device_storage_read(size_t device_index)
{

  uint64_t ia;
  int temp_size;
  char tempstring[20];
  bool pm;

  PRINT("Loading Device Config from Persistent storage\n");

  if (device_index >= oc_core_get_num_devices()) {
    PRINT("device_index %d to large\n", (int)device_index);
    return;
  }

  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    OC_ERR(" could not get device %d\n", (int)device_index);
  }

  /* IA */
  temp_size = oc_storage_read(KNX_STORAGE_IA, (uint8_t *)&ia, sizeof(ia));
  if (temp_size > 0) {
    device->ia = (int)ia;
    PRINT("  ia (storage) %ld\n", (long)ia);
  }

  /* HOST NAME */
  temp_size = oc_storage_read(KNX_STORAGE_HOSTNAME, (uint8_t *)&tempstring, 20);
  if (temp_size > 1) {
    tempstring[temp_size] = 0;
    oc_core_set_device_hostname(device_index, tempstring);
    PRINT("  hostname (storage) %s\n", oc_string(device->hostname));
  }

  /* KNX_STORAGE_IID */
  temp_size =
    oc_storage_read(KNX_STORAGE_IID, (uint8_t *)&device->iid, sizeof(uint32_t));
  PRINT("  idd (storage) %d\n", device->iid);

  /* KNX_STORAGE_PM */
  temp_size = oc_storage_read(KNX_STORAGE_PM, (uint8_t *)&pm, 1);
  if (temp_size > 0) {
    device->pm = pm;
    PRINT("  pm (storage) %d\n", device->pm);
  }

  /* KNX_STORAGE_SA */
  temp_size =
    oc_storage_read(KNX_STORAGE_SA, (uint8_t *)&device->sa, sizeof(uint32_t));
  PRINT("  sa (storage) %d\n", device->sa);

  /* KNX_STORAGE_DA */
  temp_size =
    oc_storage_read(KNX_STORAGE_DA, (uint8_t *)&device->da, sizeof(uint32_t));
  PRINT("  da (storage) %d\n", device->da);
}

void
oc_knx_device_storage_reset(size_t device_index, int reset_mode)
{
  (void)device_index;
  (void)reset_mode;

  char buf[2] = "";
  int zero = 0;

  if (device_index >= oc_core_get_num_devices()) {
    PRINT("oc_knx_device_storage_reset: device_index %d to large\n",
          (int)device_index);
    return;
  }
  if (reset_mode == 2) {
    /* With erase code �2� (�Factory Reset to default state�),
     all addressing information and security configuration data SHALL be reset
     to default ex-factory state. */
    // writing the empty values
    oc_storage_write(KNX_STORAGE_IA, (char *)&zero, sizeof(int));
    oc_storage_write(KNX_STORAGE_IID, (char *)&zero, sizeof(uint32_t));
    oc_storage_write(KNX_STORAGE_PM, (char *)&zero, sizeof(uint8_t));
    oc_storage_write(KNX_STORAGE_SA, (char *)&zero, sizeof(uint32_t));
    oc_storage_write(KNX_STORAGE_DA, (char *)&zero, sizeof(uint32_t));
    uint32_t port =
      5683; // TODO: should be using all coap nodes define from the stack
    oc_storage_write(KNX_STORAGE_PORT, (char *)&port, sizeof(uint32_t));
    oc_storage_write(KNX_STORAGE_HOSTNAME, (char *)&buf, 1);
    // load state: unloaded, and programming mode is true
    oc_knx_lsm_set_state(device_index, LSM_S_UNLOADED);
    oc_device_info_t *device = oc_core_get_device_info(device_index);
    // set the other data to null
    device->ia = zero;
    device->iid = zero;
    device->sa = zero;
    device->da = zero;
    device->port = port;
    oc_free_string(&device->hostname);
    oc_new_string(&device->hostname, "", strlen(""));

    oc_delete_group_object_table();
    oc_delete_group_rp_table();

    oc_delete_at_table(device_index);

  } else if (reset_mode == 7) {
    /*
    With erase code �7� (�Factory Reset to default without IA�),
      all configuration data SHALL be reset to ex -factory default state
      except addressing information( IA, Device IP Address) and
      security configuration data(credentials)
      that are needed after the reset to access the device without need
      to discover the device again and /
      or renew addressing information and security credentials.
        */

    oc_delete_group_object_table();
    oc_delete_group_rp_table();
    // load state: unloaded
    oc_knx_lsm_set_state(device_index, LSM_S_UNLOADED);
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
    PRINT("device_index %d to large\n", (int)device_index);
    return false;
  }

  oc_device_info_t *device = oc_core_get_device_info(device_index);
  return device->pm;
}

void
oc_create_knx_device_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_device_resources");

  oc_create_dev_sn_resource(OC_DEV_SN, device_index);
  oc_create_dev_hwv_resource(OC_DEV_HWV, device_index);
  oc_create_dev_fwv_resource(OC_DEV_FWV, device_index);
  oc_create_dev_hwt_resource(OC_DEV_HWT, device_index);
  oc_create_dev_model_resource(OC_DEV_MODEL, device_index);
  oc_create_dev_ia_resource(OC_DEV_IA, device_index);
  oc_create_dev_hostname_resource(OC_DEV_HOSTNAME, device_index);
  oc_create_dev_iid_resource(OC_DEV_IID, device_index);
  oc_create_dev_pm_resource(OC_DEV_PM, device_index);
  oc_create_dev_ipv6_resource(OC_DEV_IPV6, device_index);
  oc_create_dev_sa_resource(OC_DEV_SA, device_index);
  oc_create_dev_da_resource(OC_DEV_DA, device_index);
  oc_create_dev_port_resource(OC_DEV_PORT, device_index);
  // should be last of the dev/xxx resources, it will list those.
  oc_create_dev_dev_resource(OC_DEV, device_index);
}
