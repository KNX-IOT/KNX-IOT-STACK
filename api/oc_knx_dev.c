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
#include "api/oc_knx_dev.h"
#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>

static void
oc_core_dev_sn_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    //cbor_encode_int(&encoder, some_value);

    // 8 is the size of a serial number.
    cbor_encode_text_string(&g_encoder, oc_string(device->serialnumber), 8);

    //cbor_encode_text_stringz(&g_encoder, oc_string(device->serialnumber));

    //oc_rep_set_text_string_no_tag(oc_string(device->serialnumber));)
    //oc_rep_begin_root_object();
    //ock_rep_set_text_string(root, oc_string(device->serialnumber));
    //oc_rep_end_root_object();
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_OK);
}


static void
oc_core_dev_sn_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

void
oc_create_dev_sn_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_sn_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/dev/sn", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_sn_get_handler, oc_core_dev_sn_put_handler, 0, 0, 1,
    "urn:knx:dpt.a[n]");
}

static void
oc_core_dev_hwv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {

    // 8 is the size of a serial number.
    cbor_encode_int(&g_encoder, (int64_t)device->hwt.major);
    cbor_encode_int(&g_encoder, (int64_t)device->hwt.minor);
    cbor_encode_int(&g_encoder, (int64_t)device->hwt.third);

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

void
oc_create_dev_hwv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hwv_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/dev/hwv", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_hwv_get_handler, 0, 0, 0, 1, "urn:knx:dpt.version");
}

static void
oc_core_dev_fwv_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    cbor_encode_int(&g_encoder, (int64_t)device->fwv.major);
    cbor_encode_int(&g_encoder, (int64_t)device->fwv.minor);
    cbor_encode_int(&g_encoder, (int64_t)device->fwv.third);

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

void
oc_create_dev_fwv_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_fwv_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/dev/fwv", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_fwv_get_handler, 0, 0, 0, 1, "urn:knx:dpt.version");
}

static void
oc_core_dev_hwt_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device != NULL) {
    cbor_encode_int(&g_encoder, (int64_t)device->hwt.major);
    cbor_encode_int(&g_encoder, (int64_t)device->hwt.minor);
    cbor_encode_int(&g_encoder, (int64_t)device->hwt.third);

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
}

void
oc_create_dev_hwt_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hwt_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/dev/hwt", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_hwt_get_handler, 0, 0, 0, 1, "urn:knx:dpt.Version");
}

static void
oc_core_dev_macaddr_get_handler(oc_request_t *request,
                                oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_dev_macaddr_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_macaddr_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/dev/macaddr", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_macaddr_get_handler, 0, 0, 0, 1, "urn:knx:dpt.varString8859_1");
}

static void
oc_core_dev_name_get_handler(oc_request_t *request,
                             oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_dev_name_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_name_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/dev/name", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_name_get_handler, 0, 0, 0, 1, "urn:knx:dpt.a[n]");
}

static void
oc_core_dev_model_get_handler(oc_request_t *request,
                              oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_dev_model_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_model_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/dev/model", OC_IF_D,
                               APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_dev_model_get_handler, 0, 0, 0, 1, "urn:knx:dpt.a[n]");
}

static void
oc_core_dev_ia_get_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

static void
oc_core_dev_ia_put_handler(oc_request_t *request,
                           oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_dev_ia_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_ia_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/dev/ia", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
                            oc_core_dev_ia_get_handler,
                            oc_core_dev_ia_put_handler, 0, 0, 1, "urn:knx:ia");
}

static void
oc_core_dev_hostname_put_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

static void
oc_core_dev_hostname_get_handler(oc_request_t *request,
                                 oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_dev_hostname_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_hostname_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/dev/hostname", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE,
    oc_core_dev_hostname_get_handler, oc_core_dev_hostname_put_handler, 0, 0, 1,
    "urn:knx:dpt.a[n]");
}

static void
oc_core_dev_iid_put_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

static void
oc_core_dev_iid_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  // size_t response_length = 0;

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  oc_send_cbor_response(request, OC_STATUS_OK);
}

void
oc_create_dev_iid_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_iid_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/dev/iid", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE,
    oc_core_dev_iid_get_handler, oc_core_dev_iid_put_handler, 0, 0, 1,
    "urn:knx:dpt.a[5]");
}

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
  oc_core_lf_populate_resource(resource_idx, device, "/dev", OC_IF_D,
                               APPLICATION_LINK_FORMAT,
                            0, oc_core_dev_dev_get_handler, 0, 0, 0, 0, "");
}

void
oc_create_knx_device_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_device_resources");

  oc_create_dev_sn_resource(OC_DEV_SN, device_index);
  oc_create_dev_hwv_resource(OC_DEV_HWV, device_index);
  oc_create_dev_fwv_resource(OC_DEV_FWV, device_index);
  oc_create_dev_hwt_resource(OC_DEV_HWT, device_index);
  oc_create_dev_macaddr_resource(OC_DEV_MACADDRESS, device_index);
  oc_create_dev_name_resource(OC_DEV_NAME, device_index);
  oc_create_dev_model_resource(OC_DEV_MODEL, device_index);
  oc_create_dev_ia_resource(OC_DEV_IA, device_index);
  oc_create_dev_hostname_resource(OC_DEV_HOSTNAME, device_index);
  oc_create_dev_iid_resource(OC_DEV_IID, device_index);
  // should be last of the dev/xxx resources, it will list those.
  oc_create_dev_dev_resource(OC_DEV, device_index);
}
