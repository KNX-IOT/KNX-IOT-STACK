/*
// Copyright (c) 2016 Intel Corporation
// Copyright (c) 2021-2023 Cascoda Ltd.
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

#include "messaging/coap/coap.h"
#include "messaging/coap/transactions.h"
#ifdef OC_TCP
#include "messaging/coap/coap_signal.h"
#endif /* OC_TCP */
#include "oc_api.h"
#ifdef OC_OSCORE
#include "security/oc_tls.h"
#endif /* OC_OSCORE */
#ifdef OC_CLIENT

static coap_transaction_t *transaction;
coap_packet_t request[1];
oc_client_cb_t *client_cb;

//#define OC_BLOCK_WISE_REQUEST

#ifdef OC_BLOCK_WISE_REQUEST
static oc_blockwise_state_t *request_buffer = NULL;
#endif /* OC_BLOCK_WISE_REQUEST */

#ifdef OC_OSCORE
oc_message_t *multicast_update = NULL;
#endif /* OC_OSCORE */
oc_event_callback_retval_t oc_ri_remove_client_cb(void *data);

static bool
dispatch_coap_request(oc_content_format_t content, oc_content_format_t accept)
{
  int payload_size = oc_rep_get_encoded_payload_size();

  if ((client_cb->method == OC_PUT || client_cb->method == OC_POST) &&
      payload_size > 0) {

#ifdef OC_BLOCK_WISE_REQUEST
    request_buffer->payload_size = (uint32_t)payload_size;
    uint32_t block_size;
#ifdef OC_TCP
    if (!(transaction->message->endpoint.flags & TCP) &&
        payload_size > OC_BLOCK_SIZE) {
#else  /* OC_TCP */
    if ((long)payload_size > OC_BLOCK_SIZE) {
#endif /* !OC_TCP */
      const void *payload = oc_blockwise_dispatch_block(
        request_buffer, 0, (uint32_t)OC_BLOCK_SIZE, &block_size);
      if (payload) {
        coap_set_payload(request, payload, block_size);
        coap_set_header_block1(request, 0, 1, (uint16_t)block_size);
        coap_set_header_size1(request, (uint32_t)payload_size);
        request->type = COAP_TYPE_CON;
        client_cb->qos = HIGH_QOS;
      }
    } else {
      coap_set_payload(request, request_buffer->buffer, payload_size);
      request_buffer->ref_count = 0;
    }
#else  /* OC_BLOCK_WISE_REQUEST */
    if (payload_size > 0) {
      coap_set_payload(request,
                       transaction->message->data + COAP_MAX_HEADER_SIZE,
                       payload_size);
    }
#endif /* !OC_BLOCK_WISE_REQUEST */
  }

  if (payload_size > 0) {
    coap_set_header_content_format(request, content);
  }
  coap_set_header_accept(request, accept);

  bool success = false;
  transaction->message->length =
    coap_serialize_message(request, transaction->message->data);
  if (transaction->message->length > 0) {

    if ((client_cb->handler.response == NULL) &&
        (client_cb->handler.discovery_all == NULL) &&
        (client_cb->handler.discovery == NULL)) {
      // set the delayed callback on 0, so we clean up immediately
      // since there is no response callback, so we are not expecting a result
      // so set the ref count on 0, so that the transaction is deleted, without
      // callback
      OC_DBG(" refcount for handle.reponse=None : %d",
             transaction->message->ref_count);
      // transaction->message->ref_count = 0;
    }

    coap_send_transaction(transaction);

    if ((client_cb->handler.response == NULL) &&
        (client_cb->handler.discovery_all == NULL) &&
        (client_cb->handler.discovery == NULL)) {
      // set the delayed callback on 0, so we clean up immediately the client_cb
      // data since there is no response callback, so we are not expecting a
      // result
      // OC_DBG("Not set delayed callback for transaction: %p", transaction);
      oc_ri_remove_client_cb(client_cb);
    } else if (client_cb->observe_seq == -1) {
      if (client_cb->qos == LOW_QOS)
        oc_set_delayed_callback(client_cb, &oc_ri_remove_client_cb,
                                OC_NON_LIFETIME);
      else
        oc_set_delayed_callback(client_cb, &oc_ri_remove_client_cb,
                                OC_EXCHANGE_LIFETIME);
    }

    success = true;
  } else {
    // transaction->message->length == 0
    // did not send the request, just remove the transaction
    coap_clear_transaction(transaction);
    oc_ri_remove_client_cb(client_cb);
  }

#ifdef OC_BLOCK_WISE_REQUEST
  if (request_buffer && request_buffer->ref_count == 0) {
    oc_blockwise_free_request_buffer(request_buffer);
  }
  request_buffer = NULL;
#endif /* OC_BLOCK_WISE_REQUEST */

  transaction = NULL;
  client_cb = NULL;

  return success;
}

static bool
prepare_coap_request_ex(oc_client_cb_t *cb, oc_content_format_t accept)
{
  coap_message_type_t type = COAP_TYPE_NON;

  if (cb->qos == HIGH_QOS) {
    type = COAP_TYPE_CON;
  }

  transaction =
    coap_new_transaction(cb->mid, cb->token, cb->token_len, &cb->endpoint);

  if (!transaction) {
    return false;
  }

  oc_rep_new(transaction->message->data + COAP_MAX_HEADER_SIZE, OC_BLOCK_SIZE);

#ifdef OC_BLOCK_WISE_REQUEST
  if (cb->method == OC_PUT || cb->method == OC_POST) {
    request_buffer = oc_blockwise_alloc_request_buffer(
      oc_string(cb->uri) + 1, oc_string_len(cb->uri) - 1, &cb->endpoint,
      cb->method, OC_BLOCKWISE_CLIENT);
    if (!request_buffer) {
      OC_ERR("request_buffer is NULL");
      return false;
    }
    oc_rep_new(request_buffer->buffer, OC_MAX_APP_DATA_SIZE);

    request_buffer->mid = cb->mid;
    request_buffer->client_cb = cb;
  }
#endif /* OC_BLOCK_WISE_REQUEST */

#ifdef OC_TCP
  if (cb->endpoint.flags & TCP) {
    coap_tcp_init_message(request, cb->method);
  } else
#endif /* OC_TCP */
  {
    coap_udp_init_message(request, type, cb->method, cb->mid);
  }

  coap_set_header_accept(request, accept);

  coap_set_token(request, cb->token, cb->token_len);

  coap_set_header_uri_path(request, oc_string(cb->uri), oc_string_len(cb->uri));

  if (cb->observe_seq != -1)
    coap_set_header_observe(request, cb->observe_seq);

  if (oc_string_len(cb->query) > 0) {
    coap_set_header_uri_query(request, oc_string(cb->query));
  }

  client_cb = cb;

  return true;
}

static bool
prepare_coap_request(oc_client_cb_t *cb)
{
  return prepare_coap_request_ex(cb, APPLICATION_CBOR);
}

#ifdef OC_OSCORE
bool
oc_do_multicast_update(void)
{
  int payload_size = oc_rep_get_encoded_payload_size();

  if (payload_size > 0 && multicast_update) {
    coap_set_payload(request, multicast_update->data + COAP_MAX_HEADER_SIZE,
                     payload_size);
  } else {
    goto do_multicast_update_error;
  }

  if (payload_size > 0) {
    // still the inner...
    coap_set_header_content_format(request, APPLICATION_CBOR);
  }

  multicast_update->length =
    coap_serialize_message(request, multicast_update->data);
  if (multicast_update->length > 0) {
    oc_send_message(multicast_update);
  } else {
    goto do_multicast_update_error;
  }

#ifdef OC_IPV4
  oc_message_t *multicast_update4 = oc_internal_allocate_outgoing_message();
  if (multicast_update4) {
    oc_make_ipv4_endpoint(mcast4, IPV4 | MULTICAST | SECURED, 5683, 0xe0, 0x00,
                          0x01, 0xbb);

    memcpy(&multicast_update4->endpoint, &mcast4, sizeof(oc_endpoint_t));

    multicast_update4->length = multicast_update->length;
    memcpy(multicast_update4->data, multicast_update->data,
           multicast_update->length);

    oc_send_message(multicast_update4);
  }
#endif /* OC_IPV4 */

  multicast_update = NULL;
  return true;
do_multicast_update_error:
  oc_message_unref(multicast_update);
  multicast_update = NULL;
  return false;
}

bool
oc_init_multicast_update(oc_endpoint_t *mcast, const char *uri,
                         const char *query)
{
  coap_message_type_t type = COAP_TYPE_NON;

  multicast_update = oc_internal_allocate_outgoing_message();

  if (!multicast_update) {
    return false;
  }

  memcpy(&multicast_update->endpoint, mcast, sizeof(oc_endpoint_t));
  oc_rep_new(multicast_update->data + COAP_MAX_HEADER_SIZE, OC_BLOCK_SIZE);
  coap_udp_init_message(request, type, OC_POST, coap_get_mid());
  // still the inner message
  coap_set_header_accept(request, APPLICATION_CBOR);

  request->token_len = 8;
  int i = 0;
  uint32_t r;
  while (i < request->token_len) {
    r = oc_random_value();
    memcpy(request->token + i, &r, sizeof(r));
    i += sizeof(r);
  }

  coap_set_header_uri_path(request, uri, strlen(uri));

  if (query) {
    coap_set_header_uri_query(request, query);
  }

  return true;
}
#endif /* OC_OSCORE */

void
oc_free_server_endpoints(oc_endpoint_t *endpoint)
{
  oc_endpoint_t *next;
  while (endpoint != NULL) {
    next = endpoint->next;
    oc_free_endpoint(endpoint);
    endpoint = next;
  }
}

bool
oc_get_response_payload_raw(oc_client_response_t *response,
                            const uint8_t **payload, size_t *size,
                            oc_content_format_t *content_format)
{
  if (!response || !payload || !size || !content_format) {
    return false;
  }
  if (response->_payload && response->_payload_len > 0) {
    *content_format = response->content_format;
    *payload = response->_payload;
    *size = response->_payload_len;
    return true;
  }
  return false;
}

bool
oc_get_diagnostic_message(oc_client_response_t *response, const char **msg,
                          size_t *size)
{
  oc_content_format_t cf = 0;
  if (oc_get_response_payload_raw(response, (const uint8_t **)msg, size, &cf)) {
    if (cf != TEXT_PLAIN) {
      return false;
    }
    return true;
  }
  return false;
}

bool
oc_do_delete(const char *uri, oc_endpoint_t *endpoint, const char *query,
             oc_response_handler_t handler, oc_qos_t qos, void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_DELETE, query,
                                             client_handler, qos, user_data);

  if (!cb)
    return false;

  bool status = false;

  status = prepare_coap_request(cb);

  if (status)
    status = dispatch_coap_request(APPLICATION_CBOR, APPLICATION_CBOR);

  return status;
}

bool
oc_do_delete_ex(const char *uri, oc_endpoint_t *endpoint, const char *query,
                oc_response_handler_t handler, oc_qos_t qos,
                oc_content_format_t content, oc_content_format_t accept,
                void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_DELETE, query,
                                             client_handler, qos, user_data);

  if (!cb)
    return false;

  bool status = false;

  status = prepare_coap_request(cb);

  if (status)
    status = dispatch_coap_request(content, accept);

  return status;
}

bool
oc_do_get_ex_secured(const char *uri, oc_endpoint_t *endpoint,
                     const char *query, const char *token,
                     oc_response_handler_t handler, oc_qos_t qos,
                     oc_content_format_t content, oc_content_format_t accept,
                     void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  endpoint->flags += OSCORE;
  PRINT("  enable OSCORE encryption\n");

  oc_endpoint_set_oscore_id_from_str(endpoint, (char *)token);

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_GET, query,
                                             client_handler, qos, user_data);
  if (!cb)
    return false;

  bool status = false;

  status = prepare_coap_request(cb);

  if (status)
    status = dispatch_coap_request(content, accept);

  return status;
}

bool
oc_do_get_ex(const char *uri, oc_endpoint_t *endpoint, const char *query,
             oc_response_handler_t handler, oc_qos_t qos,
             oc_content_format_t content, oc_content_format_t accept,
             void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_GET, query,
                                             client_handler, qos, user_data);
  if (!cb)
    return false;

  bool status = false;

  status = prepare_coap_request(cb);

  if (status)
    status = dispatch_coap_request(content, accept);

  return status;
}

bool
oc_do_get(const char *uri, oc_endpoint_t *endpoint, const char *query,
          oc_response_handler_t handler, oc_qos_t qos, void *user_data)
{
  return oc_do_get_ex(uri, endpoint, query, handler, qos, APPLICATION_CBOR,
                      APPLICATION_CBOR, user_data);
}

bool
oc_init_put(const char *uri, oc_endpoint_t *endpoint, const char *query,
            oc_response_handler_t handler, oc_qos_t qos, void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_PUT, query,
                                             client_handler, qos, user_data);
  if (!cb)
    return false;

  return prepare_coap_request(cb);
}

bool
oc_init_post(const char *uri, oc_endpoint_t *endpoint, const char *query,
             oc_response_handler_t handler, oc_qos_t qos, void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_POST, query,
                                             client_handler, qos, user_data);
  if (!cb) {
    return false;
  }

  return prepare_coap_request(cb);
}

bool
oc_do_put(void)
{
  return dispatch_coap_request(APPLICATION_CBOR, APPLICATION_CBOR);
}

bool
oc_do_put_ex(oc_content_format_t content, oc_content_format_t accept)
{
  return dispatch_coap_request(content, accept);
}

bool
oc_do_post(void)
{
  return dispatch_coap_request(APPLICATION_CBOR, APPLICATION_CBOR);
}

bool
oc_do_post_ex(oc_content_format_t content, oc_content_format_t accept)
{
  return dispatch_coap_request(content, accept);
}

bool
oc_do_observe(const char *uri, oc_endpoint_t *endpoint, const char *query,
              oc_response_handler_t handler, oc_qos_t qos, void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_GET, query,
                                             client_handler, qos, user_data);
  if (!cb)
    return false;

  cb->observe_seq = 0;

  bool status = false;

  status = prepare_coap_request(cb);

  if (status)
    status = dispatch_coap_request(APPLICATION_CBOR, APPLICATION_CBOR);

  return status;
}

bool
oc_stop_observe(const char *uri, oc_endpoint_t *endpoint)
{
  oc_client_cb_t *cb = oc_ri_get_client_cb(uri, endpoint, OC_GET);

  if (!cb)
    return false;

  cb->mid = coap_get_mid();
  cb->observe_seq = 1;

  bool status = false;

  status = prepare_coap_request(cb);

  if (status)
    status = dispatch_coap_request(APPLICATION_CBOR, APPLICATION_CBOR);

  return status;
}

#ifdef OC_TCP
oc_event_callback_retval_t
oc_remove_ping_handler(void *data)
{
  oc_client_cb_t *cb = (oc_client_cb_t *)data;

  oc_client_response_t timeout_response;
  timeout_response.code = OC_PING_TIMEOUT;
  timeout_response.endpoint = &cb->endpoint;
  timeout_response.user_data = cb->user_data;
  cb->handler.response(&timeout_response);

  return oc_ri_remove_client_cb(cb);
}

bool
oc_send_ping(bool custody, oc_endpoint_t *endpoint, uint16_t timeout_seconds,
             oc_response_handler_t handler, void *user_data)
{
  oc_client_handler_t client_handler = {
    .response = handler,
    .discovery = NULL,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(
    "/ping", endpoint, 0, NULL, client_handler, LOW_QOS, user_data);
  if (!cb)
    return false;

  if (!coap_send_ping_message(endpoint, custody ? 1 : 0, cb->token,
                              cb->token_len)) {
    oc_ri_remove_client_cb(cb);
    return false;
  }

  oc_set_delayed_callback(cb, oc_remove_ping_handler, timeout_seconds);
  return true;
}
#endif /* OC_TCP */

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

void
oc_stop_multicast(oc_client_response_t *response)
{
  oc_client_cb_t *cb = (oc_client_cb_t *)response->client_cb;
  cb->stop_multicast_receive = true;
}

static bool
dispatch_ip_discovery_ex(oc_client_cb_t *cb4, const char *uri,
                         const char *query, oc_client_handler_t handler,
                         oc_endpoint_t *endpoint, oc_content_format_t accept,
                         oc_content_format_t content, void *user_data)
{
  if (!endpoint) {
    OC_ERR("require valid endpoint");
    return false;
  }

  oc_client_cb_t *cb = oc_ri_alloc_client_cb(uri, endpoint, OC_GET, query,
                                             handler, LOW_QOS, user_data);

  if (cb) {
    cb->discovery = true;
    if (cb4) {
      cb->mid = cb4->mid;
      memcpy(cb->token, cb4->token, cb4->token_len);
    }

    if (prepare_coap_request_ex(cb, accept) &&
        dispatch_coap_request(content, accept)) {
      goto exit;
    }

    if (transaction) {
      coap_clear_transaction(transaction);
      transaction = NULL;
      oc_ri_remove_client_cb(cb);
      client_cb = cb = NULL;
    }

    return false;
  }

exit:

  return true;
}

static bool
multi_scope_ipv6_discovery_wk(oc_client_cb_t *cb4, uint8_t scope,
                              const char *query, oc_client_handler_t handler,
                              void *user_data)
{
  // ALL_COAP_NODES_IPV6_SITE = "FF05::FD"
  PRINT("  multi_scope_ipv6_discovery_wk: %d\n", scope);

  oc_make_ipv6_endpoint(mcast, IPV6 | DISCOVERY, 5683, 0xff, scope, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0xfd);

  // debug info
  oc_string_t mcast_str;
  oc_endpoint_to_string(&mcast, &mcast_str);
  PRINT("   sending to: %s\n", oc_string_checked(mcast_str));
  oc_free_string(&mcast_str);

  PRINT("   query: %s\n", query);

  mcast.addr.ipv6.scope = 0;
  return dispatch_ip_discovery_ex(cb4, ".well-known/core", query, handler,
                                  &mcast, APPLICATION_LINK_FORMAT,
                                  APPLICATION_LINK_FORMAT, user_data);
}

bool
oc_do_wk_discovery(const char *uri_query, oc_discovery_handler_t handler,
                   void *user_data)
{
  oc_client_handler_t handlers = {
    .response = NULL,
    .discovery = handler,
    .discovery_all = NULL,
  };

  oc_client_cb_t *cb4 = NULL;
  // bool status = multi_scope_ipv6_discovery_wk(cb4, 0x02, uri_query,
  //                                         handlers, user_data);

  //  bool status =
  //    multi_scope_ipv6_discovery_wk(cb4, 0x05, uri_query, handlers,
  //    user_data);
  bool status =
    multi_scope_ipv6_discovery_wk(cb4, 0x02, uri_query, handlers, user_data);

  return status;
}

bool
oc_do_wk_discovery_all(const char *uri_query, int scope,
                       oc_discovery_all_handler_t handler, void *user_data)
{
  oc_client_handler_t handlers = {
    .response = NULL,
    .discovery = NULL,
    .discovery_all = handler,
  };

  oc_client_cb_t *cb4 = NULL;

  bool status =
    multi_scope_ipv6_discovery_wk(cb4, scope, uri_query, handlers, user_data);

  return status;
}

// -----------------------------------------------------------------------------

void
oc_close_session(oc_endpoint_t *endpoint)
{
  if (endpoint->flags & SECURED) {
#ifdef OC_OSCORE
    oc_tls_close_connection(endpoint);
#endif /* OC_SECURITY */
  } else if (endpoint->flags & TCP) {
#ifdef OC_TCP
    oc_connectivity_end_session(endpoint);
#endif /* OC_TCP */
  }
}

// -----------------------------------------------------------------------------

int
oc_lf_number_of_entries(const char *payload, int payload_len)
{
  int nr_entries = 0;
  int i;
  if (payload == NULL) {
    return nr_entries;
  }
  if (payload_len < 5) {
    return nr_entries;
  }

  // multiple lines
  for (i = 0; i < payload_len; i++) {
    if (payload[i] == ',') {
      nr_entries++;
    }
  }
  if (nr_entries > 0) {
    // add the last entry, that does not have the continuation character.
    nr_entries++;
  }

  if (nr_entries == 0) {
    // only 1 line
    if (payload[0] == '<') {
      nr_entries = 1;
    }
  }

  return nr_entries;
}

int
oc_lf_get_line(const char *payload, int payload_len, int entry,
               const char **line, int *line_len)
{
  int nr_entries = 0;
  int i;
  if (payload == NULL) {
    return nr_entries;
  }
  if (payload_len < 5) {
    return nr_entries;
  }

  int begin_line_index = 0;
  int end_line_index = 0;
  bool begin_set = false;
  bool end_set = false;

  // find begin
  for (i = 0; i < payload_len - 1; i++) {
    if (entry == nr_entries) {
      if (begin_set == false) {
        begin_line_index = i;
        begin_set = true;
      }
    }
    if (entry + 1 == nr_entries) {
      if (end_set == false) {
        end_line_index = i;
        end_set = true;
      }
    }
    if (payload[i] == ',') {
      nr_entries++;
    }
  }
  if (end_line_index == 0) {
    end_line_index = payload_len;
  }

  if (payload[begin_line_index] == '\n') {
    begin_line_index++;
  }
  // remove the trailing comma, if it exists.
  if (payload[end_line_index - 1] == ',') {
    end_line_index--;
  }
  int line_tot = end_line_index - begin_line_index;

  *line = &payload[begin_line_index];
  *line_len = line_tot;

  return 1;
}

int
oc_lf_get_entry_uri(const char *payload, int payload_len, int entry,
                    const char **uri, int *uri_len)
{
  const char *line = NULL;
  int line_len = 0;
  int i;
  int begin_uri = 0;
  int end_uri = 0;

  oc_lf_get_line(payload, payload_len, entry, &line, &line_len);

  for (i = 0; i < line_len; i++) {
    if (line[i] == '<') {
      begin_uri = i + 1;
    }
    if (line[i] == '>') {
      end_uri = i;
      break;
    }
  }

  *uri = &line[begin_uri];
  *uri_len = end_uri - begin_uri;

  return 1;
}

int
oc_lf_get_entry_param(const char *payload, int payload_len, int entry,
                      const char *param, const char **p_out, int *p_len)
{
  const char *line = NULL;
  int line_len = 0;
  int i;
  int begin_param = 0;
  int end_param = 0;
  int found = 0;

  oc_lf_get_line(payload, payload_len, entry, &line, &line_len);

  // <coap://[fe80::8d4c:632a:c5e7:ae09]:60054/p/a>;rt="urn:knx:dpa.352.51";if=if.a;ct=60
  int param_len = (int)strlen(param);
  for (i = 0; i < line_len - param_len - 1; i++) {
    if (line[i] == ';') {
      if (strncmp(&line[i + 1], param, param_len) == 0) {
        begin_param = i + 1;
        found = 1;
        break;
      }
    }
  }
  if (found == 1) {
    for (i = begin_param + 1; i < line_len - 1; i++) {
      if (line[i] == ';') {
        end_param = i;
        break;
      }
    }
    if (end_param == 0) {
      end_param = line_len;
    }

    // remove the "param=" part from the return value.
    *p_out = &line[begin_param + param_len + 1];
    *p_len = end_param - (begin_param + param_len + 1);

  } else {
    *p_out = line;
    *p_len = line_len;
  }

  return found;
}

#endif /* OC_CLIENT */
