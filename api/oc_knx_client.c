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

// ----------------------------------------------------------------------------

#define COAP_PORT (5683)
#define MAX_SECRET_LEN 32
#define MAX_PASSWORD_LEN 30

// ----------------------------------------------------------------------------

typedef struct broker_s_mode_userdata_t
{
  int ia;          //!< internal address of the destination
  char path[20];   //< the path on the device designated with ia
  int ga;          //!< group address to use
  char rp_type[3]; //!< mode to send the message "w"  = 1  "r" = 2  "rp" = 3
  char resource_url[20]; //< the url to pull the data from.
} broker_s_mode_userdata_t;

typedef struct oc_spake_context_t
{
  char spake_password[MAX_PASSWORD_LEN]; /**< spake password */
  oc_string_t serial_number;             /**< the serial number of the device */
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

static void oc_send_s_mode(oc_endpoint_t *endpoint, char *path, int sia_value,
                           int group_address, char *rp, uint8_t *value_data,
                           int value_size);

static int oc_s_mode_get_resource_value(char *resource_url, char *rp,
                                        uint8_t *buf, int buf_size);

// ----------------------------------------------------------------------------

#ifdef OC_SPAKE

static void
update_tokens(uint8_t *secret, int secret_size)
{
  oc_oscore_set_auth(secret, secret_size);
}

static void
finish_spake_handshake(oc_client_response_t *data)
{
  if (data->code != OC_STATUS_CHANGED) {
    OC_DBG_SPAKE("Error in Credential Verification!!!\n");
    return;
  }
  // OC_DBG_SPAKE("SPAKE2+ Handshake Finished!\n");
  // OC_DBG_SPAKE("  code: %d\n", data->code);
  // OC_DBG_SPAKE("Shared Secret: ");
  // for (int i = 0; i < 16; i++) {
  //  PRINT("%02x", Ka_Ke[i + 16]);
  //}
  // OC_DBG_SPAKE("\n");

  // shared_key is 16-byte array - NOT NULL TERMINATED
  uint8_t *shared_key = Ka_Ke + 16;
  size_t shared_key_len = 16;

  update_tokens(shared_key, shared_key_len);

  if (m_spake_cb) {
    // PRINT("CALLING CALLBACK------->\n");
    m_spake_cb(0, shared_key, shared_key_len);
  }
}

static void
do_credential_verification(oc_client_response_t *data)
{
  OC_DBG_SPAKE("\nReceived Credential Response!\n");

  OC_DBG_SPAKE("  code: %d\n", data->code);
  if (data->code != OC_STATUS_CHANGED) {
    OC_DBG_SPAKE("Error in Credential Response!!!\n");
    return;
  }

  char buffer[200];
  memset(buffer, 200, 1);
  oc_rep_to_json(data->payload, (char *)&buffer, 200, true);
  OC_DBG_SPAKE("%s", buffer);

  uint8_t *pB_bytes, *cB_bytes;
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
  uint8_t pA_bytes[63];
  size_t len_pA;
  mbedtls_ecp_group grp;

  mbedtls_ecp_group_init(&grp);
  mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

  oc_spake_calc_transcript_initiator(&w0, &w1, &privA, &pA, pB_bytes, Ka_Ke);
  oc_spake_calc_cA(Ka_Ke, cA, pB_bytes);

  mbedtls_ecp_point_write_binary(&grp, &pA, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                 &len_pA, pA_bytes, 63);
  oc_spake_calc_cB(Ka_Ke, local_cB, pA_bytes);

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

  char buffer[300];
  memset(buffer, 300, 1);
  oc_rep_to_json(data->payload, (char *)&buffer, 300, true);
  OC_DBG_SPAKE("%s", buffer);

  // TODO parse payload, obtain seed & salt, use it to derive w0 & w1
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
    rep = rep->next;
  }

  // TODO WARNING: init without free leaks memory every time it is called,
  // but for the test client this is not important
  // use mbedtls_mpi_free(mbedtls_mpi * X);
  mbedtls_mpi_init(&w0);
  mbedtls_mpi_init(&w1);
  mbedtls_mpi_init(&privA);
  mbedtls_ecp_point_init(&pA);
  mbedtls_ecp_point_init(&pubA);
  // use the global variable that comes with the input of the handshake
  oc_spake_calc_w0_w1(g_spake_ctx.spake_password, 32, salt, it, &w0, &w1);

  oc_spake_gen_keypair(&privA, &pubA);
  oc_spake_calc_pA(&pA, &pubA, &w0);
  uint8_t bytes_pA[65];
  oc_spake_encode_pubkey(&pA, bytes_pA);

  oc_init_post("/.well-known/knx/spake", data->endpoint, NULL,
               &do_credential_verification, HIGH_QOS, NULL);

  oc_rep_begin_root_object();
  oc_rep_i_set_byte_string(root, 10, bytes_pA, 65);
  oc_rep_end_root_object();

  oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR);
}

#endif /* OC_SPAKE */

int
oc_initiate_spake(oc_endpoint_t *endpoint, char *password)
{
  int return_value = -1;

#ifndef OC_SPAKE
  (void)endpoint;
#else /* OC_SPAKE*/
  // do parameter exchange
  oc_init_post("/.well-known/knx/spake", endpoint, NULL,
               &do_credential_exchange, HIGH_QOS, NULL);

  // Payload consists of just a random number? should be pretty easy...
  uint8_t
    rnd[32]; // not actually used by the server, so just send some gibberish
  oc_rep_begin_root_object();
  oc_rep_i_set_byte_string(root, 15, rnd, 32);
  oc_rep_end_root_object();

  strncpy((char *)&g_spake_ctx.spake_password, password, MAX_PASSWORD_LEN);

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

  PRINT("discovery_ia_cb\n");
  oc_endpoint_print(endpoint);

  size_t device_index = 0;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  int sender_ia = device->ia;

  broker_s_mode_userdata_t *cb_data = (broker_s_mode_userdata_t *)user_data;

  int value_size;
  if (cb_data->resource_url == NULL) {
    return OC_STOP_DISCOVERY;
  }
  if (cb_data->path == NULL) {
    return OC_STOP_DISCOVERY;
  }

  uint8_t *buffer = malloc(100);
  if (!buffer) {
    OC_WRN("discovery_ia_cb: out of memory allocating buffer");
  } //! buffer

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
oc_knx_client_do_broker_request(char *resource_url, int ia, char *destination,
                                char *rp)
{
  char query[20];
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
  }
  return 0;
}

// ----------------------------------------------------------------------------

bool
oc_is_s_mode_request(oc_request_t *request)
{
  if (request == NULL) {
    return false;
  }

  PRINT("  oc_is_s_mode_request %.*s\n", (int)request->uri_path_len,
        request->uri_path);
  if (strncmp(".knx", request->uri_path, request->uri_path_len) == 0) {
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

static void
oc_issue_s_mode(int scope, int sia_value, int group_address, char *rp,
                uint8_t *value_data, int value_size)
{
  //(void)sia_value; /* variable not used */

  PRINT("  oc_issue_s_mode : scope %d\n", scope);
  int ula_prefix = 0;

#ifndef GROUP_ADDRESSING
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
  group_mcast = oc_create_multicast_group_address(group_mcast, group_address,
                                                  ula_prefix, scope);
  PRINT("  ");
  PRINTipaddr(group_mcast);
  PRINT("\n");
#endif

  oc_send_s_mode(&group_mcast, "/.knx", sia_value, group_address, rp,
                 value_data, value_size);
}

static void
oc_send_s_mode(oc_endpoint_t *endpoint, char *path, int sia_value,
               int group_address, char *rp, uint8_t *value_data, int value_size)
{

  PRINT("  oc_send_s_mode : \n");
  PRINT("  ");
  PRINTipaddr(*endpoint);
  PRINT("\n");

#ifndef OC_OSCORE
  if (oc_init_post(path, endpoint, NULL, NULL, LOW_QOS, NULL)) {
#else  /* OC_OSCORE */
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
    // st M Service type code(write = w, read = r, response = rp) Enum : w, r,
    // rp
    oc_rep_i_set_text_string(value, 6, rp);

    // set the "value" key
    oc_rep_i_set_key(&value_map, 1);
    // copy the data, this is already in cbor from the fake response of the
    // resource GET function
    oc_rep_encode_raw_encoder(&value_map, value_data, value_size);

    cbor_encoder_close_container_checked(&root_map, &value_map);

    oc_rep_end_root_object();

    PRINT("S-MODE Payload Size: %d\n", oc_rep_get_encoded_payload_size());
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

  if (resource_url == NULL) {
    return 0;
  }

  oc_resource_t *my_resource =
    oc_ri_get_app_resource_by_uri(resource_url, strlen(resource_url), 0);
  if (my_resource == NULL) {
    PRINT(" oc_do_s_mode : error no URL found %s\n", resource_url);
    return 0;
  }

  uint8_t *buffer = malloc(100);
  if (!buffer) {
    OC_WRN("oc_do_s_mode: out of memory allocating buffer");
  } //! buffer

  oc_request_t request = { 0 };
  oc_response_t response = { 0 };
  response.separate_response = 0;
  oc_response_buffer_t response_buffer;
  // if (!response_buf && resource) {
  //  OC_DBG("coap_notify_observers: Issue GET request to resource %s\n\n",
  //         oc_string(resource->uri));
  response_buffer.buffer = buffer;
  response_buffer.buffer_size = 100;

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

  oc_rep_new(response_buffer.buffer, response_buffer.buffer_size);

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
  OC_ERR(" allocated buf too small to contain s-mode value");
  return 0;
}

void
oc_do_s_mode(char *resource_url, char *rp)
{
  int scope = 2;
  oc_do_s_mode_with_scope(scope, resource_url, rp);
}

void
oc_do_s_mode_with_scope(int scope, char *resource_url, char *rp)
{
  int value_size;
  if (resource_url == NULL) {
    return;
  }

  uint8_t *buffer = malloc(100);
  if (!buffer) {
    OC_WRN("oc_do_s_mode: out of memory allocating buffer");
  } //! buffer

  value_size = oc_s_mode_get_resource_value(resource_url, rp, buffer, 100);

  oc_resource_t *my_resource =
    oc_ri_get_app_resource_by_uri(resource_url, strlen(resource_url), 0);
  if (my_resource == NULL) {
    PRINT(" oc_do_s_mode : error no URL found %s\n", resource_url);
    return;
  }

  // get the sender ia
  size_t device_index = 0;
  oc_device_info_t *device = oc_core_get_device_info(device_index);
  int sia_value = device->ia;

  int group_address = -1;

  // loop over all group addresses and issue the s-mode command
  int index = oc_core_find_group_object_table_url(resource_url);
  while (index != -1) {
    int ga_len = oc_core_find_group_object_table_number_group_entries(index);
    oc_cflag_mask_t cflags = oc_core_group_object_table_cflag_entries(index);
    PRINT(" index %d rp = %s cflags %d flags=", index, rp, cflags);
    oc_print_cflags(cflags);

    // With a read command to a Group Object, the device send this Group
    // Object�s value.
    if (((cflags & OC_CFLAG_READ) == 0) &&
        ((cflags & OC_CFLAG_TRANSMISSION) == 0) &&
        ((cflags & OC_CFLAG_INIT) > 0)) {
      PRINT("    skipping index %d due to cflags %d flags=", index, cflags);
      oc_print_cflags(cflags);
    } else {
      PRINT("    handling: index %d\n", index);
      for (int j = 0; j < ga_len; j++) {
        group_address = oc_core_find_group_object_table_group_entry(index, j);
        PRINT("      ga : %d\n", group_address);
        // issue the s-mode command
        oc_issue_s_mode(scope, sia_value, group_address, rp, buffer,
                        value_size);

        // the recipient table contains the list of destinations that will
        // receive data. loop over the full recipient table and send a message
        // if the group is there
        for (int j = 0; j < oc_core_get_recipient_table_size(); j++) {
          bool found =
            oc_core_check_recipient_index_on_group_address(j, group_address);
          if (found) {
            char *url = oc_core_get_recipient_index_url_or_path(j);
            if (url) {
              PRINT(" broker send: %s\n", url);
              int ia = oc_core_get_recipient_ia(j);
              oc_knx_client_do_broker_request(resource_url, ia, url, rp);
            }
          }
        }
      }
    }
    index = oc_core_find_next_group_object_table_url(resource_url, index);
  }
}

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