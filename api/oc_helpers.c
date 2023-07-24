/*
// Copyright (c) 2016 Intel Corporation
// Copyright (c) 2022,2023 Cascoda Ltd.
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
#include <ctype.h>
#include <inttypes.h>

static bool mmem_initialized = false;

#ifndef MAX
#define MAX(n, m) (((n) < (m)) ? (m) : (n))
#endif

#ifndef MIN
#define MIN(n, m) (((n) < (m)) ? (n) : (m))
#endif

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
_oc_new_byte_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring, const char *str, size_t str_len)
{
  oc_malloc(
#ifdef OC_MEMORY_TRACE
    func,
#endif
    ocstring, str_len, BYTE_POOL);
  memcpy(oc_string(*ocstring), (const uint8_t *)str, str_len);
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
  case FLOAT_POOL:
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
    oc_assert(false);
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
oc_conv_uint64_to_dec_string(char *str, uint64_t number)
{
  if (number == 0) {
    snprintf(str, 2, "0");
    return 0;
  }

  // Determine the length of the string representation
  uint64_t temp = number;
  int numDigits = 0; // Note: This needs to be an int to prevent underflow

  while (temp != 0) {
    temp /= 10;
    numDigits++;
  }

  // Convert the number to a string
  int i; // int to prevent underflow!!
  for (i = numDigits - 1; i >= 0; i--) {
    str[i] = '0' + (number % 10);
    number /= 10;
  }
  str[numDigits] = '\0';

  return 0;
}

int
oc_print_uint64_t(uint64_t number, enum StringRepresentation rep)
{
  char str[21]; // uint64_t decimal number has max 20 numbers + 1 for null
                // terminator

  if (rep == DEC_REPRESENTATION)
    oc_conv_uint64_to_dec_string(str, number);
  else
    oc_conv_uint64_to_hex_string(str, number);

  printf("%s", str);
}

int
oc_conv_uint64_to_hex_string(char *str, uint64_t number)
{
  char temp_str[17];

  if (number == 0) {
    snprintf(str, 2, "0");
    return 0;
  }

  // Convert to hex string, but will include leading zeros
  for (uint8_t i = 0; i < 16; ++i) {
    uint8_t nibble = (number >> ((16 - (i + 1)) * 4));
    sprintf(temp_str + i, "%x", nibble & 0xF);
  }
  temp_str[16] = '\0';

  // Remove leading zeros
  uint8_t leading_zeros;
  for (leading_zeros = 0; leading_zeros < 16; ++leading_zeros) {
    if (temp_str[leading_zeros] != '0')
      break;
  }
  strcpy(str, temp_str + leading_zeros);

  return 0;
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

  size_t a = (size_t)((double)hex_str_len / 2.0 + 0.5);

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
    int processed_fields = sscanf(&hex_str[0], "%1x", &tmp);
    if (processed_fields != 1) {
      return -1;
    }
    array[a++] = (uint8_t)tmp;
  }

  if (hex_str_len >= 2) {
    // save guard against string lengths of 1
    for (i = start; i <= hex_str_len - 2; i += 2) {
      int processed_fields = sscanf(&hex_str[i], "%2x", &tmp);
      if (processed_fields != 1) {
        return -1;
      }
      array[a++] = (uint8_t)tmp;
    }
  }

  return 0;
}

int
oc_conv_hex_string_to_oc_string(const char *hex_str, size_t hex_str_len,
                                oc_string_t *out)
{
  int return_value = -1;
  size_t size_bytes = (hex_str_len / 2);

  PRINT("oc_conv_hex_string_to_oc_string len:%d -> bytes:%d\n",
        (int)hex_str_len, (int)size_bytes);

  oc_free_string(out);

  PRINT("oc_conv_hex_string_to_oc_string free string\n");
  oc_alloc_string(out, size_bytes);
  PRINT("oc_conv_hex_string_to_oc_string alloc string\n");
  char *ptr = oc_string(*out);
  PRINT("oc_conv_hex_string_to_oc_string ptr\n");
  if (ptr != NULL) {
    return_value =
      oc_conv_hex_string_to_byte_array(hex_str, hex_str_len, ptr, &size_bytes);
  }
  PRINT("oc_conv_hex_string_to_oc_string result=%d\n", (int)return_value);
  return return_value;
}

int
oc_string_is_hex_array(oc_string_t hex_string)
{
  char *array = oc_string(hex_string);
  int array_len = strlen(array);
  for (int i = 0; i < array_len; i++) {
    if (isxdigit(array[i]) == false) {
      return -1;
    }
  }
  return 0;
}

int
oc_char_print_hex(const char *str, int str_len)
{
  for (int i = 0; i < str_len; i++) {
    PRINT("%02x", (unsigned char)str[i]);
  }
  return str_len;
}

int
oc_string_print_hex(oc_string_t hex_string)
{
  char *str = oc_string(hex_string);
  int length = oc_byte_string_len(hex_string);
  return oc_char_print_hex(str, length);
}

int
oc_string_println_hex(oc_string_t hex_string)
{
  int retval = oc_string_print_hex(hex_string);
  PRINT("\n");
  return retval;
}

int
oc_char_println_hex(const char *str, int str_len)
{
  int retval;
  retval = oc_char_print_hex(str, str_len);
  PRINT("\n");
  return retval;
}

int
oc_string_copy(oc_string_t *string1, oc_string_t string2)
{
  oc_free_string(string1);
  oc_new_string(string1, oc_string(string2), oc_string_len(string2));
  return 0;
}

int
oc_string_copy_from_char(oc_string_t *string1, const char *string2)
{
  oc_free_string(string1);
  oc_new_string(string1, string2, strlen(string2));
  return 0;
}

int
oc_string_copy_from_char_with_size(oc_string_t *string1, const char *string2,
                                   size_t string2_len)
{
  oc_free_string(string1);
  oc_new_string(string1, string2, string2_len);
  return 0;
}

int
oc_byte_string_copy_from_char_with_size(oc_string_t *string1,
                                        const char *string2, size_t string2_len)
{
  oc_free_string(string1);
  oc_new_byte_string(string1, string2, string2_len);
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

int
oc_uri_get_wildcard_value_as_int_after_underscore(const char *uri_resource,
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

bool
oc_uri_contains_wildcard_value_underscore(const char *uri_resource,
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

char *
oc_strnchr(const char *string, char p, int size)
{
  int i;
  for (i = 0; i < size; i++) {
    if (string[i] == p) {
      return (char *)&string[i];
    }
  }
  return NULL;
}

int
oc_char_convert_to_lower(char *str)
{
  for (; *str; ++str)
    *str = tolower(*str);
  return 0;
}

int
oc_get_sn_from_ep(const char *param, int param_len, char *sn, int sn_len,
                  uint32_t *ia)
{
  int error = -1;
  memset(sn, 0, 30);
  *ia = 0;
  if (param_len < 10) {
    return error;
  }
  if (strncmp(param, "\"knx://sn.", 10) == 0) {
    // spec 1.1 ep= contents: (with quote)
    // "knx://sn.<sn> knx://ia.<ia>"
    char *blank = oc_strnchr(param, ' ', param_len);
    if (blank == NULL) {
      // the ia part is missing, so length -10 and 1 less to adjust for quot
      strncpy(sn, (char *)&param[10], param_len - 11);
    } else {
      int offset = blank - param;
      int len = offset - 10;
      strncpy(sn, &param[10], len);
      if (strncmp(&param[offset + 1], "knx://ia.", 9) == 0) {
        // read from hex
        *ia = (uint32_t)strtol(&param[offset + 1 + 9], NULL, 16);
        error = 0;
      }
    }
  } else if (strncmp(param, "knx://sn.", 9) == 0) {
    // spec 1.1 ep= contents: (without quote)
    // knx://sn.<sn> knx://ia.<ia>"
    char *blank = oc_strnchr(param, ' ', param_len);
    if (blank == NULL) {
      // the ia part is missing, so length -10 and 1 less to adjust for quot
      strncpy(sn, (char *)&param[9], param_len - 11);
    } else {
      int offset = blank - param;
      int len = offset - 9;
      strncpy(sn, &param[9], len);
      if (strncmp(&param[offset + 1], "knx://ia.", 9) == 0) {
        // read from hex
        *ia = (uint32_t)strtol(&param[offset + 1 + 9], NULL, 16);
        error = 0;
      }
    }
  } else if (strncmp(param, "\"knx://ia.", 10) == 0) {
    // spec 1.1 ep= contents:
    // "knx://ia.<sn> knx://sn.<ia>"
    char *blank = oc_strnchr(param, ' ', param_len);
    if (blank == NULL) {
      // the sn part is missing
      PRINT("oc_get_sn_from_ep 222 string: string ia : '%s'\n", &param[10]);
      // read from hex
      *ia = (uint32_t)strtol(&param[10], NULL, 16);
    } else {
      int offset = blank - param;
      char *quote = oc_strnchr(&param[offset], '\"', param_len);
      int quote_len = quote - (&param[offset]);
      int len_q = quote_len - 10;
      int len = param_len - offset - 9;
      if (len > len_q) {
        len = len_q;
      }
      *ia = (uint32_t)strtol(&param[10], NULL, 16);
      if (strncmp(&param[offset + 1], "knx://sn.", 9) == 0) {
        strncpy(sn, (char *)&param[offset + 1 + 9], len);
        error = 0;
      }
    }
  } else if (strncmp(param, "knx://ia.", 9) == 0) {
    // spec 1.1 ep= contents:
    // knx://ia.<sn> knx://sn.<ia>"
    char *blank = oc_strnchr(param, ' ', param_len);
    if (blank == NULL) {
      // the sn part is missing
      PRINT("oc_get_sn_from_ep 222 string: string ia : '%s'\n", &param[9]);
      // read from hex
      *ia = (uint32_t)strtol(&param[9], NULL, 16);
    } else {
      int offset = blank - param;
      char *quote = oc_strnchr(&param[offset], '\"', param_len);
      int quote_len = quote - (&param[offset]);
      int len_q = quote_len - 10;
      int len = param_len - offset - 9;
      if (len > len_q) {
        len = len_q;
      }
      *ia = (uint32_t)strtol(&param[9], NULL, 16);
      if (strncmp(&param[offset + 1], "knx://sn.", 9) == 0) {
        strncpy(sn, (char *)&param[offset + 1 + 9], len);
        error = 0;
      }
    }
  }
  return error;
}

static int
parse_uint64(const char *str, uint64_t *value)
{
  int filled_var = sscanf(str, "%" SCNx64, value);

  if (filled_var == 1) {
    return 0;
  }
  return -1;
}

// parse ia from "knx://ia.<ia>.
static int
parse_ia(const char *str, uint32_t *value)
{
  *value = (uint32_t)strtol(&str[9], NULL, 16);
  return 0;
}

// parse iid from knx://ia.<ia>.<iid>
static int
parse_iid(const char *str, uint64_t *value)
{
  char *point = oc_strnchr(&str[1 + 9], '.', 20);
  if (point == NULL) {
    return -1;
  }
  if (isxdigit(*(point + 1)) == 0) {
    // first expected digit is not hex
    return -1;
  }
  return parse_uint64(point + 1, value);
}

// parse iid from knx://sn.<sn>
static int
parse_sn(const char *str, char *sn, int len_input)
{
  if (str) {
    int len = strlen(str);
    int cp_len = len;
    int cp_len_quote = len;
    int cp_len_blank = len;

    char *blank = oc_strnchr(str, ' ', len);
    char *quote = oc_strnchr(str, '"', len);
    if (blank) {
      cp_len_blank = MAX((blank - str) - 9, 0);
    }
    if (quote) {
      cp_len_quote = MAX((quote - str) - 9, 0);
    }
    cp_len = MIN(cp_len_quote, cp_len_blank);
    if (cp_len > len_input) {
      return -1;
    }
    if (cp_len == 0) {
      return -1;
    }
    if (str && strncmp(str, "knx://sn.", 9) == 0) {
      strncpy(sn, (char *)&str[9], cp_len);
      return 0;
    }
  }
  return -1;
}

int
oc_get_sn_ia_iid_from_ep(const char *param, int param_len, char *sn, int sn_len,
                         uint32_t *ia, uint64_t *iid)
{
  int error = -1;
  memset(sn, 0, sn_len);
  *ia = 0;
  *iid = 0;
  if (param_len < 10) {
    return -1;
  }
  if (param == NULL) {
    return -1;
  }
  char *k = oc_strnchr(param, 'k', param_len);
  if (k == NULL) {
    return -1;
  }
  // starting with serial number
  // "knx://sn.<sn> knx://ia.<ia>.<iid>"
  if (strncmp(k, "knx://sn.", 9) == 0) {
    error = parse_sn(k, sn, sn_len);
    if (error) {
      return -1;
    }
    // find the next k, note that the sn can't contain a k
    char *k2 = oc_strnchr(&param[9], 'k', param_len - 9);
    if (k2 == NULL) {
      // the ia part is missing
      return -1;
    }
    // make sure it is the ia string
    if (strncmp(k2, "knx://ia.", 9) == 0) {
      error = parse_ia(k2, ia);
      if (error != 0) {
        return -1;
      }
      error = parse_iid(k2, iid);
      if (error != 0) {
        return -1;
      }
      // all ok
      return 0;
    }
  } else if (strncmp(k, "knx://ia.", 9) == 0) {
    // "knx://ia.<ia>.<iid> knx://sn.<sn>"
    error = parse_ia(k, ia);
    if (error != 0) {
      return -1;
    }
    error = parse_iid(k, iid);
    if (error != 0) {
      return -1;
    }
    // find the next k, note that the ia & iid can't contain a k
    char *k2 = oc_strnchr(&param[9], 'k', param_len - 9);
    if (k2 == NULL) {
      // the ia part is missing
      return -1;
    }
    if (strncmp(k2, "knx://sn.", 9) == 0) {
      error = parse_sn(k2, sn, sn_len);
      if (error != 0) {
        return -1;
      }
      return 0;
    }
  }
  // if not returned, then error
  return -1;
}