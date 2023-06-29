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
#include "api/oc_knx_client.h"
#include "api/oc_knx_fp.h"
#include "api/oc_knx_sec.h"
#ifdef OC_SPAKE
#include "oc_spake2plus.h"
#endif
#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// ----------------------------------------------------------------------------

#define COAP_PORT (5683)
#define MAX_SECRET_LEN (32)
#define MAX_PASSWORD_LEN (30)
#define MAX_SERIAL_NUMBER_LEN (7) // serial number in bytes

// ----------------------------------------------------------------------------

typedef struct broker_s_mode_userdata_t
{
  int ia;          /**< internal address of the destination */
  char path[20];   /**< the path on the device designated with ia */
  uint32_t ga;     /**< group address to use */
  char rp_type[3]; /**< mode to send the message "w"  = 1  "r" = 2  "a" = 3
                      ("rp") */
  char resource_url[20]; /**< the url to pull the data from. */
} broker_s_mode_userdata_t;

typedef struct oc_spake_context_t
{
  char spake_password[MAX_PASSWORD_LEN]; /**< spake password */
  oc_string_t serial_number; /**< the serial number of the device string */
  oc_string_t recipient_id;  /**< the recipient id used (byte string) */
  oc_string_t oscore_id;     /**< the oscore id used (byte string) */
} oc_spake_context_t;

// ----------------------------------------------------------------------------

oc_s_mode_response_cb_t m_s_mode_cb = NULL;
oc_spake_cb_t m_spake_cb = NULL;

// SPAKE2
#ifdef OC_SPAKE
static mbedtls_mpi w0, w1, privA;
static mbedtls_ecp_point pA, pubA;
static uint8_t Ka_Ke[MAX_SECRET_LEN];
static oc_spake_context_t g_spake_ctx;
#endif

// ----------------------------------------------------------------------------

static void oc_send_s_mode(oc_endpoint_t *endpoint, char *path,
                           uint32_t sia_value, uint32_t group_address, char *rp,
                           uint8_t *value_data, int value_size);

static int oc_s_mode_get_resource_value(char *resource_url, char *rp,
                                        uint8_t *buf, int buf_size);

// ----------------------------------------------------------------------------

#ifdef OC_SPAKE

static void
update_tokens(uint8_t *secret, int secret_size)
{
  PRINT("update_tokens: \n");
  oc_oscore_set_auth_mac(oc_string(g_spake_ctx.serial_number),
                         oc_string_len(g_spake_ctx.serial_number),
                         oc_string(g_spake_ctx.recipient_id),
                         oc_byte_string_len(g_spake_ctx.recipient_id), secret,
                         secret_size);
}

static void
finish_spake_handshake(oc_client_response_t *data)
{
  if (data->code != OC_STATUS_CHANGED) {
    OC_DBG_SPAKE("Error in Credential Verification!!!\n");
    mbedtls_mpi_free(&w0);
    mbedtls_mpi_free(&w1);
    mbedtls_mpi_free(&privA);
    mbedtls_ecp_point_free(&pA);
    mbedtls_ecp_point_free(&pubA);
    return;
  }

  // shared_key is 16-byte array - NOT NULL TERMINATED
  uint8_t *shared_key = Ka_Ke + 16;
  int shared_key_len = 16;

  update_tokens(shared_key, shared_key_len);

  // free up the memory used by the handshake
  mbedtls_mpi_free(&w0);
  mbedtls_mpi_free(&w1);
  mbedtls_mpi_free(&privA);
  mbedtls_ecp_point_free(&pA);
  mbedtls_ecp_point_free(&pubA);

  if (m_spake_cb) {
    m_spake_cb(
      0, oc_string(g_spake_ctx.serial_number), oc_string(g_spake_ctx.oscore_id),
      oc_byte_string_len(g_spake_ctx.oscore_id), shared_key, shared_key_len);
  }
}

static void
do_credential_verification(oc_client_response_t *data)
{
  OC_DBG_SPAKE("\nReceived Credential Response!\n");

  OC_DBG_SPAKE("  code: %d\n", data->code);
  if (data->code != OC_STATUS_CHANGED) {
    OC_DBG_SPAKE("Error in Credential Response!!!\n");
    mbedtls_mpi_free(&w0);
    mbedtls_mpi_free(&w1);
    mbedtls_mpi_free(&privA);
    mbedtls_ecp_point_free(&pA);
    mbedtls_ecp_point_free(&pubA);
    return;
  }

  oc_print_rep_as_json(data->payload, true);

  uint8_t *pB_bytes = NULL;
  uint8_t **cB_bytes = NULL;

  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_BYTE_STRING) {
      // pb
      if (rep->iname == 11) {
        pB_bytes = rep->value.string.ptr;
      }
      // cb
      if (rep->iname == 13) {
        cB_bytes = rep->value.string.ptr;
      }
    }

    rep = rep->next;
  }

  uint8_t cA[32];
  uint8_t local_cB[32];
  uint8_t pA_bytes[kPubKeySize];
  size_t len_pA;
  mbedtls_ecp_group grp;

  mbedtls_ecp_group_init(&grp);
  mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

  oc_spake_calc_transcript_initiator(&w0, &w1, &privA, &pA, pB_bytes, Ka_Ke);
  oc_spake_calc_cA(Ka_Ke, cA, pB_bytes);

  mbedtls_ecp_point_write_binary(&grp, &pA, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                 &len_pA, pA_bytes, kPubKeySize);
  oc_spake_calc_cB(Ka_Ke, local_cB, pA_bytes);

  mbedtls_ecp_group_free(&grp);

  oc_init_post("/.well-known/knx/spake", data->endpoint, NULL,
               &finish_spake_handshake, HIGH_QOS, NULL);
  oc_rep_begin_root_object();
  oc_rep_i_set_byte_string(root, 14, cA, 32);
  oc_rep_end_root_object();
  oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR);
}

static void
do_credential_exchange(oc_client_response_t *data)
{
  OC_DBG_SPAKE("\nReceived Parameter Response!\n");

  OC_DBG_SPAKE("  code: %d\n", data->code);
  if (data->code != OC_STATUS_CHANGED) {
    OC_DBG_SPAKE("Error in Parameter Response!!! %d\n", data->code);
    return;
  }
  oc_print_rep_as_json(data->payload, true);

  char buffer[300];
  memset(buffer, 300, 1);
  oc_rep_to_json(data->payload, (char *)&buffer, 300, true);
  OC_DBG_SPAKE("%s", buffer);

  int it;
  uint8_t *salt;

  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    // rnd - we just skip
    if (rep->type == OC_REP_BYTE_STRING && rep->iname == 15)
      ;

    // pbkdf2 - interesting
    if (rep->type == OC_REP_OBJECT && rep->iname == 12) {
      oc_rep_t *inner_rep = rep->value.object;
      while (inner_rep != NULL) {
        // it
        if (inner_rep->type == OC_REP_INT && inner_rep->iname == 16) {
          it = inner_rep->value.integer;
        }
        // salt
        if (inner_rep->type == OC_REP_BYTE_STRING && inner_rep->iname == 5) {
          salt = inner_rep->value.string.ptr;
        }

        inner_rep = inner_rep->next;
      }
    }
    // oscore context
    if (rep->type == OC_REP_BYTE_STRING && rep->iname == 0) {
      strncpy((char *)&g_spake_ctx.oscore_id, oc_string(rep->value.string),
              MAX_PASSWORD_LEN);
    }
    rep = rep->next;
  }

  mbedtls_mpi_init(&w0);
  mbedtls_mpi_init(&w1);
  mbedtls_mpi_init(&privA);
  mbedtls_ecp_point_init(&pA);
  mbedtls_ecp_point_init(&pubA);
  // use the global variable that comes with the input of the handshake
  oc_spake_calc_w0_w1(g_spake_ctx.spake_password, 32, salt, it, &w0, &w1);

  oc_spake_gen_keypair(&privA, &pubA);
  oc_spake_calc_pA(&pA, &pubA, &w0);
  uint8_t bytes_pA[kPubKeySize];
  oc_spake_encode_pubkey(&pA, bytes_pA);

  oc_init_post("/.well-known/knx/spake", data->endpoint, NULL,
               &do_credential_verification, HIGH_QOS, NULL);

  oc_rep_begin_root_object();
  oc_rep_i_set_byte_string(root, 10, bytes_pA, kPubKeySize);
  oc_rep_end_root_object();

  oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR);
}

#endif /* OC_SPAKE */

int
oc_initiate_spake_parameter_request(oc_endpoint_t *endpoint,
                                    char *serial_number, char *password,
                                    char *recipient_id, size_t recipient_id_len)
{
  int return_value = -1;

#ifndef OC_SPAKE
  (void)endpoint;
#else /* OC_SPAKE*/
  // do parameter exchange
  oc_init_post("/.well-known/knx/spake", endpoint, NULL,
               &do_credential_exchange, HIGH_QOS, NULL);

  // TODO fill with actual random data
  uint8_t
    rnd[32]; // not actually used by the server, so just send some gibberish
  oc_rep_begin_root_object();

  oc_rep_i_set_byte_string(root, 0, recipient_id, recipient_id_len);
  oc_byte_string_copy_from_char_with_size(&g_spake_ctx.recipient_id,
                                          recipient_id, recipient_id_len);

  oc_rep_i_set_byte_string(root, 15, rnd, 32);
  oc_rep_end_root_object();

  strncpy((char *)&g_spake_ctx.spake_password, password, MAX_PASSWORD_LEN);

  oc_string_copy_from_char(&g_spake_ctx.serial_number, serial_number);

  // oc_conv_hex_string_to_oc_string(endpoint->oscore_id,
  //                                 strlen(endpoint->oscore_id),
  //                                     &(g_spake_ctx.serial_number));
  // oc_string_copy_from_char(&g_spake_ctx.serial_number,
  // endpoint->serial_number);

  if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
    return_value = 0;
  }

#endif /* OC_SPAKE */
  return return_value;
}

int
oc_initiate_spake(oc_endpoint_t *endpoint, char *password, char *recipient_id)
{
  int return_value = -1;

  // sort this one out later..
  return return_value;

#ifndef OC_SPAKE
  (void)endpoint;
#else /* OC_SPAKE*/
  // do parameter exchange
  oc_init_post("/.well-known/knx/spake", endpoint, NULL,
               &do_credential_exchange, HIGH_QOS, NULL);

  // TODO fill with actual random data
  uint8_t
    rnd[32]; // not actually used by the server, so just send some gibberish
  oc_rep_begin_root_object();
  if (recipient_id) {
    // convert from hex string to bytes
    oc_conv_hex_string_to_oc_string(recipient_id, strlen(recipient_id),
                                    &g_spake_ctx.recipient_id);
    oc_rep_i_set_byte_string(root, 0, oc_string(g_spake_ctx.recipient_id),
                             oc_byte_string_len(g_spake_ctx.recipient_id));
    // oc_rep_i_set_byte_string(root, 0, oscore_id, strlen(oscore_id));
    // strncpy((char *)&g_spake_ctx.recipient_id, recipient_id,
    // MAX_PASSWORD_LEN);
  }
  oc_rep_i_set_byte_string(root, 15, rnd, 32);
  oc_rep_end_root_object();

  // password : still a string
  strncpy((char *)&g_spake_ctx.spake_password, password, MAX_PASSWORD_LEN);
  // oc_string_copy(&g_spake_ctx.serial_number, endpoint->oscore_id);

  // serial number in endpoint is a string, so it needs to be converted before
  // going into the spake
  // oc_conv_hex_string_to_oc_string(endpoint->serial_number,
  // strlen(endpoint->serial_number),
  //                                &g_spake_ctx.serial_number);
  // oc_string_copy_from_char(&g_spake_ctx.serial_number,
  // endpoint->serial_number);

  if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
    return_value = 0;
  }

#endif /* OC_SPAKE */
  return return_value;
}

// ----------------------------------------------------------------------------

static oc_discovery_flags_t
discovery_ia_cb(const char *payload, int len, oc_endpoint_t *endpoint,
                void *user_data)
{
  //(void)anchor;
  (void)payload;
  (void)len;
  uint8_t buffer[100];

  PRINT("discovery_ia_cb\n");
  oc_endpoint_print(endpoint);

  size_t device_index = 0;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  uint32_t sender_ia = device->ia;

  broker_s_mode_userdata_t *cb_data = (broker_s_mode_userdata_t *)user_data;

  int value_size;
  if (cb_data->resource_url == NULL) {
    return OC_STOP_DISCOVERY;
  }
  if (cb_data->path == NULL) {
    return OC_STOP_DISCOVERY;
  }

  value_size =
    oc_s_mode_get_resource_value(cb_data->resource_url, "r", buffer, 100);

  oc_send_s_mode(endpoint, cb_data->path, sender_ia, cb_data->ga,
                 cb_data->rp_type, buffer, value_size);

  if (cb_data) {
    free(user_data);
  }

  return OC_STOP_DISCOVERY;
}

int
oc_knx_client_do_broker_request(char *resource_url, uint32_t ia,
                                char *destination, char *rp)
{
  char query[20];

  // TODO: do the new discovery here
  snprintf(query, 20, "if=urn:knx:ia.%d", ia);

  // not sure if we should use a malloc here, what would happen if there are no
  // devices found? because that causes a memory leak
  broker_s_mode_userdata_t *cb_data =
    (broker_s_mode_userdata_t *)malloc(sizeof(broker_s_mode_userdata_t));
  if (cb_data != NULL) {
    memset(cb_data, 0, sizeof(broker_s_mode_userdata_t));
    cb_data->ia = ia;
    strncpy(cb_data->rp_type, rp, 2);
    strncpy(cb_data->resource_url, resource_url, 20);
    strncpy(cb_data->path, destination, 20);

    oc_do_wk_discovery_all(query, 2, discovery_ia_cb, cb_data);
    oc_do_wk_discovery_all(query, 3, discovery_ia_cb, cb_data);
    oc_do_wk_discovery_all(query, 5, discovery_ia_cb, cb_data);
  } else {
    OC_ERR("cb_data is NULL");
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

bool
oc_is_redirected_request(oc_request_t *request)
{
  if (request == NULL) {
    return false;
  }

  PRINT("  oc_is_redirected_request %.*s\n", (int)request->uri_path_len,
        request->uri_path);
  if (strncmp(".knx", request->uri_path, request->uri_path_len) == 0) {
    return true;
  }
  if (strncmp("/p", request->uri_path, request->uri_path_len) == 0) {
    return true;
  }
  return false;
}

oc_rep_t *
oc_s_mode_get_value(oc_request_t *request)
{

  /* loop over the request document to parse all the data */
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_OBJECT: {
      // find the storage index, e.g. for this object
      oc_rep_t *object = rep->value.object;

      object = rep->value.object;
      while (object != NULL) {
        // search for "value" (1)
        if (object->iname == 1) {
          return object;
        }
        object = object->next;
      }
    }
    case OC_REP_NIL:
      break;
    default:
      break;
    }
    rep = rep->next;
  }
  return NULL;
}

void
oc_issue_s_mode(int scope, int sia_value, uint32_t grpid,
                uint32_t group_address, uint64_t iid, char *rp,
                uint8_t *value_data, int value_size)
{
  PRINT("  oc_issue_s_mode : scope %d\n", scope);

#ifdef S_MODE_ALL_COAP_NODES
#ifdef OC_OSCORE
  oc_make_ipv6_endpoint(group_mcast, IPV6 | MULTICAST | OSCORE, COAP_PORT, 0xff,
                        -scope, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0xfd);
#else
  oc_make_ipv6_endpoint(group_mcast, IPV6 | DISCOVERY | MULTICAST, COAP_PORT,
                        0xff, scope, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00,
                        0xfd);
  // oc_make_ipv6_endpoint(mcast, IPV6 | DISCOVERY | MULTICAST, 5683, 0xff,
  // scope,
  //                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0xfd);
#endif

#else
  /* using group addressing */
  oc_endpoint_t group_mcast;
  memset(&group_mcast, 0, sizeof(group_mcast));
  group_mcast =
    oc_create_multicast_group_address(group_mcast, grpid, iid, scope);
#endif
  // set the group_address to the group address, since this field is used
  // to find the OSCORE context id
  group_mcast.group_address = group_address;
  oc_send_s_mode(&group_mcast, "/.knx", sia_value, group_address, rp,
                 value_data, value_size);
}

static void
oc_send_s_mode(oc_endpoint_t *endpoint, char *path, uint32_t sia_value,
               uint32_t group_address, char *rp, uint8_t *value_data,
               int value_size)
{
  char token[8];

  PRINT("  oc_send_s_mode : \n");
  PRINT("  ");
  PRINTipaddr(*endpoint);
  PRINT("\n");

#ifndef OC_OSCORE
  if (oc_init_post(path, endpoint, NULL, NULL, LOW_QOS, NULL)) {
#else  /* OC_OSCORE */
  /* not sure if it is needed, the endpoint should already have the OSCORE flag
   * set */
  endpoint->flags = endpoint->flags | OSCORE;
  if (oc_init_multicast_update(endpoint, path, NULL)) {
#endif /* OC_OSCORE */
    /*
    { 4: <sia>, 5: { 6: <st>, 7: <ga>, 1: <value> } }
    */

    oc_rep_begin_root_object();

    oc_rep_i_set_int(root, 4, sia_value);

    oc_rep_i_set_key(&root_map, 5);
    CborEncoder value_map;
    cbor_encoder_create_map(&root_map, &value_map, CborIndefiniteLength);

    // ga
    oc_rep_i_set_int(value, 7, group_address);
    // st M Service type code(write = w, read = r, response = a)
    // Enum : w, r, a (rp)
    oc_rep_i_set_text_string(value, 6, rp);

    // set the "value" key
    // oc_rep_i_set_key(&value_map, 1);
    // copy the data, this is already in cbor from the fake response of the
    // resource GET function
    // the GET function retrieves the data = { 1 : <value> } e.g including
    // the open/close object data. hence this needs to be removed.
    if (value_size > 2) {
      oc_rep_encode_raw_encoder(&value_map, &value_data[1], value_size - 2);
    }

    cbor_encoder_close_container_checked(&root_map, &value_map);

    oc_rep_end_root_object();

    PRINT("oc_send_s_mode: S-MODE Payload Size: %d\n",
          oc_rep_get_encoded_payload_size());
    OC_LOGbytes_OSCORE(oc_rep_get_encoder_buf(),
                       oc_rep_get_encoded_payload_size());

#ifndef OC_OSCORE
    if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  Sent POST request\n");
#else
    if (oc_do_multicast_update()) {
      PRINT("  Sent oc_do_multicast_update update\n");
#endif
    } else {
      PRINT("  Could not send POST request\n");
    }
  }
}

static int
oc_s_mode_get_resource_value(char *resource_url, char *rp, uint8_t *buf,
                             int buf_size)
{
  (void)rp;
  uint8_t buffer[50];

  if (resource_url == NULL) {
    return 0;
  }

  oc_resource_t *my_resource =
    oc_ri_get_app_resource_by_uri(resource_url, strlen(resource_url), 0);
  if (my_resource == NULL) {
    PRINT(" oc_do_s_mode : error no URL found %s\n", resource_url);
    return 0;
  }

  oc_request_t request = { 0 };
  oc_response_t response = { 0 };
  response.separate_response = 0;
  oc_response_buffer_t response_buffer;

  response_buffer.buffer = buffer;
  response_buffer.buffer_size = 50;

  // same initialization as oc_ri.c
  response_buffer.code = 0;
  response_buffer.response_length = 0;
  response_buffer.content_format = 0;

  response.separate_response = NULL;
  response.response_buffer = &response_buffer;

  request.response = &response;
  request.request_payload = NULL;
  request.query = NULL;
  request.query_len = 0;
  request.resource = NULL;
  request.origin = NULL;
  request._payload = NULL;
  request._payload_len = 0;

  request.content_format = APPLICATION_CBOR;
  request.accept = APPLICATION_CBOR;
  request.uri_path = resource_url;
  request.uri_path_len = strlen(resource_url);

  oc_rep_new(response_buffer.buffer, (int)response_buffer.buffer_size);

  // get the value...oc_request_t request_obj;
  oc_interface_mask_t iface_mask = OC_IF_NONE;
  // void *data;
  my_resource->get_handler.cb(&request, iface_mask, NULL);

  // get the data
  int value_size = oc_rep_get_encoded_payload_size();
  uint8_t *value_data = request.response->response_buffer->buffer;

  // Cache value data, as it gets overwritten in oc_issue_do_s_mode
  if (value_size < buf_size) {
    memcpy(buf, value_data, value_size);
    return value_size;
  }
  OC_ERR(" allocated buffer too small to contain s-mode value");
  return 0;
}

void
oc_do_s_mode_read(int64_t group_address)
{
  size_t device_index = 0;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  uint32_t sia_value = device->ia;
  uint64_t iid = device->iid;
  uint32_t grpid = 0;

  PRINT("oc_do_s_mode_read : ga=%u ia=%d, iid=%" PRIu64 "\n",
        (uint32_t)group_address, sia_value, iid);

  // find the grpid that belongs to the group address
  grpid = oc_find_grpid_in_publisher_table(group_address);
  if (grpid > 0) {
#ifdef OC_USE_MULTICAST_SCOPE_2
    oc_issue_s_mode(2, sia_value, grpid, group_address, iid, "r", 0, 0);
#endif
    oc_issue_s_mode(5, sia_value, grpid, group_address, iid, "r", 0, 0);
  } else if (group_address > 0) {

#ifdef OC_USE_MULTICAST_SCOPE_2
    oc_issue_s_mode(2, sia_value, group_address, group_address, iid, "r", 0, 0);
#endif
    oc_issue_s_mode(5, sia_value, group_address, group_address, iid, "r", 0, 0);
  }
}

// note: this function does not check the transmit flag
// the caller of this function needs to check if the flag is set.
void
oc_do_s_mode_with_scope_and_check(int scope, char *resource_url, char *rp,
                                  bool check)
{
  int value_size;
  bool error = true;
  uint8_t buffer[50];

  // do the checks
  if (strcmp(rp, "w") == 0) {
    error = false;
  } else if (strcmp(rp, "r") == 0) {
    error = false;
  } else if (strcmp(rp, "a") == 0) {
    // spec 1.1
    error = false;
  } else if (strcmp(rp, "rp") == 0) {
    // spec 1.0
    error = false;
  }
  if (error) {
    OC_ERR("oc_do_s_mode_with_scope_internal : rp value incorrect %s", rp);
    return;
  }

  if (resource_url == NULL) {
    OC_ERR("oc_do_s_mode_with_scope_internal: resource url is NULL");
    return;
  }

  // get the device
  size_t device_index = 0;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  if (device == NULL) {
    PRINT(" oc_do_s_mode_with_scope_internal : device is NULL\n");
    return;
  }

  // check if the device is in "loaded state"
  if (oc_is_device_in_runtime(device_index) == false) {
    PRINT(
      " oc_do_s_mode_with_scope_internal : device not in loaded state: %d\n",
      device->lsm_s);
    return;
  }

  oc_resource_t *my_resource =
    oc_ri_get_app_resource_by_uri(resource_url, strlen(resource_url), 0);
  if (my_resource == NULL) {
    PRINT(" oc_do_s_mode_with_scope_internal : error no URL found %s\n",
          resource_url);
    return;
  }

  value_size = oc_s_mode_get_resource_value(resource_url, rp, buffer, 50);

  // get the sender ia
  uint32_t sia_value = device->ia;
  uint64_t iid = device->iid;
  uint32_t group_address = 0;

  // loop over all group addresses and issue the s-mode command
  int index = oc_core_find_group_object_table_url(resource_url);
  if (index == -1) {
    PRINT(" oc_do_s_mode_with_scope_internal : no table entry found for %s\n",
          resource_url);
    return;
  }
  while (index != -1) {
    int ga_len = oc_core_find_group_object_table_number_group_entries(index);
    oc_cflag_mask_t cflags = oc_core_group_object_table_cflag_entries(index);

    PRINT(" index %d rp = %s cflags %d flags=", index, rp, cflags);
    oc_print_cflags(cflags);

    bool do_send = (cflags & OC_CFLAG_TRANSMISSION) > 0;
    if (check == false) {
      PRINT("    not checking flags.. always send\n");
      do_send = true;
    }

    if (do_send) {
      // o
      PRINT(" index %d rp = %s cflags %d flags=", index, rp, cflags);
      oc_print_cflags(cflags);

      // With a read command to a Group Object, the device send this Group
      // Object's value.
      PRINT("    handling: index %d\n", index);
      for (int j = 0; j < ga_len; j++) {
        group_address = oc_core_find_group_object_table_group_entry(index, j);
        PRINT("      ga : %d\n", group_address);
        if (j == 0) {
          // issue the s-mode command, but only for the first ga entry
          uint32_t grpid = oc_find_grpid_in_recipient_table(group_address);
          if (grpid > 0) {
            oc_issue_s_mode(scope, sia_value, grpid, group_address, iid, rp,
                            buffer, value_size);
          } else {
            // send to group address in multicast address
            oc_issue_s_mode(scope, sia_value, group_address, group_address, iid,
                            rp, buffer, value_size);
          }
        }
        // the recipient table contains the list of destinations that will
        // receive data. loop over the full recipient table and send a message
        // if the group is there
        for (int jr = 0; jr < oc_core_get_recipient_table_size(); jr++) {
          bool found =
            oc_core_check_recipient_index_on_group_address(jr, group_address);
          if (found) {
            char *url = oc_core_get_recipient_index_url_or_path(jr);
            if (url) {
              PRINT(" broker send: %s\n", url);
              uint32_t ia = oc_core_get_recipient_ia(jr);
              if (ia > 0) {
                // ia == 0 is reserved, so only send with ia > 0
                oc_knx_client_do_broker_request(resource_url, ia, url, rp);
              }
            }
          }
        }
      }
    } else {
      PRINT("    not send due to flags\n");
    }
    /* cflag */
    index = oc_core_find_next_group_object_table_url(resource_url, index);
  }
}
// note: this function does not check the transmit flag
// the caller of this function needs to check if the flag is set.
void
oc_do_s_mode_with_scope_no_check(int scope, char *resource_url, char *rp)
{
  oc_do_s_mode_with_scope_and_check(scope, resource_url, rp, false);
}

// note: this function does check the transmit flag
void
oc_do_s_mode_with_scope(int scope, char *resource_url, char *rp)
{
  oc_do_s_mode_with_scope_and_check(scope, resource_url, rp, true);
}

// ----------------------------------------------------------------------------

bool
oc_set_spake_response_cb(oc_spake_cb_t my_func)
{
  m_spake_cb = my_func;
  return true;
}

// ----------------------------------------------------------------------------

bool
oc_set_s_mode_response_cb(oc_s_mode_response_cb_t my_func)
{
  m_s_mode_cb = my_func;
  return true;
}

oc_s_mode_response_cb_t
oc_get_s_mode_response_cb()
{
  return m_s_mode_cb;
}

// ----------------------------------------------------------------------------