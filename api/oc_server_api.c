/*
// Copyright (c) 2016 Intel Corporation
// Copyright (c) 2022 Cascoda Ltd.
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

#include "messaging/coap/engine.h"
#include "messaging/coap/oc_coap.h"
#include "messaging/coap/separate.h"
#include "oc_api.h"

//#ifdef OC_SECURITY
//#include "security/oc_store.h"
//#endif /* OC_SECURITY */

#ifdef OC_DYNAMIC_ALLOCATION
#include <stdlib.h>
#endif /* OC_DYNAMIC_ALLOCATION */

#include "oc_core_res.h"

static size_t query_iterator;

int
oc_add_device(const char *name, const char *version, const char *base,
              const char *serialnumber, oc_add_device_cb_t add_device_cb,
              void *data)
{
  if (!oc_core_add_device(name, version, base, serialnumber, add_device_cb,
                          data))
    return -1;

  return 0;
}

int
oc_init_platform(const char *mfg_name, oc_init_platform_cb_t init_platform_cb,
                 void *data)
{
  if (!oc_core_init_platform(mfg_name, init_platform_cb, data))
    return -1;
  return 0;
}

int
oc_get_query_value(oc_request_t *request, const char *key, char **value)
{
  if (!request)
    return -1;
  return oc_ri_get_query_value(request->query, request->query_len, key, value);
}

int
oc_query_value_exists(oc_request_t *request, const char *key)
{
  if (!request)
    return -1;
  return oc_ri_query_exists(request->query, request->query_len, key);
}

bool
oc_query_values_available(oc_request_t *request)
{
  if (!request)
    return false;
  if (request->query_len > 0) {
    return true;
  }
  return false;
}

static int
response_length(void)
{
  // int size = oc_rep_get_encoded_payload_size();
  // return (size <= 2) ? 0 : size;
  return oc_rep_get_encoded_payload_size();
}

void
oc_send_response(oc_request_t *request, oc_status_t response_code)
{
  if (request && request->response && request->response->response_buffer) {
    request->response->response_buffer->content_format = APPLICATION_CBOR;
    request->response->response_buffer->response_length = response_length();
    request->response->response_buffer->code = oc_status_code(response_code);
  }
}

void
oc_send_cbor_response(oc_request_t *request, oc_status_t response_code)
{
  if (request && request->response && request->response->response_buffer) {
    request->response->response_buffer->content_format = APPLICATION_CBOR;
    if ((response_code == OC_STATUS_OK) ||
        (response_code == OC_STATUS_CHANGED)) {
      request->response->response_buffer->response_length = response_length();
    } else {
      request->response->response_buffer->response_length = 0;
    }

    request->response->response_buffer->code = oc_status_code(response_code);
  }
}

void
oc_send_cbor_response_no_payload_size(oc_request_t *request,
                                      oc_status_t response_code)
{
  oc_send_cbor_response_with_payload_size(request, response_code, 0);
}

void
oc_send_cbor_response_with_payload_size(oc_request_t *request,
                                        oc_status_t response_code,
                                        size_t payload_size)
{
  if (request && request->response && request->response->response_buffer) {
    request->response->response_buffer->content_format = APPLICATION_CBOR;
    request->response->response_buffer->response_length = payload_size;
    request->response->response_buffer->code = oc_status_code(response_code);
  }
}

void
oc_send_json_response(oc_request_t *request, oc_status_t response_code)
{
  request->response->response_buffer->content_format = APPLICATION_JSON;
  request->response->response_buffer->response_length = response_length();
  request->response->response_buffer->code = oc_status_code(response_code);
}

void
oc_send_linkformat_response(oc_request_t *request, oc_status_t response_code,
                            size_t response_length)
{
  request->response->response_buffer->content_format = APPLICATION_LINK_FORMAT;
  request->response->response_buffer->response_length = response_length;
  request->response->response_buffer->code = oc_status_code(response_code);
}

void
oc_ignore_request(oc_request_t *request)
{
  request->response->response_buffer->code = OC_IGNORE;
}

void
oc_set_delayed_callback(void *cb_data, oc_trigger_t callback, uint16_t seconds)
{
  oc_ri_add_timed_event_callback_seconds(cb_data, callback, seconds);
}

void
oc_set_delayed_callback_ms(void *cb_data, oc_trigger_t callback,
                           uint16_t miliseconds)
{
  oc_ri_add_timed_event_callback_ticks(cb_data, callback, miliseconds);
}

void
oc_remove_delayed_callback(void *cb_data, oc_trigger_t callback)
{
  oc_ri_remove_timed_event_callback(cb_data, callback);
}

void
oc_init_query_iterator(void)
{
  query_iterator = 0;
}

int
oc_iterate_query(oc_request_t *request, char **key, size_t *key_len,
                 char **value, size_t *value_len)
{
  query_iterator++;
  return oc_ri_get_query_nth_key_value(request->query, request->query_len, key,
                                       key_len, value, value_len,
                                       query_iterator);
}

bool
oc_iterate_query_get_values(oc_request_t *request, const char *key,
                            char **value, int *value_len)
{
  char *current_key = 0;
  size_t key_len = 0, v_len;
  int pos = 0;

  do {
    pos = oc_iterate_query(request, &current_key, &key_len, value, &v_len);
    *value_len = (int)v_len;
    if (pos != -1 && strlen(key) == key_len &&
        memcmp(key, current_key, key_len) == 0) {
      goto more_or_done;
    }
  } while (pos != -1);

  *value_len = -1;

more_or_done:
  if (pos == -1 || (size_t)pos >= request->query_len) {
    return false;
  }
  return true;
}

#ifdef OC_SERVER

bool
oc_get_request_payload_raw(oc_request_t *request, const uint8_t **payload,
                           size_t *size, oc_content_format_t *content_format)
{
  if (!request || !payload || !size || !content_format) {
    return false;
  }
  if (request->_payload && request->_payload_len > 0) {
    *content_format = request->content_format;
    *payload = request->_payload;
    *size = request->_payload_len;
    return true;
  }
  return false;
}

void
oc_send_response_raw(oc_request_t *request, const uint8_t *payload, size_t size,
                     oc_content_format_t content_format,
                     oc_status_t response_code)
{
  request->response->response_buffer->content_format = content_format;
  memcpy(request->response->response_buffer->buffer, payload, size);
  request->response->response_buffer->response_length = size;
  request->response->response_buffer->code = oc_status_code(response_code);
}

void
oc_send_diagnostic_message(oc_request_t *request, const char *msg,
                           size_t msg_len, oc_status_t response_code)
{
  oc_send_response_raw(request, (const uint8_t *)msg, msg_len, TEXT_PLAIN,
                       response_code);
}

static void
oc_populate_resource_object(oc_resource_t *resource, const char *name,
                            const char *uri, uint8_t num_resource_types,
                            size_t device)
{
  if (name) {
    resource->name.ptr = name;
    resource->name.size = strlen(name) + 1;
    resource->name.next = NULL;
  }
  oc_check_uri(uri);
  resource->uri.next = NULL;
  resource->uri.ptr = uri;
  resource->uri.size = strlen(uri) + 1; // include null terminator in size
  oc_new_string_array(&resource->types, num_resource_types);
  resource->properties = 0;
  resource->device = device;

#ifdef OC_OSCORE
  resource->properties |= OC_SECURE;
#endif /* OC_OSCORE */
}

oc_resource_t *
oc_new_resource(const char *name, const char *uri, uint8_t num_resource_types,
                size_t device_index)
{
  oc_resource_t *resource = NULL;
  if (strlen(uri) < OC_MAX_URL_LENGTH) {

    resource = oc_ri_alloc_resource();
    if (resource) {
      resource->interfaces = OC_IF_NONE;
      resource->observe_period_seconds = 0;
      resource->num_observers = 0;
      resource->properties = OC_DISCOVERABLE;
      oc_populate_resource_object(resource, name, uri, num_resource_types,
                                  device_index);
    }
  } else {
    OC_ERR(" resource uri longer than 30 bytes: %d", (int)strlen(uri));
  }

  return resource;
}

void
oc_resource_bind_resource_interface(oc_resource_t *resource,
                                    oc_interface_mask_t iface_mask)
{
  if (resource == NULL) {
    OC_ERR("oc_resource_bind_resource_interface: resource is NULL");
    return;
  }

  resource->interfaces |= iface_mask;
}

void
oc_resource_bind_resource_type(oc_resource_t *resource, const char *type)
{
  if (resource) {
    oc_string_array_add_item(resource->types, (char *)type);
  } else {
    OC_ERR("oc_resource_bind_resource_type: resource is NULL");
  }
}

void
oc_resource_bind_dpt(oc_resource_t *resource, const char *dpt)
{
  if (resource) {
    oc_free_string(&resource->dpt);
    memset(&resource->dpt, 0, sizeof(oc_string_t));
    if (dpt) {
      oc_new_string(&resource->dpt, dpt, strlen(dpt));
    }
  } else {
    OC_ERR("oc_resource_bind_dpt: resource is NULL");
  }
}

void
oc_resource_bind_content_type(oc_resource_t *resource,
                              oc_content_format_t content_type)
{
  if (resource) {
    resource->content_type = content_type;
  } else {
    OC_ERR("oc_resource_bind_content_type: resource is NULL");
  }
}

#ifdef OC_SECURITY
void
oc_resource_make_public(oc_resource_t *resource)
{
  resource->properties &= ~OC_SECURE;
}
#endif /* OC_SECURITY */

void
oc_resource_set_discoverable(oc_resource_t *resource, bool state)
{
  if (resource == NULL) {
    OC_ERR("oc_resource_set_discoverable: resource is NULL");
    return;
  }

  if (state)
    resource->properties |= OC_DISCOVERABLE;
  else
    resource->properties &= ~OC_DISCOVERABLE;
}

void
oc_resource_set_observable(oc_resource_t *resource, bool state)
{
  if (resource == NULL) {
    OC_ERR("oc_resource_set_observable: resource is NULL");
    return;
  }

  if (state)
    resource->properties |= OC_OBSERVABLE;
  else
    resource->properties &= ~(OC_OBSERVABLE | OC_PERIODIC);
}

void
oc_resource_set_periodic_observable(oc_resource_t *resource, uint16_t seconds)
{
  if (resource == NULL) {
    OC_ERR("oc_resource_set_periodic_observable: resource is NULL");
    return;
  }

  resource->properties |= OC_OBSERVABLE | OC_PERIODIC;
  resource->observe_period_seconds = seconds;
}

void
oc_resource_set_function_block_instance(oc_resource_t *resource,
                                        uint8_t instance)
{
  if (resource == NULL) {
    OC_ERR("oc_resource_set_function_block_instance: resource is NULL");
    return;
  }

  resource->fb_instance = instance;
}

void
oc_resource_set_properties_cbs(oc_resource_t *resource,
                               oc_get_properties_cb_t get_properties,
                               void *get_props_user_data,
                               oc_set_properties_cb_t set_properties,
                               void *set_props_user_data)
{
  if (resource == NULL) {
    OC_ERR("oc_resource_set_properties_cbs: resource is NULL");
    return;
  }

  resource->get_properties.cb.get_props = get_properties;
  resource->get_properties.user_data = get_props_user_data;
  resource->set_properties.cb.set_props = set_properties;
  resource->set_properties.user_data = set_props_user_data;
}

void
oc_resource_set_request_handler(oc_resource_t *resource, oc_method_t method,
                                oc_request_callback_t callback, void *user_data)
{
  oc_request_handler_t *handler = NULL;

  if (resource == NULL) {
    OC_ERR("oc_resource_set_request_handler: resource is NULL");
    return;
  }

  switch (method) {
  case OC_GET:
    handler = &resource->get_handler;
    break;
  case OC_POST:
    handler = &resource->post_handler;
    break;
  case OC_PUT:
    handler = &resource->put_handler;
    break;
  case OC_DELETE:
    handler = &resource->delete_handler;
    break;
  default:
    break;
  }

  if (handler) {
    handler->cb = callback;
    handler->user_data = user_data;
  }
}

bool
oc_add_resource(oc_resource_t *resource)
{
  return oc_ri_add_resource(resource);
}

bool
oc_delete_resource(oc_resource_t *resource)
{
  return oc_ri_delete_resource(resource);
}

static oc_event_callback_retval_t
oc_delayed_delete_resource_cb(void *data)
{
  oc_resource_t *resource = (oc_resource_t *)data;
  oc_delete_resource(resource);
  return OC_EVENT_DONE;
}

void
oc_delayed_delete_resource(oc_resource_t *resource)
{
  oc_set_delayed_callback(resource, oc_delayed_delete_resource_cb, 0);
}

void
oc_indicate_separate_response(oc_request_t *request,
                              oc_separate_response_t *response)
{
  request->response->separate_response = response;
  oc_send_response(request, OC_STATUS_OK);
}

void
oc_set_separate_response_buffer(oc_separate_response_t *handle)
{
  coap_separate_t *cur = oc_list_head(handle->requests);
  handle->response_state = oc_blockwise_alloc_response_buffer(
    oc_string(cur->uri), oc_string_len(cur->uri), &cur->endpoint, cur->method,
    OC_BLOCKWISE_SERVER);
#ifdef OC_BLOCK_WISE
  oc_rep_new(handle->response_state->buffer, OC_MAX_APP_DATA_SIZE);
#else  /* OC_BLOCK_WISE */
  oc_rep_new(handle->buffer, OC_BLOCK_SIZE);
#endif /* !OC_BLOCK_WISE */
}

static void
oc_send_separate_response_with_length(oc_separate_response_t *handle,
                                      oc_status_t response_code, size_t length)
{
  oc_response_buffer_t response_buffer;
  response_buffer.buffer = handle->response_state->buffer;
  response_buffer.response_length = length;
  response_buffer.code = oc_status_code(response_code);
  response_buffer.content_format = APPLICATION_CBOR;

  coap_separate_t *cur = oc_list_head(handle->requests), *next = NULL;
  coap_packet_t response[1];

  while (cur != NULL) {
    next = cur->next;
    if (cur->observe < 3) {
      coap_transaction_t *t = coap_new_transaction(
        coap_get_mid(), cur->token, cur->token_len, &cur->endpoint);
      if (t) {
        coap_separate_resume(response, cur,
                             (uint8_t)oc_status_code(response_code), t->mid);
        coap_set_header_content_format(response,
                                       response_buffer.content_format);

#ifdef OC_BLOCK_WISE
        oc_blockwise_state_t *response_state = NULL;
#ifdef OC_TCP
        if (!(cur->endpoint.flags & TCP) &&
            response_buffer.response_length > cur->block2_size) {
#else  /* OC_TCP */
        if (response_buffer.response_length > cur->block2_size) {
#endif /* !OC_TCP */
          response_state = oc_blockwise_find_response_buffer(
            oc_string(cur->uri), oc_string_len(cur->uri), &cur->endpoint,
            cur->method, NULL, 0, OC_BLOCKWISE_SERVER);
          if (response_state) {
            if (response_state->payload_size ==
                response_state->next_block_offset) {
              oc_blockwise_free_response_buffer(response_state);
              response_state = NULL;
            } else {
              goto next_separate_request;
            }
          }
          response_state = oc_blockwise_alloc_response_buffer(
            oc_string(cur->uri), oc_string_len(cur->uri), &cur->endpoint,
            cur->method, OC_BLOCKWISE_SERVER);
          if (!response_state) {
            goto next_separate_request;
          }

          memcpy(response_state->buffer, response_buffer.buffer,
                 response_buffer.response_length);
          response_state->payload_size =
            (uint32_t)response_buffer.response_length;

          uint32_t payload_size = 0;
          const void *payload = oc_blockwise_dispatch_block(
            response_state, 0, cur->block2_size, &payload_size);
          if (payload) {
            coap_set_payload(response, payload, payload_size);
            coap_set_header_block2(response, 0, 1, cur->block2_size);
            coap_set_header_size2(response, response_state->payload_size);
            oc_blockwise_response_state_t *bwt_res_state =
              (oc_blockwise_response_state_t *)response_state;
            coap_set_header_etag(response, bwt_res_state->etag, COAP_ETAG_LEN);
          }
        } else
#endif /* OC_BLOCK_WISE */
          if (response_buffer.response_length > 0) {
            coap_set_payload(response, handle->response_state->buffer,
                             response_buffer.response_length);
          }
        coap_set_status_code(response, response_buffer.code);
        t->message->length = coap_serialize_message(response, t->message->data);
        if (t->message->length > 0) {
          coap_send_transaction(t);
        } else {
          coap_clear_transaction(t);
        }
      }
    } else {
      oc_resource_t *resource = oc_ri_get_app_resource_by_uri(
        oc_string(cur->uri), oc_string_len(cur->uri), cur->endpoint.device);
      if (resource) {
        coap_notify_observers(resource, &response_buffer, &cur->endpoint);
      }
    }
#ifdef OC_BLOCK_WISE
  next_separate_request:
#endif /* OC_BLOCK_WISE */
    coap_separate_clear(handle, cur);
    cur = next;
  }
  handle->active = 0;
  oc_blockwise_free_response_buffer(handle->response_state);
}

void
oc_send_separate_response(oc_separate_response_t *handle,
                          oc_status_t response_code)
{
  size_t length;
  if (handle->response_state->payload_size != 0)
    length = handle->response_state->payload_size;
  else
    length = response_length();

  oc_send_separate_response_with_length(handle, response_code, length);
}

void
oc_send_empty_separate_response(oc_separate_response_t *handle,
                                oc_status_t response_code)
{
  oc_send_separate_response_with_length(handle, response_code, 0);
}

int
oc_notify_observers(oc_resource_t *resource)
{
  return coap_notify_observers(resource, NULL, NULL);
}
#endif /* OC_SERVER */
