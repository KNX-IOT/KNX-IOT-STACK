/*
// Copyright (c) 2016 Intel Corporation
// Copyright (c) 2021 Cascoda Ltd.
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
/**
  @brief generic helpers
  @file
*/
#ifndef OC_HELPERS_H
#define OC_HELPERS_H

#include "util/oc_list.h"
#include "util/oc_mmem.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct oc_mmem oc_handle_t, oc_string_t, oc_array_t, oc_string_array_t,
  oc_byte_string_array_t;

enum StringRepresentation {
  DEC_REPRESENTATION = 0,
  HEX_REPRESENTATION,
};

#define oc_cast(block, type) ((type *)(OC_MMEM_PTR(&(block))))

/**
 * @brief cast oc_string to string
 *
 */
#define oc_string(ocstring) (oc_cast(ocstring, char))

/**
 * @brief cast oc_string to string, replace null pointer results
 * with a pointer to "NULL"
 *
 */
#define oc_string_checked(ocstring)                                            \
  (oc_cast(ocstring, char) ? oc_cast(ocstring, char) : "NULL")

#ifdef OC_MEMORY_TRACE
#define oc_alloc_string(ocstring, size)                                        \
  _oc_alloc_string(__func__, ocstring, size)
#define oc_new_string(ocstring, str, str_len)                                  \
  _oc_new_string(__func__, ocstring, str, str_len)

#define oc_free_string(ocstring) _oc_free_string(__func__, ocstring)
#define oc_free_int_array(ocarray) (_oc_free_array(__func__, ocarray, INT_POOL))
#define oc_free_bool_array(ocarray)                                            \
  (_oc_free_array(__func__, ocarray, BYTE_POOL))
#define oc_free_float_array(ocarray)                                           \
  (_oc_free_array(__func__, ocarray, FLOAT_POOL))
#define oc_free_double_array(ocarray)                                          \
  (_oc_free_array(__func__, ocarray, DOUBLE_POOL))

#define oc_new_int_array(ocarray, size)                                        \
  (_oc_new_array(__func__, ocarray, size, INT_POOL))
#define oc_new_bool_array(ocarray, size)                                       \
  (_oc_new_array(__func__, ocarray, size, BYTE_POOL))
#define oc_new_float_array(ocarray, size)                                      \
  (_oc_new_array(__func__, ocarray, size, FLOAT_POOL))
#define oc_new_double_array(ocarray, size)                                     \
  (_oc_new_array(__func__, ocarray, size, DOUBLE_POOL))

#define oc_new_string_array(ocstringarray, size)                               \
  (_oc_alloc_string_array(__func__, ocstringarray, size))

#define oc_free_string_array(ocstringarray)                                    \
  (_oc_free_string(__func__, ocstringarray))

#define oc_new_byte_string_array(ocstringarray, size)                          \
  (_oc_alloc_string_array(__func__, ocstringarray, size))

#define oc_free_byte_string_array(ocstringarray)                               \
  (__func__, _oc_free_string(ocstringarray))

#else /* OC_MEMORY_TRACE */

/**
 * @brief allocate oc_string
 *
 */
#define oc_alloc_string(ocstring, size) _oc_alloc_string((ocstring), (size))

/**
 * @brief create new string from string (null terminated)
 *
 */
#define oc_new_string(ocstring, str, str_len)                                  \
  _oc_new_string(ocstring, str, str_len)

/**
 * @brief create new (byte) string from string (not null terminated)
 *
 */
#define oc_new_byte_string(ocstring, str, str_len)                             \
  _oc_new_byte_string(ocstring, str, str_len)

/**
 * @brief free ocstring
 *
 */
#define oc_free_string(ocstring) _oc_free_string(ocstring)

/**
 * @brief free array of integers
 *
 */
#define oc_free_int_array(ocarray) (_oc_free_array(ocarray, INT_POOL))

/**
 * @brief free array of booleans
 *
 */
#define oc_free_bool_array(ocarray) (_oc_free_array(ocarray, BYTE_POOL))

/**
 * @brief free array of floats
 *
 */
#define oc_free_float_array(ocarray) (_oc_free_array(ocarray, FLOAT_POOL))

/**
 * @brief free array of doubles
 *
 */
#define oc_free_double_array(ocarray) (_oc_free_array(ocarray, DOUBLE_POOL))

/**
 * @brief new integer array
 *
 */
#define oc_new_int_array(ocarray, size) (_oc_new_array(ocarray, size, INT_POOL))

/**
 * @brief new boolean array
 *
 */
#define oc_new_bool_array(ocarray, size)                                       \
  (_oc_new_array(ocarray, size, BYTE_POOL))

/**
 * @brief new float array
 *
 */
#define oc_new_float_array(ocarray, size)                                      \
  (_oc_new_array(ocarray, size, FLOAT_POOL))

/**
 * @brief new double array
 *
 */
#define oc_new_double_array(ocarray, size)                                     \
  (_oc_new_array(ocarray, size, DOUBLE_POOL))

/**
 * @brief new oc string array
 *
 */
#define oc_new_string_array(ocstringarray, size)                               \
  (_oc_alloc_string_array(ocstringarray, size))

/**
 * @brief free oc string array
 *
 */
#define oc_free_string_array(ocstringarray) (_oc_free_string(ocstringarray))

#define oc_new_byte_string_array(ocstringarray, size)                          \
  (_oc_alloc_string_array(ocstringarray, size))

#define oc_free_byte_string_array(ocstringarray)                               \
  (_oc_free_string(ocstringarray))

#endif /* !OC_MEMORY_TRACE */
#define _MAKE_NULL(...) NULL
#define _ECHO
#define OC_SIZE_ZERO() _MAKE_NULL, 0
#define OC_SIZE_MANY(x) _ECHO, x
/**
 * @brief Helper macros to create const versions of oc types
 * These are special and need some help to understand things correctly
 */
/**
 * @brief creates a const oc_mmem struct
 * unlikely to be used outside of the library
 * @param count number of elements
 * @param ptr pointer to const data
 */
#define oc_mmem_create_const(count, ptr)                                       \
  {                                                                            \
    NULL, count, ptr                                                           \
  }
#define oc_string_create_const(s) oc_mmem_create_const(sizeof(s), s)
#define oc_string_array_create_const(f, n, ...)                                \
  oc_mmem_create_const(                                                        \
    (n * STRING_ARRAY_ITEM_MAX_LEN),                                           \
    f((const char[n][STRING_ARRAY_ITEM_MAX_LEN]){ __VA_ARGS__ }))
#define oc_int_array_create_const(f, n, ...)                                   \
  oc_mmem_create_const(n, f((const int64_t[n]){ __VA_ARGS__ }))
#define oc_bool_array_create_const(f, n, ...)                                  \
  oc_mmem_create_const(n, f((const bool[n]){ __VA_ARGS__ }))
#define oc_float_array_create_const(f, n, ...)                                 \
  oc_mmem_create_const(n, f((const float[n]){ __VA_ARGS__ }))
#define oc_double_array_create_const(f, n, ...)                                \
  oc_mmem_create_const(n, f((const double[n]){ __VA_ARGS__ }))

void oc_concat_strings(oc_string_t *concat, const char *str1, const char *str2);
#define oc_string_len(ocstring) ((ocstring).size ? (ocstring).size - 1 : 0)
#define oc_byte_string_len(ocstring) ((ocstring).size)

#define oc_int_array_size(ocintarray) ((ocintarray).size)
#define oc_bool_array_size(ocboolarray) ((ocboolarray).size)
#define oc_float_array_size(ocfloatarray) ((ocfloatarray).size)
#define oc_double_array_size(ocdoublearray) ((ocdoublearray).size)
#define oc_string_array_size(ocstringarray)                                    \
  ((ocstringarray).size / STRING_ARRAY_ITEM_MAX_LEN)
#define oc_int_array(ocintarray) (oc_cast(ocintarray, int64_t))
#define oc_bool_array(ocboolarray) (oc_cast(ocboolarray, bool))
#define oc_float_array(ocfloatarray) (oc_cast(ocfloatarray, float))
#define oc_double_array(ocdoublearray) (oc_cast(ocdoublearray, double))
#define oc_string_array(ocstringarray)                                         \
  ((char(*)[STRING_ARRAY_ITEM_MAX_LEN])(OC_MMEM_PTR(&(ocstringarray))))

#ifdef OC_DYNAMIC_ALLOCATION
#define STRING_ARRAY_ITEM_MAX_LEN 32
#else /* OC_DYNAMIC_ALLOCATION */
#define STRING_ARRAY_ITEM_MAX_LEN 32
#endif /* !OC_DYNAMIC_ALLOCATION */

bool _oc_copy_string_to_array(oc_string_array_t *ocstringarray,
                              const char str[], size_t index);
bool _oc_string_array_add_item(oc_string_array_t *ocstringarray,
                               const char str[]);
void oc_join_string_array(oc_string_array_t *ocstringarray,
                          oc_string_t *ocstring);

bool _oc_copy_byte_string_to_array(oc_string_array_t *ocstringarray,
                                   const char str[], size_t str_len,
                                   size_t index);
bool _oc_byte_string_array_add_item(oc_string_array_t *ocstringarray,
                                    const char str[], size_t str_len);

/* Arrays of text strings */
#define oc_string_array_add_item(ocstringarray, str)                           \
  (_oc_string_array_add_item(&(ocstringarray), str))
#define oc_string_array_get_item(ocstringarray, index)                         \
  (oc_string(ocstringarray) + (index)*STRING_ARRAY_ITEM_MAX_LEN)
#define oc_string_array_set_item(ocstringarray, str, index)                    \
  (_oc_copy_string_to_array(&(ocstringarray), str, index))
#define oc_string_array_get_item_size(ocstringarray, index)                    \
  (strlen((const char *)oc_string_array_get_item(ocstringarray, index)))
#define oc_string_array_get_allocated_size(ocstringarray)                      \
  ((ocstringarray).size / STRING_ARRAY_ITEM_MAX_LEN)

/* Arrays of byte strings */
#define oc_byte_string_array_add_item(ocstringarray, str, str_len)             \
  (_oc_byte_string_array_add_item(&(ocstringarray), str, str_len))
#define oc_byte_string_array_get_item(ocstringarray, index)                    \
  (oc_string(ocstringarray) + (index)*STRING_ARRAY_ITEM_MAX_LEN + 1)
#define oc_byte_string_array_set_item(ocstringarray, str, str_len, index)      \
  (_oc_copy_byte_string_to_array(&(ocstringarray), str, str_len, index))
#define oc_byte_string_array_get_item_size(ocstringarray, index)               \
  (*(oc_string(ocstringarray) + (index)*STRING_ARRAY_ITEM_MAX_LEN))
#define oc_byte_string_array_get_allocated_size(ocstringarray)                 \
  ((ocstringarray).size / STRING_ARRAY_ITEM_MAX_LEN)

/**
 * @brief new oc_string from string
 *
 * @param ocstring the ocstring to be allocated
 * @param str terminated string
 * @param str_len size of the string to be copied
 */
void _oc_new_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring, const char *str, size_t str_len);

/**
 * @brief new oc_string byte from string
 *
 * @param ocstring the ocstring to be allocated
 * @param str not terminated string
 * @param str_len size of the string to be copied
 */
void _oc_new_byte_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring, const char *str, size_t str_len);

/**
 * @brief allocate oc_string
 *
 * @param ocstring the ocstring to be allocated
 * @param size size to be allocated
 */
void _oc_alloc_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring, size_t size);

/**
 * @brief free oc string
 *
 * @param ocstring the ocstring to be freed
 */
void _oc_free_string(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_t *ocstring);

/**
 * @brief free array
 *
 * @param ocarray the ocarray to be freed
 * @param type pool type
 */
void _oc_free_array(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_array_t *ocarray, pool type);

/**
 * @brief new array
 *
 * @param ocarray the ocarray to be freed
 * @param size the size to be allocated
 * @param type pool type
 */
void _oc_new_array(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_array_t *ocarray, size_t size, pool type);

/**
 * @brief allocate string array
 *
 * @param ocstringarray array to be allocated
 * @param size the size of the string array
 */
void _oc_alloc_string_array(
#ifdef OC_MEMORY_TRACE
  const char *func,
#endif
  oc_string_array_t *ocstringarray, size_t size);

/** Conversions between hex encoded strings and byte arrays */

/**
 * @brief convert array to hex
 *
 * Note: hex_str is pre allocated with hex_str_len
 *
 * @param[in] array the array of bytes
 * @param[in] array_len length of the array
 * @param hex_str data as hex
 * @param hex_str_len length of the hex string
 * @return int 0 success
 */
int oc_conv_byte_array_to_hex_string(const uint8_t *array, size_t array_len,
                                     char *hex_str, size_t *hex_str_len);

/**
 * @brief convert hex string to byte array
 *
 * @param[in] hex_str hex string input
 * @param[in] hex_str_len size of the hex string
 * @param array the array of bytes
 * @param array_len length of the byte array
 * @return int 0 success
 */
int oc_conv_hex_string_to_byte_array(const char *hex_str, size_t hex_str_len,
                                     uint8_t *array, size_t *array_len);
/**
 * @brief convert hex string to oc_string
 *
 * @param[in] hex_str hex string input
 * @param[in] hex_str_len size of the hex string
 * @param out the allocated oc_string
 * @return int 0 success
 */
int oc_conv_hex_string_to_oc_string(const char *hex_str, size_t hex_str_len,
                                    oc_string_t *out);

/**
 * @brief checks if the input is an array containing hex values
 * e.g. [0-9,A-F,a-f]
 *
 * @param[in] hex_string the input string to be checked
 * @return int 0 success
 */
int oc_string_is_hex_array(oc_string_t hex_string);

/**
 * @brief prints the input as hex string
 *
 * @param[in] hex_string the input string to be printed
 * @return int printed amount of %x
 */
int oc_string_print_hex(oc_string_t hex_string);

/**
 * @brief prints the input as hex string with newline (\n) at the end.
 *
 * @param[in] hex_string the input string to be printed
 * @return int printed amount of %x
 */
int oc_string_println_hex(oc_string_t hex_string);

/**
 * @brief converts the input string to lower case
 *
 * @param[in] string the input string that gets converted
 * @return int 0 success
 */
int oc_char_convert_to_lower(char *string);

/**
 * @brief prints the string as hex
 *
 * @param[in] str the input string to be printed
 * @param[in] str_len the length of the input string
 * @return int printed amount of %x
 */
int oc_char_print_hex(const char *str, int str_len);
/**
 * @brief prints the input as hex string with newline (\n) at the end.
 *
 * @param[in] str the input string to be printed
 * @param[in] str_len the length of the input string
 * @return int printed amount of %x
 */
int oc_char_println_hex(const char *str, int str_len);

/**
 * @brief checks if the uri contains a wildcard (e.g. "*")
 *
 * @param uri The URI to be checked.
 * @return true
 * @return false
 */
bool oc_uri_contains_wildcard(const char *uri);

/**
 * @brief retrieve the wild card value as integer
 * The invoked URI is checked against the URI of a resource
 * that might contain a wild card, if the resource URI contains a wild card
 * then the invoked URI is compared against this URI and
 * e.g.  resource URI: / abc / *
 * invoked URI: / abc / 1
 * return will be 1.
 *
 * NOTE: the wild card part of the URL should only contain a number, e.g. no
 * prefix to the number
 * @param uri_resource The URI with wild card
 * @param uri_len The length of the URI with wild card
 * @param uri_invoked The URI that should match a wild card
 * @param invoked_len The URI length of the invoked URI
 * @return int -1 is error, otherwise the value is the integer value which is
 * used as value for the wild card.
 */
int oc_uri_get_wildcard_value_as_int(const char *uri_resource, size_t uri_len,
                                     const char *uri_invoked,
                                     size_t invoked_len);

/**
 * @brief function to check if in the wild card section is a "_" (underscore)
 * underscores can be used in functional block uri to have more than 1 instance
 * e.g. fb* as wild card and fb333_1 as url
 * @param uri_resource The URI with wild card
 * @param uri_len The length of the URI with wild card
 * @param uri_invoked The URI that should match a wild card
 * @param invoked_len The URI length of the invoked URI
 * @return true
 * @return false
 */
bool oc_uri_contains_wildcard_value_underscore(const char *uri_resource,
                                               size_t uri_len,
                                               const char *uri_invoked,
                                               size_t invoked_len);

/**
 * @brief retrieve the integer after the "_" (underscore)
 * e.g. retrieve 1 from fb333_1 as url and fb* as wild card
 * @param uri_resource The URI with wild card
 * @param uri_len The length of the URI with wild card
 * @param uri_invoked The URI that should match a wild card
 * @param invoked_len The URI length of the invoked URI
 * @return int
 */
int oc_uri_get_wildcard_value_as_int_after_underscore(const char *uri_resource,
                                                      size_t uri_len,
                                                      const char *uri_invoked,
                                                      size_t invoked_len);

/**
 * @brief retrieve the wild card value as string
 * The invoked URI is checked against the URI of a resource
 * that might contain a wild card, if the resource URI contains a wild card
 * then the invoked URI is compared against this URI and
 * e.g.  resource URI: / abc / *
 * invoked URI: / abc / y
 * return will be y.
 *
 * @param uri_resource The URI with wild card
 * @param uri_len The length of the URI with wild card
 * @param uri_invoked The URI that should match a wild card
 * @param invoked_len The URI length of the invoked URI
 * @param value the actual value that represents the wild card
 * @return int -1 is error, otherwise the value is the integer length of the
 * string
 */
int oc_uri_get_wildcard_value_as_string(const char *uri_resource,
                                        size_t uri_len, const char *uri_invoked,
                                        size_t invoked_len, const char **value);

/**
 * @brief search a string (non null terminated) for a character
 *
 * @param string the string to be searched
 * @param p the character to be found
 * @param size the size of the string
 * @return NULL = not found, other wise position in string
 * string
 */
char *oc_strnchr(const char *string, char p, int size);

/**
 * @brief retrieves the serial number and individual address from the ep
 * parameter
 *
 * deprecated!!
 *
 * @param param the string to be searched
 * @param param_len the length of the parameter
 * @param sn the sn for storage
 * @param sn_len the length of the sn for storage
 * @param ia the individual address
 * @return 0 == ok
 * string
 */
int oc_get_sn_from_ep(const char *param, int param_len, char *sn, int sn_len,
                      uint32_t *ia);

/**
 * @brief retrieves the serial number and individual address from the ep
 * parameter
 *
 * @param param the string to be searched
 * @param param_len the length of the parameter
 * @param sn the serial number
 * @param sn_len the length of the serial number
 * @param ia the individual address
 * @param iid the installation id
 * @return 0 == ok
 * string
 */
int oc_get_sn_ia_iid_from_ep(const char *param, int param_len, char *sn,
                             int sn_len, uint32_t *ia, uint64_t *iid);

/**
 * @brief copy string from char*
 *
 * @param string1 the oc_string to copy to
 * @param string2 the char* to copy from
 * @return int 0 == success
 */
int oc_string_copy_from_char(oc_string_t *string1, const char *string2);

/**
 * @brief copy string from char*
 *
 * Note: adds a null terminator
 * @param string1 the oc_string to copy to
 * @param string2 the char* to copy from
 * @param string2_len the length of string2
 * @return int 0 == success
 */
int oc_string_copy_from_char_with_size(oc_string_t *string1,
                                       const char *string2, size_t string2_len);

/**
 * @brief copy byte string from char*
 *
 * Note: does NOT add a null terminator
 * @param string1 the oc_string to copy to
 * @param string2 the char* to copy from
 * @param string2_len the length of string2
 * @return int 0 == success
 */
int oc_byte_string_copy_from_char_with_size(oc_string_t *string1,
                                            const char *string2,
                                            size_t string2_len);

/**
 * @brief copy oc_string
 *
 * @param string1 the oc_string to copy to
 * @param string2 the oc_string to copy from
 * @return int 0 == success
 */
int oc_string_copy(oc_string_t *string1, oc_string_t string2);

/**
 * @brief copy oc_string used as a byte string
 *
 * @param string1 the oc_string to copy to
 * @param string2 the oc_string to copy from
 * @return int 0 == success
 */
int oc_byte_string_copy(oc_string_t *string1, oc_string_t string2);

/**
 * @brief oc_string compare
 *
 * @param string1 string 1 to be compared
 * @param string2 string 2 to be compared
 * @return int 0 == equal
 */
int oc_string_cmp(oc_string_t string1, oc_string_t string2);

/**
 * @brief oc_string compare for byte strings (no null terminator)
 *
 * @param string1 byte string 1 to be compared
 * @param string2 byte string 2 to be compared
 * @return int 0 == equal
 */
int oc_byte_string_cmp(oc_string_t string1, oc_string_t string2);

/**
 * @brief url compare
 * same as string compare but ignores the leading / of the urls
 *
 * @param string1 url to be compared
 * @param string2 url to be compared
 * @return int 0 == equal
 */
int oc_url_cmp(oc_string_t string1, oc_string_t string2);

/**
 * @brief print a uint64_t, in either decimal or hex representation
 *
 * @param number
 * @param rep - string representation chosen (decimal or hex)
 * @return int always returns 0
 */
int oc_print_uint64_t(uint64_t number, enum StringRepresentation rep);

/**
 * @brief Converts a uint64_t to a decimal string representation
 *
 * @param[in] number number to be converted to string
 * @param[out] str Resulting string after conversion. IMPORTANT: Should have
 * a size of at least 22 bytes (21 + null terminator)
 * @return int always returns 0
 */
int oc_conv_uint64_to_dec_string(char *str, uint64_t number);

/**
 * @brief Converts a uint64_t to a hex string representation
 *
 * @param[in] number number to be converted to hexadecimal string
 * @param[out] str Resulting string after conversion. IMPORTANT: Should have
 * a size of at least 17 bytes (16 + null terminator)
 * @return int always returns 0
 */
int oc_conv_uint64_to_hex_string(char *str, uint64_t number);

#ifdef __cplusplus
}
#endif

#endif /* OC_HELPERS_H */
