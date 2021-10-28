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
#include "api/oc_knx_fp.h"
#include "oc_core_res.h"
#include <stdio.h>




/**
 * @brief request information structure
 *
 *      
	{
		"ga": 2305,
		"dpt": "1"
	},
	{
		"ga": 2306,
		"dpt": "5.1",
		"s": {
			"ga": 1306,
			"groupkey": "<key2>",
			"secSettings": {
				"a": true,
				"c": true
			}
		}
	}
]

 */
typedef struct oc_group_address_mapping_table_t
{
  int ga;           ///< KNX IoT group address, "ga" - 7 
  oc_string_t dpt;  ///< Datapoint type, "dpt" 116

  int s_ga;        ///< KNX Classic group address  "ga" - 7 
  oc_string_t s_group_key; ///< KNX secure shared group key "groupKey" - 107
  bool s_secSettings_a;    ///< "a" - 97 The field determine if authentication shall be applied for KNX Classic secure group communication.
  bool s_secSettings_b;  ///< "c" - 99 The field determine if authentication shall be applied for KNX Classic secure group communication.
} oc_group_address_mapping_table_t;

#define GAMT_MAX_ENTRIES 20
oc_group_address_mapping_table_t g_groups[GAMT_MAX_ENTRIES];
int g_gamt_current_entries = 0;


static void
oc_core_fp_gm_get_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_fp_gm_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /* example entry: </fp/gm/1>;ct=50 */
  for (i = 0; i < g_gamt_current_entries; i++) {
    if (i > 0) {
      length = oc_rep_add_line_to_buffer(",\n");
      response_length += length;
    }

    length = oc_rep_add_line_to_buffer("<fp/gm/");
    response_length += length;
    char string[10];
    sprintf((char *)&string, "%d", i + 1);
    length = oc_rep_add_line_to_buffer(string);
    response_length += length;

    length = oc_rep_add_line_to_buffer(";ct=50");
    response_length += length;
  }

  if (g_gamt_current_entries > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_gm_get_handler - end\n");
}


static void
oc_core_fp_gm_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  request->response->response_buffer->content_format = APPLICATION_CBOR;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_fp_gm_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/gm", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_fp_gm_get_handler, oc_core_fp_gm_post_handler, 0, 0, 0, 1,
    "urn:knx:if.c");
}


static void
oc_core_fp_gm_x_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_fp_gm_x_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  if (g_gamt_current_entries > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_gm_get_handler - end\n");
}



static void
oc_core_fp_gm_x_del_handler(oc_request_t *request,
                            oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int length = 0;
  PRINT("oc_core_fp_gm_x_del_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /* example entry: </fp/gm/1>;ct=50 */
  for (i = 0; i < g_gamt_current_entries; i++) {
    if (i > 0) {
      length = oc_rep_add_line_to_buffer(",\n");
      response_length += length;
    }

    length = oc_rep_add_line_to_buffer("<fp/gm/");
    response_length += length;
    char string[10];
    sprintf((char *)&string, "%d", i + 1);
    length = oc_rep_add_line_to_buffer(string);
    response_length += length;

    length = oc_rep_add_line_to_buffer(";ct=50");
    response_length += length;
  }

  if (g_gamt_current_entries > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fp_gm_get_handler - end\n");
}




void
oc_create_fp_gm_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_gm_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/gm/*", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE, oc_core_fp_gm_x_del_handler, 0, 0,
    oc_core_fp_gm_x_get_handler, 0, 1,
    "urn:knx:if.c");
}

static void
oc_core_fp_p_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  request->response->response_buffer->content_format = APPLICATION_CBOR;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_fp_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_p_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/p", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    0, oc_core_fp_p_post_handler, 0, 0, 0, 1, "urn:knx:if.c");
}

static void
oc_core_fp_r_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                          void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_CBOR) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  // size_t device_index = request->resource->device;

  request->response->response_buffer->content_format = APPLICATION_CBOR;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

void
oc_create_fp_r_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_r_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/fp/r", OC_IF_D, APPLICATION_CBOR, OC_DISCOVERABLE,
    0, oc_core_fp_r_post_handler, 0, 0, 0, 1, "urn:knx:if.c");
}

static void
oc_core_p_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                      void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_JSON) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /*
  int length = clf_add_line_to_buffer("{");
  response_length += length;

  length = clf_add_line_to_buffer("\"api\": { \"version\": \"1.0\",");
  response_length += length;

  length = clf_add_line_to_buffer("\"base\": \"/ \"}");
  response_length += length;

  length = clf_add_line_to_buffer("}");
  response_length += length;
  */

  request->response->response_buffer->content_format = APPLICATION_JSON;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

static void
oc_core_p_post_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                       void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;

  /* check if the accept header is cbor-format */
  if (request->accept != APPLICATION_JSON) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }
  /*
  int length = clf_add_line_to_buffer("{");
  response_length += length;

  length = clf_add_line_to_buffer("\"api\": { \"version\": \"1.0\",");
  response_length += length;

  length = clf_add_line_to_buffer("\"base\": \"/ \"}");
  response_length += length;

  length = clf_add_line_to_buffer("}");
  response_length += length;
  */

  request->response->response_buffer->content_format = APPLICATION_JSON;
  request->response->response_buffer->code = oc_status_code(OC_STATUS_OK);
  request->response->response_buffer->response_length = response_length;
}

static void
oc_create_p_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_p_resource\n");
  oc_core_lf_populate_resource(
    resource_idx, device, "/p", OC_IF_LL, APPLICATION_CBOR, OC_DISCOVERABLE,
    oc_core_p_get_handler, 0, oc_core_p_post_handler, 0, 1, "urn:knx:if.c");
}

/*
void
oc_create_fp_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fp_resource\n");
  oc_core_lf_populate_resource(resource_idx, device, "/fp", OC_IF_LL,
APPLICATION_LINK_FORMAT, OC_DISCOVERABLE, oc_core_p_get_handler, 0,
                            oc_core_p_post_handler, 0, 1, "urn:knx:if.c");
}
*/

void
oc_create_knx_fp_resources(size_t device_index)
{
  OC_DBG("oc_create_knx_fp_resources");

  oc_create_fp_gm_resource(OC_KNX_FP_GM, device_index);
  oc_create_fp_gm_x_resource(OC_KNX_FP_GM_X, device_index);

  //oc_create_fp_p_resource(OC_KNX_FP_P, device_index);
  //oc_create_fp_r_resource(OC_KNX_FP_R, device_index);
  //oc_create_p_resource(OC_KNX_P, device_index);
  // oc_create_fp_resource(OC_KNX_FP, device_index);
}
