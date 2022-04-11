/*
// Copyright (c) 2016 Intel Corporation
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

#include "oc_helpers.h"
#include "port/oc_assert.h"
#include "port/oc_log.h"
#include <stdbool.h>
#include <stdlib.h>

static bool mmem_initialized = false;

static void
oc_malloc(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_handle_t *block, size_t num_items, pool pool_type)
{
  if (!mmem_initialized) {
    oc_mmem_init();
    mmem_initialized = true;
  }
  size_t alloc_ret = _oc_mmem_alloc(
#ifdef OC_MEMORY_TRACE
    func,
#endif
    block, num_items, pool_type);
  // oc_assert(alloc_ret > 0);
}

static void
oc_free(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_handle_t *block, pool pool_type)
{
  _oc_mmem_free(
#ifdef OC_MEMORY_TRACE
    func,
#endif
    block, pool_type);

  block->next = 0;
  block->ptr = 0;
  block->size = 0;
}

void
_oc_new_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring, const char *str, size_t str_len)
{
  oc_malloc(
#ifdef OC_MEMORY_TRACE
    func,
#endif
    ocstring, str_len + 1, BYTE_POOL);
  memcpy(oc_string(*ocstring), (const uint8_t *)str, str_len);
  memcpy(oc_string(*ocstring) + str_len, (const uint8_t *)"", 1);
}

void
_oc_alloc_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring, size_t size)
{
  oc_malloc(
#ifdef OC_MEMORY_TRACE
    func,
#endif
    ocstring, size, BYTE_POOL);
}

void
_oc_free_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring)
{
  if (ocstring && ocstring->size > 0) {
    oc_free(
#ifdef OC_MEMORY_TRACE
      func,
#endif
      ocstring, BYTE_POOL);
  }
}

void
oc_concat_strings(oc_string_t *concat, const char *str1, const char *str2)
{
  size_t len1 = strlen(str1), len2 = strlen(str2);
  oc_alloc_string(concat, len1 + len2 + 1);
  memcpy(oc_string(*concat), str1, len1);
  memcpy(oc_string(*concat) + len1, str2, len2);
  memcpy(oc_string(*concat) + len1 + len2, (const char *)"", 1);
}

void
_oc_new_array(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_array_t *ocarray, size_t size, pool type)
{
  switch (type) {
  case INT_POOL:
  case BYTE_POOL:
  case DOUBLE_POOL:
    oc_malloc(
#ifdef OC_MEMORY_TRACE
      func,
#endif
      ocarray, size, type);
    break;
  default:
    break;
  }
}

void
_oc_free_array(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_array_t *ocarray, pool type)
{
  oc_free(
#ifdef OC_MEMORY_TRACE
    func,
#endif
    ocarray, type);
}

void
_oc_alloc_string_array(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_array_t *ocstringarray, size_t size)
{
  _oc_alloc_string(
#ifdef OC_MEMORY_TRACE
    func,
#endif
    ocstringarray, size * STRING_ARRAY_ITEM_MAX_LEN);

  size_t i, pos;
  for (i = 0; i < size; i++) {
    pos = i * STRING_ARRAY_ITEM_MAX_LEN;
    memcpy((char *)oc_string(*ocstringarray) + pos, (const char *)"", 1);
  }
  ocstringarray->size = size * STRING_ARRAY_ITEM_MAX_LEN;
}

bool
_oc_copy_byte_string_to_array(oc_string_array_t *ocstringarray,
                              const char str[], size_t str_len, size_t index)
{
  if (strlen(str) >= STRING_ARRAY_ITEM_MAX_LEN) {
    return false;
  }
  size_t pos = index * STRING_ARRAY_ITEM_MAX_LEN;
  oc_string(*ocstringarray)[pos] = (uint8_t)str_len;
  pos++;
  memcpy(oc_string(*ocstringarray) + pos, (const uint8_t *)str, str_len);
  return true;
}

bool
_oc_byte_string_array_add_item(oc_string_array_t *ocstringarray,
                               const char str[], size_t str_len)
{
  bool success = false;
  size_t i;
  for (i = 0; i < oc_byte_string_array_get_allocated_size(*ocstringarray);
       i++) {
    if (oc_byte_string_array_get_item_size(*ocstringarray, i) == 0) {
      success = oc_byte_string_array_set_item(*ocstringarray, str, str_len, i);
      break;
    }
  }
  return success;
}

bool
_oc_copy_string_to_array(oc_string_array_t *ocstringarray, const char str[],
                         size_t index)
{
  if (strlen(str) >= STRING_ARRAY_ITEM_MAX_LEN) {
    return false;
  }
  size_t pos = index * STRING_ARRAY_ITEM_MAX_LEN;
  size_t len = strlen(str);
  memcpy(oc_string(*ocstringarray) + pos, (const uint8_t *)str, len);
  memcpy(oc_string(*ocstringarray) + pos + len, (const uint8_t *)"", 1);
  return true;
}

bool
_oc_string_array_add_item(oc_string_array_t *ocstringarray, const char str[])
{
  bool success = false;
  size_t i;
  if (ocstringarray == NULL) {
    return false;
  }
  for (i = 0; i < oc_string_array_get_allocated_size(*ocstringarray); i++) {
    if (oc_string_array_get_item_size(*ocstringarray, i) == 0) {
      success = oc_string_array_set_item(*ocstringarray, str, i);
      break;
    }
  }
  return success;
}

void
oc_join_string_array(oc_string_array_t *ocstringarray, oc_string_t *ocstring)
{
  size_t len = 0;
  size_t i;
  for (i = 0; i < oc_string_array_get_allocated_size(*ocstringarray); i++) {
    const char *item =
      (const char *)oc_string_array_get_item(*ocstringarray, i);
    if (strlen(item)) {
      len += strlen(item);
      len++;
    }
  }
  oc_alloc_string(ocstring, len);
  len = 0;
  for (i = 0; i < oc_string_array_get_allocated_size(*ocstringarray); i++) {
    const char *item =
      (const char *)oc_string_array_get_item(*ocstringarray, i);
    if (strlen(item)) {
      if (len > 0) {
        oc_string(*ocstring)[len] = ' ';
        len++;
      }
      memcpy((char *)oc_string(*ocstring) + len, item, strlen(item));
      len += strlen(item);
    }
  }
  strcpy((char *)oc_string(*ocstring) + len, "");
}

int
oc_conv_byte_array_to_hex_string(const uint8_t *array, size_t array_len,
                                 char *hex_str, size_t *hex_str_len)
{
  if (*hex_str_len < array_len * 2 + 1) {
    return -1;
  }

  *hex_str_len = 0;

  size_t i;

  for (i = 0; i < array_len; i++) {
    snprintf(hex_str + *hex_str_len, 3, "%02x", array[i]);
    *hex_str_len += 2;
  }

  hex_str[*hex_str_len++] = '\0';

  return 0;
}

int
oc_conv_hex_string_to_byte_array(const char *hex_str, size_t hex_str_len,
                                 uint8_t *array, size_t *array_len)
{
  if (hex_str_len < 1) {
    return -1;
  }

  size_t a = hex_str_len / 2.0 + 0.5;

  if (*array_len < a) {
    return -1;
  }

  *array_len = a;
  a = 0;

  uint32_t tmp;
  size_t i, start;

  if (hex_str_len % 2 == 0) {
    start = 0;
  } else {
    start = 1;
    sscanf(&hex_str[0], "%1x", &tmp);
    array[a++] = (uint8_t)tmp;
  }

  for (i = start; i <= hex_str_len - 2; i += 2) {
    sscanf(&hex_str[i], "%2x", &tmp);
    array[a++] = (uint8_t)tmp;
  }

  return 0;
}

int oc_string_copy(oc_string_t* string1, oc_string_t string2)
{
  oc_free_string(string1);
  oc_new_string(string1, oc_string(string2), oc_string_len(string2));
  return 0;
}

int
oc_string_copy_from_char(oc_string_t* string1, char* string2)
{
  oc_free_string(string1);
  oc_new_string(string1, string2, strlen(string2));
  return 0;
}


int
oc_string_cmp(oc_string_t string1, oc_string_t string2)
{
  if (oc_string_len(string1) != oc_string_len(string2)) {
    return -1;
  }
  return strncmp(oc_string(string1), oc_string(string2),
                 oc_string_len(string1));
}

int
oc_url_cmp(oc_string_t string1, oc_string_t string2)
{
  char *str1 = oc_string(string1);
  char *str2 = oc_string(string2);
  char *cmp1 = str1;
  char *cmp2 = str2;

  if ((strlen(str1) > 1) && (str1[0] == '/')) {
    /* skip the leading / */
    cmp1 = &str1[1];
  }

  if ((strlen(str2) > 1) && (str2[0] == '/')) {
    /* skip the leading / */
    cmp2 = &str2[1];
  }

  return strncmp(cmp1, cmp2, strlen(cmp1));
}

bool
oc_uri_contains_wildcard(const char *uri)
{
  if (uri == NULL)
    return false;

  size_t len = strlen(uri);
  if (uri[len - 1] == '*') {
    return true;
  }
  return false;
}

int
oc_uri_get_wildcard_value_as_int(const char *uri_resource, size_t uri_len,
                                 const char *uri_invoked, size_t invoked_len)
{
  if (uri_resource[uri_len - 1] == '*') {
    if ((invoked_len + 1) >= uri_len) {
      int value = atoi(&uri_invoked[uri_len - 2]);
      return value;
    }
  }

  return -1;
}

bool
oc_uri_contains_wildcard_value_underscore(const char *uri_resource,
                                          size_t uri_len,
                                          const char *uri_invoked,
                                          size_t invoked_len)
{
  if (uri_resource[uri_len - 1] == '*') {
    if ((invoked_len + 1) >= uri_len) {
      char *underscore = strchr(&uri_invoked[uri_len - 2], '_');
      if (underscore) {
        int value = atoi(underscore + 1);
        return value;
      }
    }
  }

  return false;
}

int
oc_uri_get_wildcard_value_as_int_after_underscore(const char *uri_resource,
                                                  size_t uri_len,
                                                  const char *uri_invoked,
                                                  size_t invoked_len)
{
  if (uri_resource[uri_len - 1] == '*') {
    if ((invoked_len + 1) >= uri_len) {
      if (strchr(&uri_invoked[uri_len - 2], '_')) {
        return true;
      }
    }
  }
  return -1;
}

int
oc_uri_get_wildcard_value_as_string(const char *uri_resource, size_t uri_len,
                                    const char *uri_invoked, size_t invoked_len,
                                    const char **value)
{
  if (uri_resource[uri_len - 1] == '*') {
    if ((invoked_len + 1) >= uri_len) {
      *value = &uri_invoked[uri_len - 2];
      size_t len = invoked_len - uri_len + 2;
      return (int)len;
    }
  }

  return -1;
}
