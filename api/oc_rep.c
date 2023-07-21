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

#include "oc_rep.h"
#include "oc_base64.h"
#include "oc_config.h"
#include "port/oc_assert.h"
#include "port/oc_log.h"
#include "util/oc_memb.h"

#include <inttypes.h>

static struct oc_memb *rep_objects;
static uint8_t *g_buf;
CborEncoder g_encoder, root_map, links_array;
CborError g_err;

void
oc_rep_set_pool(struct oc_memb *rep_objects_pool)
{
  rep_objects = rep_objects_pool;
}

void
oc_rep_new(uint8_t *out_payload, int size)
{
  g_err = CborNoError;
  g_buf = out_payload;
  cbor_encoder_init(&g_encoder, out_payload, size, 0);
}

CborError
oc_rep_get_cbor_errno(void)
{
  return g_err;
}

const uint8_t *
oc_rep_get_encoder_buf(void)
{
  return g_buf;
}

void
oc_rep_encode_raw(const uint8_t *data, size_t len)
{
  memcpy(g_encoder.data.ptr, data, len);
  g_encoder.data.ptr = g_encoder.data.ptr + len;
  g_err = CborNoError;
}

void
oc_rep_encode_raw_encoder(CborEncoder *encoder, const uint8_t *data, size_t len)
{
  memcpy(encoder->data.ptr, data, len);
  encoder->data.ptr = encoder->data.ptr + len;
  g_err = CborNoError;
}

int
oc_rep_add_line_to_buffer(const char *line)
{
  int len = 0;
  if (line != NULL) {
    len = (int)strlen(line);
    oc_rep_encode_raw((uint8_t *)line, len);
  }
  return len;
}

int
oc_rep_add_line_size_to_buffer(const char *line, int len)
{
  if (line != NULL) {
    oc_rep_encode_raw((uint8_t *)line, len);
    return len;
  }
  return 0;
}

int
oc_rep_get_encoded_payload_size(void)
{
  size_t size = cbor_encoder_get_buffer_size(&g_encoder, g_buf);
  if (g_err == CborErrorOutOfMemory) {
    OC_ERR("Insufficient memory: Increase OC_MAX_APP_DATA_SIZE to "
           "accommodate a larger payload");
  }
  if (g_err != CborNoError)
    return -1;
  return (int)size;
}

static oc_rep_t *
_alloc_rep(void)
{
  oc_rep_t *rep = oc_memb_alloc(rep_objects);
  if (rep != NULL) {
    rep->name.size = 0;
    rep->iname = -1;
  }
#ifdef OC_DEBUG
  oc_assert(rep != NULL);
#endif
  return rep;
}

static void
_free_rep(oc_rep_t *rep_value)
{
  oc_memb_free(rep_objects, rep_value);
}

void
oc_free_rep(oc_rep_t *rep)
{
  if (rep == 0)
    return;
  oc_free_rep(rep->next);
  switch (rep->type) {
  case OC_REP_BYTE_STRING_ARRAY:
  case OC_REP_STRING_ARRAY:
    oc_free_string_array(&rep->value.array);
    break;
  case OC_REP_BOOL_ARRAY:
    oc_free_bool_array(&rep->value.array);
    break;
  case OC_REP_FLOAT_ARRAY:
    oc_free_float_array(&rep->value.array);
    break;
  case OC_REP_DOUBLE_ARRAY:
    oc_free_double_array(&rep->value.array);
    break;
  case OC_REP_INT_ARRAY:
    oc_free_int_array(&rep->value.array);
    break;
  case OC_REP_BYTE_STRING:
  case OC_REP_STRING:
    oc_free_string(&rep->value.string);
    break;
  case OC_REP_OBJECT:
    oc_free_rep(rep->value.object);
    break;
  case OC_REP_OBJECT_ARRAY:
    oc_free_rep(rep->value.object_array);
    break;
  case OC_REP_MIXED_ARRAY:
    oc_free_rep(rep->value.mixed_array);
    break;
  default:
    break;
  }
  if (rep->name.size > 0)
    oc_free_string(&rep->name);
  _free_rep(rep);
}

static void oc_parse_rep_value_object(CborValue *value, oc_rep_t **rep,
                                      CborError *err);
static void oc_parse_rep_value_array(CborValue *value, oc_rep_t **rep,
                                     CborError *err);

/*
  An Object is a collection of key-value pairs.
  A value_object value points to the first key-value pair,
  and subsequent items are accessed via the next pointer.

  An Object Array is a collection of objects, where each object
  is a collection of key-value pairs.
  A value_object_array value points to the first object in the
  array. This object is then traversed via its value_object pointer.
  Subsequent objects in the object array are then accessed through
  the next pointer of the first object.
*/

static void
oc_parse_single_entity(CborValue *value, oc_rep_t **rep, CborError *err)
{
  size_t len;
  if (*rep == NULL)
    *rep = _alloc_rep();

  if (*rep == NULL) {
    *err = CborErrorOutOfMemory;
    return;
  }
  oc_rep_t *cur = *rep;

  switch (value->type) {
  case CborTagType: {
    CborTag tag;
    cbor_value_get_tag(value, &tag);
    /* skip over CBOR Tags */
    // goto get_tagged_value;
  } break;
  case CborIntegerType:
    *err |= cbor_value_get_int64(value, &cur->value.integer);
    cur->type = OC_REP_INT;
    break;
  case CborBooleanType:
    *err |= cbor_value_get_boolean(value, &cur->value.boolean);
    cur->type = OC_REP_BOOL;
    break;
  case CborFloatType:
    *err |= cbor_value_get_float(value, &cur->value.float_p);
    cur->type = OC_REP_FLOAT;
    break;
  case CborDoubleType:
    *err |= cbor_value_get_double(value, &cur->value.double_p);
    cur->type = OC_REP_DOUBLE;
    break;
  case CborByteStringType:
    *err |= cbor_value_calculate_string_length(value, &len);
    len++;
    if (*err != CborNoError || len == 0)
      return;
    oc_alloc_string(&cur->value.string, len);
    *err |= cbor_value_copy_byte_string(
      value, oc_cast(cur->value.string, uint8_t), &len, NULL);
    cur->type = OC_REP_BYTE_STRING;
    break;
  case CborTextStringType:
    *err |= cbor_value_calculate_string_length(value, &len);
    len++;
    if (*err != CborNoError || len == 0)
      return;
    oc_alloc_string(&cur->value.string, len);
    *err |= cbor_value_copy_text_string(value, oc_string(cur->value.string),
                                        &len, NULL);
    cur->type = OC_REP_STRING;
    break;
  case CborMapType:
    oc_parse_rep_value_object(value, rep, err);
    break;
  case CborArrayType:
    oc_parse_rep_value_array(value, rep, err);
    break;
  default:
    break;
  }
}

/* Parse single property */
static void
oc_parse_rep_value(CborValue *value, oc_rep_t **rep, CborError *err)
{
  size_t k, len;
  CborValue map, array;
  *rep = _alloc_rep();
  if (*rep == NULL) {
    *err = CborErrorOutOfMemory;
    return;
  }
  oc_rep_t *cur = *rep, **prev = 0;
  cur->next = 0;
  cur->value.object_array = 0;

  /* key */
  if (cbor_value_is_text_string(value)) {
    /* key as string */
    *err |= cbor_value_calculate_string_length(value, &len);
    len++;
    if (*err != CborNoError || len == 0)
      return;
    oc_alloc_string(&cur->name, len);
    *err |= cbor_value_copy_text_string(value, (char *)oc_string(cur->name),
                                        &len, NULL);
    if (*err != CborNoError)
      return;
  } else if (cbor_value_is_integer(value)) {
    /* key as integer */
    *err |= cbor_value_get_int(value, &cur->iname);
    if (*err != CborNoError)
      return;
  } else {
    *err = CborErrorIllegalType;
    return;
  }

get_tagged_value:
  *err |= cbor_value_advance(value);
  /* value */
  switch (value->type) {
  case CborTagType: {
    CborTag tag;
    cbor_value_get_tag(value, &tag);
    /* skip over CBOR Tags */
    goto get_tagged_value;
  } break;

  case CborInvalidType:
    *err |= CborErrorIllegalType;
    return;
  default:
    oc_parse_single_entity(value, rep, err);
    break;
  }
}

static void
oc_parse_rep_value_object(CborValue *value, oc_rep_t **rep, CborError *err)
{

  if (value->type != CborMapType) {
    *err |= CborErrorIllegalType;
    return;
  }
  size_t k, len;
  CborValue map;
  if (*rep == NULL)
    *rep = _alloc_rep();
  if (*rep == NULL) {
    *err = CborErrorOutOfMemory;
    return;
  }
  oc_rep_t *cur = *rep, **prev = 0;
  cur->next = 0;
  cur->value.object_array = 0;

  oc_rep_t **obj = &cur->value.object;
  *err |= cbor_value_enter_container(value, &map);
  while (!cbor_value_at_end(&map)) {
    oc_parse_rep_value(&map, obj, err);
    if (*err != CborNoError)
      return;
    if ((obj) && (*obj)) {
      (*obj)->next = NULL;
      obj = &(*obj)->next;
    }
    *err |= cbor_value_advance(&map);
  }
  cur->type = OC_REP_OBJECT;
}

static void
oc_parse_rep_value_array(CborValue *value, oc_rep_t **rep, CborError *err)
{

  size_t k, len;
  CborValue array;
  if (*rep == NULL)
    *rep = _alloc_rep();
  if (*rep == NULL) {
    *err = CborErrorOutOfMemory;
    return;
  }
  oc_rep_t *cur = *rep, **prev = 0;
  cur->next = 0;
  cur->value.object_array = 0;

  oc_rep_value_type_t type = OC_REP_NIL;
  *err |= cbor_value_enter_container(value, &array);
  len = 0;
  // we need to iterate through anyway to check the types
  // cbor_value_get_array_length(value, &len);
  if (len == 0) {
    CborValue t = array;
    while (!cbor_value_at_end(&t)) {
      len++;
      if (*err != CborNoError)
        return;
      switch (t.type) {
      case CborIntegerType:
        if (type == OC_REP_NIL || type == OC_REP_INT)
          type = OC_REP_INT;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      case CborFloatType:
        if (type == OC_REP_NIL || type == OC_REP_FLOAT)
          type = OC_REP_FLOAT;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      case CborDoubleType:
        if (type == OC_REP_NIL || type == OC_REP_DOUBLE)
          type = OC_REP_DOUBLE;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      case CborBooleanType:
        if (type == OC_REP_NIL || type == OC_REP_BOOL)
          type = OC_REP_BOOL;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      case CborByteStringType:
        if (type == OC_REP_NIL || type == OC_REP_BYTE_STRING)
          type = OC_REP_BYTE_STRING;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      case CborTextStringType:
        if (type == OC_REP_NIL || type == OC_REP_STRING)
          type = OC_REP_STRING;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      case CborMapType:
        if (type == OC_REP_NIL || type == OC_REP_OBJECT)
          type = OC_REP_OBJECT;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      case CborArrayType:
        if (type == OC_REP_NIL || type == OC_REP_ARRAY)
          type = OC_REP_ARRAY;
        else
          type = OC_REP_MIXED_ARRAY;
        break;
      }
      *err = cbor_value_advance(&t);
    }
  }
  k = 0;
  while (!cbor_value_at_end(&array)) {
    switch (type) {
    case OC_REP_INT: {
      if (k == 0) {
        oc_new_int_array(&cur->value.array, len);
        cur->type = OC_REP_INT_ARRAY;
      }
      if (array.type != CborIntegerType) {
        *err |= CborErrorIllegalType;
        return;
      }
      *err |= cbor_value_get_int64(&array, oc_int_array(cur->value.array) + k);
    } break;
    case OC_REP_BOOL: {
      if (k == 0) {
        oc_new_bool_array(&cur->value.array, len);
        cur->type = OC_REP_BOOL_ARRAY;
      }
      if (array.type != CborBooleanType) {
        *err |= CborErrorIllegalType;
        return;
      }
      *err |=
        cbor_value_get_boolean(&array, oc_bool_array(cur->value.array) + k);
    } break;
    case OC_REP_FLOAT: {
      if (k == 0) {
        oc_new_float_array(&cur->value.array, len);
        cur->type = OC_REP_FLOAT_ARRAY;
      }
      if (array.type != CborFloatType) {
        *err |= CborErrorIllegalType;
        return;
      }
      *err |=
        cbor_value_get_float(&array, oc_float_array(cur->value.array) + k);
    } break;
    case OC_REP_DOUBLE: {
      if (k == 0) {
        oc_new_double_array(&cur->value.array, len);
        cur->type = OC_REP_DOUBLE_ARRAY;
      }
      if (array.type != CborDoubleType) {
        *err |= CborErrorIllegalType;
        return;
      }
      *err |=
        cbor_value_get_double(&array, oc_double_array(cur->value.array) + k);
    } break;
    case OC_REP_BYTE_STRING: {
      if (k == 0) {
        oc_new_byte_string_array(&cur->value.array, len);
        cur->type = OC_REP_BYTE_STRING_ARRAY;
      }
      if (array.type != CborByteStringType) {
        *err |= CborErrorIllegalType;
        return;
      }
      *err |= cbor_value_calculate_string_length(&array, &len);
      if (len >= STRING_ARRAY_ITEM_MAX_LEN) {
        len = STRING_ARRAY_ITEM_MAX_LEN - 1;
      }
      oc_byte_string_array_get_item_size(cur->value.array, k) = len;
      *err |= cbor_value_copy_byte_string(
        &array, (uint8_t *)oc_byte_string_array_get_item(cur->value.array, k),
        &len, NULL);
    } break;
    case OC_REP_STRING: {
      if (k == 0) {
        oc_new_string_array(&cur->value.array, len);
        cur->type = OC_REP_STRING_ARRAY;
      }
      if (array.type != CborTextStringType) {
        *err |= CborErrorIllegalType;
        return;
      }
      *err |= cbor_value_calculate_string_length(&array, &len);
      len++;
      if (len >= STRING_ARRAY_ITEM_MAX_LEN) {
        len = STRING_ARRAY_ITEM_MAX_LEN - 1;
      }
      uint8_t *size = (uint8_t *)oc_string_array_get_item(cur->value.array, k);
      *err |= cbor_value_copy_text_string(
        &array, (uint8_t *)oc_string_array_get_item(cur->value.array, k), &len,
        NULL);
      oc_string_array_get_item(cur->value.array, k)[len] = 0;
    } break;
    case OC_REP_OBJECT: {
      if (k == 0) {
        cur->type = OC_REP_OBJECT_ARRAY;
        cur->value.object_array = _alloc_rep();
        if (cur->value.object_array == NULL) {
          *err = CborErrorOutOfMemory;
          return;
        }
        prev = &cur->value.object_array;
      }

      if (array.type != CborMapType) {
        *err |= CborErrorIllegalType;
        return;
      }
      oc_parse_rep_value_object(&array, prev, err);
      (*prev)->next = 0;
      prev = &(*prev)->next;
    } break;
    case OC_REP_ARRAY: {
    }          // fallthrough to default case
    default: { // TUPLE ARRAY
      if (k == 0) {
        cur->type = OC_REP_MIXED_ARRAY;
        cur->value.mixed_array = _alloc_rep();
        if (cur->value.mixed_array == NULL) {
          *err = CborErrorOutOfMemory;
          return;
        }
        prev = &cur->value.mixed_array;
      }

      oc_parse_single_entity(&array, prev, err);
      (*prev)->next = 0;
      prev = &(*prev)->next;

    } break;
    }
    *err = cbor_value_advance(&array);
    if (*err != CborNoError)
      return;
    k++;
  }
}

int
oc_parse_rep(const uint8_t *in_payload, int payload_size, oc_rep_t **out_rep)
{
  CborParser parser;
  CborValue root_value, cur_value, map;
  CborError err = CborNoError;
  err |= cbor_parser_init(in_payload, payload_size, 0, &parser, &root_value);
  *out_rep = 0;
  if (cbor_value_is_valid(&root_value)) {
    oc_parse_single_entity(&root_value, out_rep, &err);
  }
  // since this has now changed so it returns an object/array at top level
  // rather than the first element (linked list style)
  // we need to correct this
  if (*out_rep) {
    oc_rep_t *r = *out_rep;
    if ((*out_rep)->type == OC_REP_OBJECT)
      *out_rep = (*out_rep)->value.object;
    else if ((*out_rep)->type == OC_REP_OBJECT_ARRAY)
      *out_rep = (*out_rep)->value.object_array;
    else if ((*out_rep)->type == OC_REP_MIXED_ARRAY)
      *out_rep = (*out_rep)->value.mixed_array;
    else
      return err;
    r->type = OC_REP_NIL;
    oc_free_rep(r);
  }
  return err;
}

static bool
oc_rep_get_value(oc_rep_t *rep, oc_rep_value_type_t type, const char *key,
                 void **value, size_t *size)
{
  if (!rep || !key || !value) {
    OC_ERR("Error of input parameters");
    return false;
  }

  oc_rep_t *rep_value = rep;
  while (rep_value != NULL) {
    if ((oc_string_len(rep_value->name) == strlen(key)) &&
        (strncmp(key, oc_string(rep_value->name),
                 oc_string_len(rep_value->name)) == 0) &&
        (rep_value->type == type)) {
      OC_DBG("Found the value with %s", key);
      switch (rep_value->type) {
      case OC_REP_INT:
        **(int64_t **)value = rep_value->value.integer;
        break;
      case OC_REP_BOOL:
        **(bool **)value = rep_value->value.boolean;
        break;
      case OC_REP_FLOAT:
        **(float **)value = rep_value->value.float_p;
        break;
      case OC_REP_DOUBLE:
        **(double **)value = rep_value->value.double_p;
        break;
      case OC_REP_BYTE_STRING:
      case OC_REP_STRING:
        *value = oc_string(rep_value->value.string);
        *size = oc_string_len(rep_value->value.string);
        break;
      case OC_REP_INT_ARRAY:
        *value = oc_int_array(rep_value->value.array);
        *size = (int)oc_int_array_size(rep_value->value.array);
        break;
      case OC_REP_BOOL_ARRAY:
        *value = oc_bool_array(rep_value->value.array);
        *size = (int)oc_bool_array_size(rep_value->value.array);
        break;
      case OC_REP_FLOAT_ARRAY:
        *value = oc_float_array(rep_value->value.array);
        *size = (int)oc_float_array_size(rep_value->value.array);
        break;
      case OC_REP_DOUBLE_ARRAY:
        *value = oc_double_array(rep_value->value.array);
        *size = (int)oc_double_array_size(rep_value->value.array);
        break;
      case OC_REP_BYTE_STRING_ARRAY:
      case OC_REP_STRING_ARRAY:
        **(oc_string_array_t **)value = rep_value->value.array;
        *size = (int)oc_string_array_get_allocated_size(rep_value->value.array);
        break;
      case OC_REP_OBJECT:
        *value = rep_value->value.object;
        break;
      case OC_REP_OBJECT_ARRAY:
        *value = rep_value->value.object_array;
        break;
      case OC_REP_MIXED_ARRAY:
        *value = rep_value->value.mixed_array;
        break;
      default:
        return false;
      }

      return true;
    }
    rep_value = rep_value->next;
  }

  return false;
}

static bool
oc_rep_i_get_value(oc_rep_t *rep, oc_rep_value_type_t type, int key,
                   void **value, size_t *size)
{
  if (!rep || !value) {
    OC_ERR("Error of input parameters");
    return false;
  }

  oc_rep_t *rep_value = rep;
  while (rep_value != NULL) {
    if ((rep_value->iname == key) && (rep_value->type == type)) {
      OC_DBG("Found the value with %d", key);
      switch (rep_value->type) {
      case OC_REP_INT:
        **(int64_t **)value = rep_value->value.integer;
        break;
      case OC_REP_BOOL:
        **(bool **)value = rep_value->value.boolean;
        break;
      case OC_REP_FLOAT:
        **(float **)value = rep_value->value.float_p;
        break;
      case OC_REP_DOUBLE:
        **(double **)value = rep_value->value.double_p;
        break;
      case OC_REP_BYTE_STRING:
      case OC_REP_STRING:
        *value = oc_string(rep_value->value.string);
        *size = oc_string_len(rep_value->value.string);
        break;
      case OC_REP_INT_ARRAY:
        *value = oc_int_array(rep_value->value.array);
        *size = (int)oc_int_array_size(rep_value->value.array);
        break;
      case OC_REP_BOOL_ARRAY:
        *value = oc_bool_array(rep_value->value.array);
        *size = (int)oc_bool_array_size(rep_value->value.array);
        break;
      case OC_REP_FLOAT_ARRAY:
        *value = oc_float_array(rep_value->value.array);
        *size = (int)oc_float_array_size(rep_value->value.array);
        break;
      case OC_REP_DOUBLE_ARRAY:
        *value = oc_double_array(rep_value->value.array);
        *size = (int)oc_double_array_size(rep_value->value.array);
        break;
      case OC_REP_BYTE_STRING_ARRAY:
      case OC_REP_STRING_ARRAY:
        **(oc_string_array_t **)value = rep_value->value.array;
        *size = (int)oc_string_array_get_allocated_size(rep_value->value.array);
        break;
      case OC_REP_OBJECT:
        *value = rep_value->value.object;
        break;
      case OC_REP_OBJECT_ARRAY:
        *value = rep_value->value.object_array;
        break;
      case OC_REP_MIXED_ARRAY:
        *value = rep_value->value.mixed_array;
        break;
      default:
        return false;
      }

      return true;
    }
    rep_value = rep_value->next;
  }

  return false;
}

bool
oc_rep_get_int(oc_rep_t *rep, const char *key, int64_t *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_INT, key, (void **)&value,
                          (size_t *)NULL);
}

bool
oc_rep_i_get_int(oc_rep_t *rep, int key, int64_t *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_INT, key, (void **)&value,
                            (size_t *)NULL);
}

bool
oc_rep_get_bool(oc_rep_t *rep, const char *key, bool *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_BOOL, key, (void **)&value,
                          (size_t *)NULL);
}

bool
oc_rep_i_get_bool(oc_rep_t *rep, int key, bool *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_BOOL, key, (void **)&value,
                            (size_t *)NULL);
}

bool
oc_rep_get_float(oc_rep_t *rep, const char *key, float *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_FLOAT, key, (void **)&value,
                          (size_t *)NULL);
}

bool
oc_rep_i_get_float(oc_rep_t *rep, int key, float *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_FLOAT, key, (void **)&value,
                            (size_t *)NULL);
}

bool
oc_rep_get_double(oc_rep_t *rep, const char *key, double *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_DOUBLE, key, (void **)&value,
                          (size_t *)NULL);
}

bool
oc_rep_i_get_double(oc_rep_t *rep, int key, double *value)
{
  if (!value) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_DOUBLE, key, (void **)&value,
                            (size_t *)NULL);
}

bool
oc_rep_get_byte_string(oc_rep_t *rep, const char *key, char **value,
                       size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_BYTE_STRING, key, (void **)value, size);
}

bool
oc_rep_i_get_byte_string(oc_rep_t *rep, int key, char **value, size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_BYTE_STRING, key, (void **)value, size);
}

bool
oc_rep_get_string(oc_rep_t *rep, const char *key, char **value, size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_STRING, key, (void **)value, size);
}

bool
oc_rep_i_get_string(oc_rep_t *rep, int key, char **value, size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_STRING, key, (void **)value, size);
}

bool
oc_rep_get_int_array(oc_rep_t *rep, const char *key, int64_t **value,
                     size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_INT_ARRAY, key, (void **)value, size);
}

bool
oc_rep_i_get_int_array(oc_rep_t *rep, int key, int64_t **value, size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_INT_ARRAY, key, (void **)value, size);
}

bool
oc_rep_get_bool_array(oc_rep_t *rep, const char *key, bool **value,
                      size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_BOOL_ARRAY, key, (void **)value, size);
}

bool
oc_rep_i_get_bool_array(oc_rep_t *rep, int key, bool **value, size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_BOOL_ARRAY, key, (void **)value, size);
}

bool
oc_rep_get_float_array(oc_rep_t *rep, const char *key, float **value,
                       size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_FLOAT_ARRAY, key, (void **)value, size);
}

bool
oc_rep_i_get_float_array(oc_rep_t *rep, int key, float **value, size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_FLOAT_ARRAY, key, (void **)value, size);
}

bool
oc_rep_get_double_array(oc_rep_t *rep, const char *key, double **value,
                        size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_DOUBLE_ARRAY, key, (void **)value, size);
}

bool
oc_rep_i_get_double_array(oc_rep_t *rep, int key, double **value, size_t *size)
{
  if (!size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_DOUBLE_ARRAY, key, (void **)value,
                            size);
}

bool
oc_rep_get_byte_string_array(oc_rep_t *rep, const char *key,
                             oc_string_array_t *value, size_t *size)
{
  if (!value || !size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_BYTE_STRING_ARRAY, key, (void **)&value,
                          size);
}

bool
oc_rep_i_get_byte_string_array(oc_rep_t *rep, int key, oc_string_array_t *value,
                               size_t *size)
{
  if (!value || !size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_BYTE_STRING_ARRAY, key, (void **)&value,
                            size);
}

bool
oc_rep_get_string_array(oc_rep_t *rep, const char *key,
                        oc_string_array_t *value, size_t *size)
{
  if (!value || !size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_get_value(rep, OC_REP_STRING_ARRAY, key, (void **)&value, size);
}

bool
oc_rep_i_get_string_array(oc_rep_t *rep, int key, oc_string_array_t *value,
                          size_t *size)
{
  if (!value || !size) {
    OC_ERR("Error of input parameters");
    return false;
  }
  return oc_rep_i_get_value(rep, OC_REP_STRING_ARRAY, key, (void **)&value,
                            size);
}

bool
oc_rep_get_object(oc_rep_t *rep, const char *key, oc_rep_t **value)
{
  return oc_rep_get_value(rep, OC_REP_OBJECT, key, (void **)value, NULL);
}

bool
oc_rep_i_get_object(oc_rep_t *rep, int key, oc_rep_t **value)
{
  return oc_rep_i_get_value(rep, OC_REP_OBJECT, key, (void **)value, NULL);
}

bool
oc_rep_get_object_array(oc_rep_t *rep, const char *key, oc_rep_t **value)
{
  return oc_rep_get_value(rep, OC_REP_OBJECT_ARRAY, key, (void **)value, NULL);
}

bool
oc_rep_i_get_object_array(oc_rep_t *rep, int key, oc_rep_t **value)
{
  return oc_rep_i_get_value(rep, OC_REP_OBJECT_ARRAY, key, (void **)value,
                            NULL);
}

bool
oc_rep_get_mixed_array(oc_rep_t *rep, const char *key, oc_rep_t **value)
{
  return oc_rep_get_value(rep, OC_REP_MIXED_ARRAY, key, (void **)value, NULL);
}

bool
oc_rep_i_get_mixed_array(oc_rep_t *rep, int key, oc_rep_t **value)
{
  return oc_rep_i_get_value(rep, OC_REP_MIXED_ARRAY, key, (void **)value, NULL);
}

/*
 * This macro assumes that four variables are already available to be changed.
 *
 *  - total_char_printed = running total of characters printed to buf
 *  - num_char_printed = the number of characters the command just before the
 *                       macro is called reports is printed typically the return
 *                       value of snprintf function but not always.
 *  - buf = the character buffer being updated
 *  - buf_size = the size of the character buffer being updated.
 *
 * Tracking the total number characters and moving the pointer forward in the
 * buffer so it the addition of each string looks like concatenation means
 * moving the buf pointer forward and reducing the buf_size after every function
 * that adds to the buffer.
 *
 * In addition it will update the total character count that would be printed
 * regardless of the buf_size. (total_char_print is expected to be larger than
 * or equal to buf_size if the buffer is too small.)
 */
#define OC_JSON_UPDATE_BUFFER_AND_TOTAL                                        \
  do {                                                                         \
    total_char_printed += num_char_printed;                                    \
    if (num_char_printed < buf_size && buf != NULL) {                          \
      buf += num_char_printed;                                                 \
      buf_size -= num_char_printed;                                            \
    } else {                                                                   \
      buf += buf_size;                                                         \
      buf_size = 0;                                                            \
    }                                                                          \
  } while (0);

/*
 * Internal function used to complete the oc_rep_to_json function
 *
 * This function is used when pretty_print parameter of the oc_rep_to_json
 * function is set to true. It helps produce output with reasonably human
 * readable white-space.
 */
size_t
oc_rep_to_json_tab(char *buf, size_t buf_size, int tab_depth)
{
  size_t num_char_printed = 0;
  size_t total_char_printed = 0;
  for (int i = 0; i < tab_depth; i++) {
    num_char_printed =
      snprintf(buf, buf_size, "%s", OC_PRETTY_PRINT_TAB_CHARACTER);
    OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  }
  return total_char_printed;
}

/*
 * Internal function used to complete the oc_rep_to_json function
 *
 * This function is called when the data type is an OC_REP_BYTE_STRING or
 * an OC_REP_BYTE_STRING_ARRAY. If uses the base64 encoded to encode the
 * byte_string to a base64 string.
 */
size_t
oc_rep_to_json_base64_encoded_byte_string(char *buf, size_t buf_size,
                                          char *byte_str, size_t byte_str_size)
{
  size_t num_char_printed = 0;
  size_t total_char_printed = 0;
  // calculate the b64 encoded string size
  size_t b64_buf_size = (byte_str_size / 3) * 4;
  if (byte_str_size % 3 != 0) {
    b64_buf_size += 4;
  }
  // one extra byte for terminating NUL character.
  b64_buf_size++;
  num_char_printed = snprintf(buf, buf_size, "\"");
  OC_JSON_UPDATE_BUFFER_AND_TOTAL;

  if (buf_size > b64_buf_size) {
    int output_len = oc_base64_encode((uint8_t *)byte_str, byte_str_size,
                                      (uint8_t *)buf, b64_buf_size);
    num_char_printed = output_len;
    OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  } else {
    buf += buf_size;
    buf_size = 0;
    total_char_printed += (b64_buf_size - 1);
  }
  num_char_printed = snprintf(buf, buf_size, "\"");
  OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  return total_char_printed;
}

/*
 * Internal function called by oc_rep_to_json function
 *
 * oc_rep_to_json_format will take any oc_rep_t and print out the json
 * equivalent of that value.  This function will be called recursively for
 * nested objects.
 *
 * Currently does not handle OC_REP_ARRAY data type.
 */
size_t
oc_rep_to_json_format(oc_rep_t *rep, char *buf, size_t buf_size, int tab_depth,
                      bool pretty_print)
{
  (void)buf;
  (void)buf_size;
  size_t num_char_printed = 0;
  size_t total_char_printed = 0;
  while (rep != NULL) {
    if (pretty_print) {
      num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 1);
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
    }

    if (oc_string_len(rep->name) > 0) {
      num_char_printed =
        (pretty_print)
          ? snprintf(buf, buf_size, "\"%s\" : ", oc_string_checked(rep->name))
          : snprintf(buf, buf_size, "\"%s\":", oc_string_checked(rep->name));
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
    } else {
      // this should only print if iname >= 0
      // if its <0 then it't NOT an object member
      if ((rep->iname >= 0)) {
        num_char_printed = (pretty_print)
                             ? snprintf(buf, buf_size, "\"%d\" : ", rep->iname)
                             : snprintf(buf, buf_size, "\"%d\":", rep->iname);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }
    }
    switch (rep->type) {
    case OC_REP_NIL: {
      num_char_printed = snprintf(buf, buf_size, "null");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_INT: {
#ifdef KNX_LOG_CAST_64_BIT_INTS
      num_char_printed = snprintf(buf, buf_size, "%d", (int)rep->value.integer);
#else
      num_char_printed =
        snprintf(buf, buf_size, "%" PRId64, rep->value.integer);
#endif

      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_FLOAT: {
      num_char_printed = snprintf(buf, buf_size, "%f", rep->value.float_p);
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_DOUBLE: {
      num_char_printed = snprintf(buf, buf_size, "%f", rep->value.double_p);
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_BOOL: {
      num_char_printed =
        snprintf(buf, buf_size, "%s", (rep->value.boolean) ? "true" : "false");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_BYTE_STRING: {
      char *byte_string = NULL;
      size_t byte_string_size;
      if (oc_string(rep->name) != NULL) {
        oc_rep_get_byte_string(rep, oc_string(rep->name), &byte_string,
                               &byte_string_size);
      } else {
        oc_rep_i_get_byte_string(rep, rep->iname, &byte_string,
                                 &byte_string_size);
      }
      num_char_printed = oc_rep_to_json_base64_encoded_byte_string(
        buf, buf_size, byte_string, byte_string_size);
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_STRING: {
      // oc_string_array_get_item
      num_char_printed =
        snprintf(buf, buf_size, "\"%s\"", oc_string_checked(rep->value.string));
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_OBJECT: {
      num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "{\n")
                                        : snprintf(buf, buf_size, "{");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      num_char_printed = oc_rep_to_json_format(rep->value.object, buf, buf_size,
                                               tab_depth + 1, pretty_print);
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      if (pretty_print) {
        num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 1);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }
      num_char_printed = snprintf(buf, buf_size, "}");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_INT_ARRAY: {
      num_char_printed = snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      int64_t *int_array;
      size_t int_array_size = 0;
      if (oc_string(rep->name) != NULL) {
        oc_rep_get_int_array(rep, oc_string(rep->name), &int_array,
                             &int_array_size);
      } else {
        oc_rep_i_get_int_array(rep, rep->iname, &int_array, &int_array_size);
      }
      for (size_t i = 0; i < int_array_size; i++) {
#ifdef KNX_LOG_CAST_64_BIT_INTS
        num_char_printed = snprintf(buf, buf_size, "%d", (int)int_array[i]);
#else
        num_char_printed = snprintf(buf, buf_size, "%" PRId64, int_array[i]);
#endif

        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        if (i < int_array_size - 1) {
          num_char_printed = (pretty_print) ? snprintf(buf, buf_size, ", ")
                                            : snprintf(buf, buf_size, ",");
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
      }
      num_char_printed = snprintf(buf, buf_size, "]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_FLOAT_ARRAY: {
      num_char_printed = snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      float *float_array;
      size_t float_array_size = 0;
      if (oc_string(rep->name) != NULL) {
        oc_rep_get_float_array(rep, oc_string(rep->name), &float_array,
                               &float_array_size);
      } else {
        oc_rep_i_get_float_array(rep, rep->iname, &float_array,
                                 &float_array_size);
      }
      for (size_t i = 0; i < float_array_size; i++) {
        num_char_printed = snprintf(buf, buf_size, "%f", float_array[i]);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        if (i < float_array_size - 1) {
          num_char_printed = (pretty_print) ? snprintf(buf, buf_size, ", ")
                                            : snprintf(buf, buf_size, ",");
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
      }
      num_char_printed = snprintf(buf, buf_size, "]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_DOUBLE_ARRAY: {
      num_char_printed = snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      double *double_array;
      size_t double_array_size = 0;
      if (oc_string(rep->name) != NULL) {
        oc_rep_get_double_array(rep, oc_string(rep->name), &double_array,
                                &double_array_size);
      } else {
        oc_rep_i_get_double_array(rep, rep->iname, &double_array,
                                  &double_array_size);
      }
      for (size_t i = 0; i < double_array_size; i++) {
        num_char_printed = snprintf(buf, buf_size, "%f", double_array[i]);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        if (i < double_array_size - 1) {
          num_char_printed = (pretty_print) ? snprintf(buf, buf_size, ", ")
                                            : snprintf(buf, buf_size, ",");
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
      }
      num_char_printed = snprintf(buf, buf_size, "]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_BOOL_ARRAY: {
      num_char_printed = snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      bool *bool_array;
      size_t bool_array_size = 0;
      if (oc_string(rep->name) != NULL) {
        oc_rep_get_bool_array(rep, oc_string(rep->name), &bool_array,
                              &bool_array_size);
      } else {
        oc_rep_i_get_bool_array(rep, rep->iname, &bool_array, &bool_array_size);
      }
      // oc_rep_get_bool_array(rep, oc_string(rep->name), &bool_array,
      //                      &bool_array_size);
      for (size_t i = 0; i < bool_array_size; i++) {
        num_char_printed =
          snprintf(buf, buf_size, "%s", (bool_array[i]) ? "true" : "false");
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        if (i < bool_array_size - 1) {
          num_char_printed = (pretty_print) ? snprintf(buf, buf_size, ", ")
                                            : snprintf(buf, buf_size, ",");
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
      }
      num_char_printed = snprintf(buf, buf_size, "]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_BYTE_STRING_ARRAY: {
      num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "[\n")
                                        : snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      oc_string_array_t byte_str_array;
      size_t byte_str_array_size = 0;
      if (oc_string(rep->name) != NULL) {
        oc_rep_get_byte_string_array(rep, oc_string(rep->name), &byte_str_array,
                                     &byte_str_array_size);
      } else {
        oc_rep_i_get_byte_string_array(rep, rep->iname, &byte_str_array,
                                       &byte_str_array_size);
      }
      // oc_rep_get_byte_string_array(rep, oc_string(rep->name),
      // &byte_str_array,
      //                             &byte_str_array_size);
      for (size_t i = 0; i < byte_str_array_size; i++) {
        if (pretty_print) {
          num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 2);
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
        char *byte_string = oc_byte_string_array_get_item(byte_str_array, i);
        size_t byte_string_size =
          oc_byte_string_array_get_item_size(byte_str_array, i);
        num_char_printed = oc_rep_to_json_base64_encoded_byte_string(
          buf, buf_size, byte_string, byte_string_size);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        if (i < byte_str_array_size - 1) {
          num_char_printed = (pretty_print) ? snprintf(buf, buf_size, ",\n")
                                            : snprintf(buf, buf_size, ",");
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        } else {
          if (pretty_print) {
            num_char_printed = snprintf(buf, buf_size, "\n");
            OC_JSON_UPDATE_BUFFER_AND_TOTAL;
          }
        }
      }
      if (pretty_print) {
        num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 1);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }
      num_char_printed = snprintf(buf, buf_size, "]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_STRING_ARRAY: {
      num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "[\n")
                                        : snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      oc_string_array_t str_array;
      size_t str_array_size = 0;
      if (oc_string(rep->name) != NULL) {
        oc_rep_get_string_array(rep, oc_string(rep->name), &str_array,
                                &str_array_size);
      } else {
        oc_rep_i_get_string_array(rep, rep->iname, &str_array, &str_array_size);
      }
      // oc_rep_get_string_array(rep, oc_string(rep->name), &str_array,
      //                        &str_array_size);
      for (size_t i = 0; i < str_array_size; i++) {
        if (pretty_print) {
          num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 2);
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
        num_char_printed = snprintf(buf, buf_size, "\"%s\"",
                                    oc_string_array_get_item(str_array, i));
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        if (i < str_array_size - 1) {
          num_char_printed = (pretty_print) ? snprintf(buf, buf_size, ",\n")
                                            : snprintf(buf, buf_size, ",");
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        } else {
          if (pretty_print) {
            num_char_printed = snprintf(buf, buf_size, "\n");
            OC_JSON_UPDATE_BUFFER_AND_TOTAL;
          }
        }
      }
      if (pretty_print) {
        num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 1);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }
      num_char_printed = snprintf(buf, buf_size, "]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_OBJECT_ARRAY: {
      num_char_printed = snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      oc_rep_t *rep_array = rep->value.object_array;
      if (pretty_print) {
        num_char_printed = snprintf(buf, buf_size, "\n");
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }
      do {
        oc_rep_t *rep_item = rep_array->value.object;
        if (pretty_print) {
          num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 2);
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
        num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "{\n")
                                          : snprintf(buf, buf_size, "{");
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        num_char_printed = oc_rep_to_json_format(rep_item, buf, buf_size,
                                                 tab_depth + 2, pretty_print);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        rep_array = rep_array->next;
        if (rep_array) {
          if (pretty_print) {
            num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 2);
            OC_JSON_UPDATE_BUFFER_AND_TOTAL;
          }
          num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "},\n")
                                            : snprintf(buf, buf_size, "},");
          OC_JSON_UPDATE_BUFFER_AND_TOTAL;
        }
      } while (rep_array);
      if (pretty_print) {
        num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 2);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }
      num_char_printed = snprintf(buf, buf_size, "}]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    case OC_REP_MIXED_ARRAY: {
      num_char_printed = snprintf(buf, buf_size, "[");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      oc_rep_t *rep_array = rep->value.mixed_array;
      if (pretty_print && rep_array) {
        num_char_printed = snprintf(buf, buf_size, "\n");
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }
      num_char_printed = oc_rep_to_json_format(rep_array, buf, buf_size,
                                               tab_depth + 2, pretty_print);
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      if (pretty_print) {
        num_char_printed = oc_rep_to_json_tab(buf, buf_size, tab_depth + 2);
        OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      }

      num_char_printed = snprintf(buf, buf_size, "]");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
      break;
    }
    default:
      break;
    }
    rep = rep->next;
    if (rep != NULL) {
      num_char_printed = snprintf(buf, buf_size, ",");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
    }
    if (pretty_print) {
      num_char_printed = snprintf(buf, buf_size, "\n");
      OC_JSON_UPDATE_BUFFER_AND_TOTAL;
    }
  }
  return total_char_printed;
}

size_t
oc_rep_to_json(oc_rep_t *rep, char *buf, size_t buf_size, bool pretty_print)
{
  size_t num_char_printed = 0;
  size_t total_char_printed = 0;

  bool object_array =
    (rep && (rep->type == OC_REP_OBJECT) && (oc_string_len(rep->name) == 0));

  num_char_printed = (pretty_print)
                       ? snprintf(buf, buf_size, (object_array) ? "[\n" : "{\n")
                       : snprintf(buf, buf_size, (object_array) ? "[" : "{");
  OC_JSON_UPDATE_BUFFER_AND_TOTAL;

  num_char_printed = oc_rep_to_json_format(rep, buf, buf_size, 0, pretty_print);
  OC_JSON_UPDATE_BUFFER_AND_TOTAL;

  num_char_printed = (pretty_print)
                       ? snprintf(buf, buf_size, (object_array) ? "]\n" : "}\n")
                       : snprintf(buf, buf_size, (object_array) ? "]" : "}");

  OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  return total_char_printed;
}

size_t
py_oc_rep_to_json(oc_rep_t *rep, char *buf, size_t buf_size, bool pretty_print)
{
  size_t num_char_printed = 0;
  size_t total_char_printed = 0;
  char *my_buf = buf;
  int tab = 1;

  bool object_array = (rep && (rep->type == OC_REP_ARRAY) &&
                       (oc_string_len(rep->name) == 0) && (rep->iname == -1));
  bool object = (rep && (rep->type == OC_REP_OBJECT) &&
                 (oc_string_len(rep->name) == 0) && (rep->iname == -1));

  PRINT("===> py_oc_rep_to_json: array:%d object:%d", object_array, object);
  if (rep) {
    PRINT(" type: %d", rep->type);
    if ((rep->type != OC_REP_ARRAY) && (rep->type != OC_REP_OBJECT)) {
      tab = 0;
    }
  }
  PRINT(" tab: %d \n\n", tab);

  // reserve space
  num_char_printed = snprintf(buf, buf_size, " ");
  OC_JSON_UPDATE_BUFFER_AND_TOTAL;

  if (object_array) {
    num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "[\n")
                                      : snprintf(buf, buf_size, "[");
    OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  }
  if (object) {
    num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "{\n")
                                      : snprintf(buf, buf_size, "{");
    OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  }

  num_char_printed =
    oc_rep_to_json_format(rep, buf, buf_size, tab, pretty_print);
  OC_JSON_UPDATE_BUFFER_AND_TOTAL;

  char *found = NULL;
  if (my_buf) {
    found = strchr((const char *)my_buf, ':');
  }
  if (found != NULL) {
    // content is an object
    object = true;
    my_buf[0] = '{';
  } else {
    // check if the contents maybe an array
    char *found_comma = NULL;
    if (my_buf) {
      found_comma = strchr((const char *)my_buf, ',');
    }
    if (found_comma != NULL) {
      // content is an array
      object_array = true;
      my_buf[0] = '[';
    }
  }

  if (object_array) {
    num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "]\n")
                                      : snprintf(buf, buf_size, "]");
    OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  }
  if (object) {
    num_char_printed = (pretty_print) ? snprintf(buf, buf_size, "}\n")
                                      : snprintf(buf, buf_size, "}");
    OC_JSON_UPDATE_BUFFER_AND_TOTAL;
  }

  PRINT(" total printed: %d \n", (int)total_char_printed);
  PRINT("%s\n", my_buf);

  return total_char_printed;
}

/**
 * function to print the returned cbor as JSON
 *
 */
void
oc_print_rep_as_json(oc_rep_t *rep, bool pretty_print)
{
  char *json;
  size_t json_size;
  json_size = oc_rep_to_json(rep, NULL, 0, pretty_print);
  json = (char *)malloc(json_size + 1);
  oc_rep_to_json(rep, json, json_size + 1, pretty_print);
  PRINT("oc_print_rep_as_json:\n");
  PRINT("%s\n", json);
  free(json);
}
