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
#include "api/oc_knx_fp.h"
#include "api/oc_knx_sec.h"

#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>

#define KNX_STORAGE_IA "dev_knx_ia"
#define KNX_STORAGE_HOSTNAME "dev_knx_hostname"
#define KNX_STORAGE_IID "dev_knx_iid"
#define KNX_STORAGE_PM "dev_knx_pm"

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
    cbor_encode_text_stringz(&g_encoder, oc_string(device->serialnumber));
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_sn_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_sn_resource\n");
  // cbor rt :dpa:0.11
  // json rt :dpt.serNum
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
    CborEncoder arrayEncoder;
    cbor_encoder_create_array(&g_encoder, &arrayEncoder, 3);
    cbor_encode_int(&arrayEncoder, (int64_t)device->hwv.major);
    cbor_encode_int(&arrayEncoder, (int64_t)device->hwv.minor);
    cbor_encode_int(&arrayEncoder, (int64_t)device->hwv.third);
    cbor_encoder_close_container_checked(&g_encoder, &arrayEncoder);

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

    // oc_rep_add_int(&arrayEncoder, (int64_t)device->fwv.major);

    // oc_rep_close_array(root, fibonacci);

    CborEncoder arrayEncoder;

    cbor_encoder_create_array(&g_encoder, &arrayEncoder, 3);
    cbor_encode_int(&arrayEncoder, (int64_t)device->fwv.major);
    cbor_encode_int(&arrayEncoder, (int64_t)device->fwv.minor);
    cbor_encode_int(&arrayEncoder, (int64_t)device->fwv.third);
    cbor_encoder_close_container_checked(&g_encoder, &arrayEncoder);

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
                            ":dpa.0.54", ":dpt.version");
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
    cbor_encode_text_stringz(&g_encoder, oc_string(device->hwt));
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

void
oc_core_dev_name_get_handler(oc_request_t *request,
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
  if (device != NULL && oc_string(device->name) != NULL) {
    cbor_encode_text_stringz(&g_encoder, oc_string(device->name));

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_name_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_name_resource\n");
  // cbor rt:
  oc_core_populate_resource(
    resource_idx, device, "/dev/name", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_dev_name_get_handler, 0, 0, 0, 1, ":dpt.utf8");
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
    cbor_encode_text_stringz(&g_encoder, oc_string(device->model));

    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
}

void
oc_create_dev_model_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_model_resource\n");
  oc_core_populate_resource(
    resource_idx, device, "/dev/model", OC_IF_D, APPLICATION_CBOR,
    OC_DISCOVERABLE, oc_core_dev_model_get_handler, 0, 0, 0, 1, ":dpa.0.15");
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
    cbor_encode_int(&g_encoder, (int64_t)device->ia);
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

  /* check if the accept header is CBOR-format */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  // only set ia in programming mode
  size_t device_index = request->resource->device;
  if (oc_knx_device_in_programming_mode(device_index) == false) {
    PRINT("oc_core_dev_ia_put_handler: not in programming mode\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  // parse the data.
  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    int temp;
    PRINT("  oc_core_dev_ia_put_handler received : %d\n",
          (int)rep->value.integer);
    oc_core_set_device_ia(device_index, (int)rep->value.integer);
    temp = (int)rep->value.integer;

    oc_storage_write(KNX_STORAGE_IA, (uint8_t *)&temp, sizeof(temp));

    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
  }

  oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
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
  // only set ia in programming mode
  if (oc_knx_device_in_programming_mode(device_index) == false) {
    PRINT("oc_core_dev_hostname_put_handler: not in programming mode\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_STRING)) {
    PRINT("  oc_core_dev_hostname_put_handler received : %s\n",
          oc_string(rep->value.string));
    oc_core_set_device_hostname(device_index, oc_string(rep->value.string));

    oc_storage_write(KNX_STORAGE_HOSTNAME,
                     (uint8_t *)oc_string(rep->value.string),
                     oc_string_len(rep->value.string));

    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
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
    cbor_encode_text_stringz(&g_encoder, oc_string(device->hostname));
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
    resource_idx, device, "/dev/hame", OC_IF_P, APPLICATION_CBOR,
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
  // only set ia in programming mode
  if (oc_knx_device_in_programming_mode(device_index) == false) {
    PRINT("oc_core_dev_iid_put_handler: not in programming mode\n");
    oc_send_cbor_response(request, OC_STATUS_BAD_REQUEST);
    return;
  }

  oc_rep_t *rep = request->request_payload;
  if ((rep != NULL) && (rep->type == OC_REP_INT)) {
    PRINT("  oc_core_dev_iid_put_handler received : %d\n",(int)rep->value.integer);
    oc_core_set_device_iid(device_index, oc_string(rep->value.string));

    oc_storage_write(KNX_STORAGE_IID, (uint32_t*)&rep->value.integer, sizeof(uint32_t));

    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    return;
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
    cbor_encode_int(&g_encoder,device->iid);
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
    cbor_encode_boolean(&g_encoder, device->pm);
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
  // debugging
  if (rep != NULL) {
    PRINT("  oc_core_dev_pm_put_handler type: %d\n", rep->type);
  }

  if ((rep != NULL) && (rep->type == OC_REP_BOOL)) {
    PRINT("  oc_core_dev_pm_put_handler received : %d\n", rep->value.boolean);
    device->pm = rep->value.boolean;
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    oc_storage_write(KNX_STORAGE_PM, (uint8_t *)&(rep->value.boolean), 1);
    return;
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

void
oc_knx_device_storage_read(size_t device_index)
{

  uint64_t ia;
  int temp_size;
  char tempstring[20];
  bool pm;

  PRINT("Loading Device Config from Persistent storage\n");

  if (device_index >= oc_number_of_devices()) {
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
    device->ia = ia;
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
  temp_size = oc_storage_read(KNX_STORAGE_IID, (uint32_t *)&device->iid, sizeof(uint32_t));
  PRINT("  idd (storage) %d\n",device->iid);

  /* KNX_STORAGE_PM */
  temp_size = oc_storage_read(KNX_STORAGE_PM, (uint8_t *)&pm, 1);
  if (temp_size > 0) {
    device->pm = pm;
    PRINT("  pm (storage) %d\n", device->pm);
  }
}

void
oc_knx_device_storage_reset(size_t device_index, int reset_mode)
{
  (void)device_index;
  (void)reset_mode;

  char buf[2] = "";
  int zero = 0;

  if (device_index >= oc_number_of_devices()) {
    PRINT("oc_knx_device_storage_reset: device_index %d to large\n",
          (int)device_index);
    return;
  }

  // writing the empty values
  oc_storage_write(KNX_STORAGE_IA, (char *)&zero, sizeof(int));
  oc_storage_write(KNX_STORAGE_HOSTNAME, (char *)&buf, 1);
  oc_storage_write(KNX_STORAGE_IID, (char*)&zero, sizeof(uint32_t));

  oc_storage_write(KNX_STORAGE_PM, (char *)&zero, sizeof(uint8_t));

  oc_delete_group_object_table();
  oc_delete_group_rp_table();

  oc_delete_at_table();
}

bool
oc_knx_device_in_programming_mode(size_t device_index)
{

  if (device_index >= oc_number_of_devices()) {
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
  // oc_create_dev_name_resource(OC_DEV_NAME, device_index);
  oc_create_dev_model_resource(OC_DEV_MODEL, device_index);
  oc_create_dev_ia_resource(OC_DEV_IA, device_index);
  oc_create_dev_hostname_resource(OC_DEV_HOSTNAME, device_index);
  oc_create_dev_iid_resource(OC_DEV_IID, device_index);
  oc_create_dev_pm_resource(OC_DEV_PM, device_index);
  // should be last of the dev/xxx resources, it will list those.
  oc_create_dev_dev_resource(OC_DEV, device_index);
}
