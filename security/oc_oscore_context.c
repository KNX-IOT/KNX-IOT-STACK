/*
// Copyright (c) 2020 Intel Corporation
// Copyright (c) 2022-2023 Cascoda Ltd.
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

#if defined(OC_OSCORE)

#define OC_OSCORE_DBG PRINT

#include "oc_oscore_context.h"
#include "messaging/coap/transactions.h"
#include "oc_api.h"
#include "oc_client_state.h"
//#include "oc_cred.h"
#include "oc_oscore_crypto.h"
#include "api/oc_knx_sec.h"
#include "oc_rep.h"
//#include "oc_store.h"
#include "port/oc_log.h"
OC_LIST(contexts);
OC_MEMB(ctx_s, oc_oscore_context_t, 20);

// checking against receiver in contexts
oc_oscore_context_t *
oc_oscore_find_context_by_kid(oc_oscore_context_t *ctx, size_t device_index,
                              uint8_t *kid, uint8_t kid_len)
{
  if (!ctx) {
    ctx = (oc_oscore_context_t *)oc_list_head(contexts);
  }

  PRINT("oc_oscore_find_context_by_kid : dev=%d  kid:(%d) :", (int)device_index,
        kid_len);
  oc_char_println_hex((char *)(kid), kid_len);

  while (ctx != NULL) {
    PRINT("  ---> recvid:");
    oc_char_println_hex((char *)(ctx->recvid), ctx->recvid_len);

    if (kid_len == ctx->recvid_len && memcmp(kid, ctx->recvid, kid_len) == 0) {
      PRINT("oc_oscore_find_context_by_kid FOUND  auth/at index: %d\n",
            ctx->auth_at_index);
      return ctx;
    }
    ctx = ctx->next;
  }
  return ctx;
}

oc_oscore_context_t *
oc_oscore_find_context_by_token_mid(size_t device, uint8_t *token,
                                    uint8_t token_len, uint16_t mid,
                                    uint8_t **request_piv,
                                    uint8_t *request_piv_len, bool tcp)
{
  (void)device;
  char *oscore_id = NULL;
  int oscore_id_len = 0;
#ifdef OC_CLIENT
  /* Search for client cb by token */
  oc_client_cb_t *cb = oc_ri_find_client_cb_by_token(token, token_len);

  if (cb) {
    if (request_piv && request_piv_len) {
      *request_piv = cb->piv;
      *request_piv_len = cb->piv_len;
    }
    oscore_id = cb->endpoint.oscore_id;
    oscore_id_len = cb->endpoint.oscore_id_len;
  } else {
#endif /* OC_CLIENT */
    /* Search transactions by token and mid */
    coap_transaction_t *t = coap_get_transaction_by_token(token, token_len);
    if (!t) {
      if (!tcp) {
        t = coap_get_transaction_by_mid(mid);
      }
      if (!t) {
        OC_ERR("***could not find matching OSCORE context***");
        return NULL;
      }
    }
    if (request_piv && request_piv_len) {
      *request_piv = t->message->endpoint.piv;
      *request_piv_len = t->message->endpoint.piv_len;
    }
    // serial_number = t->message->endpoint.serial_number;
    oscore_id = t->message->endpoint.oscore_id;
    oscore_id_len = t->message->endpoint.oscore_id_len;
#ifdef OC_CLIENT
  }
#endif /* OC_CLIENT */
  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);

  if (oscore_id_len == 0) {
    OC_ERR("***could not find matching OSCORE context: oscore_id is NULL***");
    return NULL;
  }

  while (ctx != NULL) {
    //  oc_sec_cred_t *cred = (oc_sec_cred_t *)ctx->cred;
    //  if (memcmp(cred->subjectuuid.id, uuid->id, 16) == 0 &&
    //      ctx->device == device) {
    //    return ctx;
    //   }
    char *ctx_serial_number = ctx->token_id;
    if (memcmp(oscore_id, ctx_serial_number, oscore_id_len) == 0) {
      PRINT("oc_oscore_find_context_by_token_mid FOUND auth/at index: %d\n",
            ctx->auth_at_index);
      return ctx;
    }
    ctx = ctx->next;
  }
  return ctx;
}

oc_oscore_context_t *
oc_oscore_find_context_by_oscore_id(size_t device, char *oscore_id,
                                    size_t oscore_id_len)
{
  (void)device;
  int cmp_len = 16;

  if (oscore_id_len > 16) {
    OC_ERR("oscore_id longer than 16: %d\n", (int)oscore_id_len);
    return NULL;
  }

  if (oscore_id_len == 0) {
    OC_ERR("oscore_id_len == 0\n");
    return NULL;
  }
  if (oscore_id == NULL) {
    OC_ERR("oscore_id NULL\n");
    return NULL;
  }
  if (oscore_id_len < 16) {
    cmp_len = oscore_id_len;
  }

  PRINT("oc_oscore_find_context_by_oscore_id:");
  oc_char_println_hex(oscore_id, oscore_id_len);

  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);
  while (ctx != NULL) {
    char *ctx_serial_number = ctx->token_id;
    if (memcmp(oscore_id, ctx_serial_number, cmp_len) == 0) {
      PRINT("oc_oscore_find_context_by_oscore_id FOUND auth/at index: %d\n",
            ctx->auth_at_index);
      OC_DBG_OSCORE("    Common IV:");
      OC_LOGbytes_OSCORE(ctx->commoniv, OSCORE_COMMON_IV_LEN);
      return ctx;
    }
    ctx = ctx->next;
  }
  PRINT("  NOT FOUND\n");
  return ctx;
}

oc_oscore_context_t *
oc_oscore_find_context_by_rid(size_t device, char *rid, size_t rid_len)
{
  (void)device;
  int cmp_len = 16;

  if (rid_len > 16) {
    OC_ERR("rid longer than 16: %d\n", (int)rid_len);
    return NULL;
  }

  if (rid_len == 0) {
    OC_ERR("rid == 0\n");
    return NULL;
  }
  if (rid == NULL) {
    OC_ERR("rid NULL\n");
    return NULL;
  }
  if (rid_len < 16) {
    cmp_len = rid_len;
  }

  PRINT("oc_oscore_find_context_by_rid:");
  oc_char_println_hex(rid, rid_len);

  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);
  while (ctx != NULL) {
    char *ctx_recvid = ctx->recvid;
    if (memcmp(rid, ctx_recvid, cmp_len) == 0) {
      PRINT("oc_oscore_find_context_by_rid FOUND auth/at index: %d\n",
            ctx->auth_at_index);
      OC_DBG_OSCORE("    Common IV:");
      OC_LOGbytes_OSCORE(ctx->commoniv, OSCORE_COMMON_IV_LEN);
      return ctx;
    }
    ctx = ctx->next;
  }
  PRINT("  NOT FOUND\n");
  return ctx;
}

oc_oscore_context_t *
oc_oscore_find_context_by_group_address(size_t device, uint32_t group_address)
{
  (void)device;

  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);

  while (ctx != NULL) {
    oc_auth_at_t *my_entry = oc_get_auth_at_entry(0, ctx->auth_at_index);
    if (my_entry) {
      oc_print_auth_at_entry(0, ctx->auth_at_index);
      for (int i = 0; i < my_entry->ga_len; i++) {

        uint32_t group_value = my_entry->ga[i];
        PRINT(
          "   oc_oscore_find_context_by_group_address : find: %u value: %u\n",
          group_address, group_value);
        if (group_address == group_value) {
          return ctx;
        }
      }
    }
    ctx = ctx->next;
  }
  return ctx;
}

void
oc_oscore_free_all_contexts()
{
  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);
  while (ctx != NULL) {
    oc_oscore_context_t *next = ctx->next;
    oc_oscore_free_context(ctx);
    ctx = next;
  }
  oc_list_init(contexts);
}

void
oc_oscore_free_context(oc_oscore_context_t *ctx)
{
  if (ctx) {
    if (ctx->desc.size > 0) {
      oc_free_string(&ctx->desc);
    }
    oc_list_remove(contexts, ctx);
    oc_memb_free(&ctx_s, ctx);
  }
}

oc_oscore_context_t *
oc_oscore_add_context(size_t device, const char *senderid, int senderid_size,
                      const char *recipientid, int recipientid_size,
                      uint64_t ssn, const char *desc, const char *mastersecret,
                      int mastersecret_size, const char *osc_ctx,
                      int osc_ctx_size, int auth_at_index, bool from_storage)
{
  PRINT("-----oc_oscore_add_context--SID:");
  oc_char_println_hex(senderid, senderid_size);
  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_memb_alloc(&ctx_s);

  if (!ctx) {
    OC_ERR("No memory for allocating context!!!");
    return NULL;
  }
  if (!senderid && !recipientid && !mastersecret) {
    OC_ERR("No sender or recipient ID or Master secret");
    return NULL;
  }
  if (mastersecret_size != OSCORE_IDCTX_LEN) {
    OC_ERR("master secret size is != %d : %d", OSCORE_IDCTX_LEN,
           mastersecret_size);
    return NULL;
  }

  if (senderid_size > OSCORE_CTXID_LEN) {
    OC_ERR("senderid_size > %d = %d", OSCORE_CTXID_LEN, senderid_size);
    return NULL;
  }
  if (recipientid_size > OSCORE_CTXID_LEN) {
    OC_ERR("recipientid_size > %d = %d", OSCORE_CTXID_LEN, recipientid_size);
    return NULL;
  }
  if (osc_ctx_size > OSCORE_IDCTX_LEN) {
    OC_ERR("osc_ctx_size > %d = %d", OSCORE_IDCTX_LEN, osc_ctx_size);
    return NULL;
  }

  ctx->device = device;
  ctx->ssn = ssn;
  ctx->auth_at_index = auth_at_index;

  PRINT("  device    : %d\n", (int)device);
  PRINT("  desc      : %s\n", desc);
  PRINT("  index     : %d\n", auth_at_index);
  PRINT("  sid size  : %d ", senderid_size);
  oc_char_println_hex(senderid, senderid_size);
  PRINT("  rid size  : %d ", recipientid_size);
  oc_char_println_hex(recipientid, recipientid_size);
  PRINT("  ctx size  : %d\n", osc_ctx_size);
  PRINT("  ms size   : %d ", mastersecret_size);
  oc_char_println_hex(mastersecret, mastersecret_size);

  /* To prevent SSN reuse, bump to higher value that could've been previously
   * used, accounting for any failed writes to nonvolatile storage.
   */
  if (from_storage) {
    ctx->ssn += OSCORE_SSN_WRITE_FREQ_K + OSCORE_SSN_PAD_F;
  }
  PRINT("  ssn       %" PRIu64 "\n", ctx->ssn);
  if (desc) {
    oc_new_string(&ctx->desc, desc, strlen(desc));
  }
  size_t id_len = OSCORE_CTXID_LEN;

  if (senderid && senderid_size > 0) {
    // if (oc_conv_hex_string_to_byte_array(senderid, senderid_size,
    //                                      ctx->sendid, &id_len) < 0) {
    //   goto add_oscore_context_error;
    // }
    memcpy(ctx->sendid, senderid, senderid_size);
    // explicit token
    memcpy(ctx->token_id, senderid, senderid_size);
    ctx->sendid_len = (uint8_t)senderid_size;
  } else {
    OC_ERR("senderid == NULL");
    goto add_oscore_context_error;
  }
  PRINT("SendID (%d):", ctx->sendid_len);
  OC_LOGbytes_OSCORE(ctx->sendid, ctx->sendid_len);

  id_len = OSCORE_CTXID_LEN;

  if (recipientid && recipientid_size > 0) {
    // if (oc_conv_hex_string_to_byte_array(recipientid, recipientid_size,
    //                                      ctx->recvid, &id_len) < 0) {
    //   goto add_oscore_context_error;
    // }
    memcpy(ctx->recvid, recipientid, recipientid_size);
    ctx->recvid_len = (uint8_t)recipientid_size;
  } else {
    OC_ERR("recipientid == NULL");
    goto add_oscore_context_error;
  }
  PRINT("RecvID (%d):", ctx->recvid_len);
  OC_LOGbytes_OSCORE(ctx->recvid, ctx->recvid_len);

  if (osc_ctx && osc_ctx_size > 0) {
    // if (oc_conv_hex_string_to_byte_array(osc_ctx, osc_ctx_size, ctx->idctx,
    //                                      &id_len) < 0) {
    //   goto add_oscore_context_error;
    // }
    memcpy(ctx->idctx, osc_ctx, osc_ctx_size);
    ctx->idctx_len = (uint8_t)osc_ctx_size;
  }
  PRINT("OSC CTX (%d):", ctx->idctx_len);
  OC_LOGbytes_OSCORE(ctx->idctx, ctx->idctx_len);

  if (mastersecret) {
    memcpy((char *)&ctx->master_secret, mastersecret, mastersecret_size);
  }

  OC_DBG_OSCORE("### Reading OSCORE context ###");
  if (senderid) {
    OC_DBG_OSCORE("### \t\tderiving Sender key ###");
    if (oc_oscore_context_derive_param(
          ctx->sendid, ctx->sendid_len, ctx->idctx, ctx->idctx_len, "Key",
          (uint8_t *)mastersecret, mastersecret_size, NULL, 0, ctx->sendkey,
          OSCORE_KEY_LEN) < 0) {
      OC_ERR("*** error deriving Sender key ###");
      goto add_oscore_context_error;
    }

    OC_DBG_OSCORE("### derived Sender key ###");
  }
  PRINT("SEND_KEY:");
  oc_char_println_hex(ctx->sendkey, OSCORE_KEY_LEN);
  OC_LOGbytes_OSCORE(ctx->sendkey, OSCORE_KEY_LEN);

  if (recipientid) {
    OC_DBG_OSCORE("### \t\tderiving Recipient key ###");
    if (oc_oscore_context_derive_param(
          ctx->recvid, ctx->recvid_len, ctx->idctx, ctx->idctx_len, "Key",
          (uint8_t *)mastersecret, mastersecret_size, NULL, 0, ctx->recvkey,
          OSCORE_KEY_LEN) < 0) {
      OC_ERR("*** error deriving Recipient key ###");
      goto add_oscore_context_error;
    }

    OC_DBG_OSCORE("### derived Recipient key ###");
  }
  PRINT("RCV_KEY:");
  oc_char_println_hex(ctx->recvkey, OSCORE_KEY_LEN);
  OC_LOGbytes_OSCORE(ctx->recvkey, OSCORE_KEY_LEN);

  OC_DBG_OSCORE("### \t\tderiving Common IV ###");
  if (oc_oscore_context_derive_param(
        NULL, 0, ctx->idctx, ctx->idctx_len, "IV", (uint8_t *)mastersecret,
        mastersecret_size, NULL, 0, ctx->commoniv, OSCORE_COMMON_IV_LEN) < 0) {
    OC_ERR("*** error deriving Common IV ###");
    goto add_oscore_context_error;
  }
  PRINT("IV:");
  oc_char_println_hex(ctx->commoniv, OSCORE_COMMON_IV_LEN);
  OC_LOGbytes_OSCORE(ctx->commoniv, OSCORE_COMMON_IV_LEN);
  OC_DBG_OSCORE("### derived Common IV ###");

  oc_list_add(contexts, ctx);

  return ctx;

add_oscore_context_error:
  OC_DBG_OSCORE("Encountered error while adding new context!");
  oc_memb_free(&ctx_s, ctx);
  return NULL;
}

int
oc_oscore_context_derive_param(const uint8_t *id, uint8_t id_len,
                               uint8_t *id_ctx, uint8_t id_ctx_len,
                               const char *type, uint8_t *secret,
                               uint8_t secret_len, uint8_t *salt,
                               uint8_t salt_len, uint8_t *param,
                               uint8_t param_len)
{
  uint8_t info[OSCORE_INFO_MAX_LEN];
  CborEncoder e, a;
  CborError err = CborNoError;

  /* From RFC 8613: Section 3.2.1:
      info = [
        id : bstr,
        id_context : bstr / nil,
        alg_aead : int / tstr,
        type : tstr,
        L : uint,
      ]
  */
  cbor_encoder_init(&e, info, OSCORE_INFO_MAX_LEN, 0);
  /* Array of 5 elements */
  err |= cbor_encoder_create_array(&e, &a, 5);
  /* Sender ID, Recipient ID or empty string for Common IV */
  err |= cbor_encode_byte_string(&a, id, id_len);
  /* id_context or null if not provided */
  if (id_ctx_len > 0) {
    err |= cbor_encode_byte_string(&a, id_ctx, id_ctx_len);
  } else {
    err |= cbor_encode_null(&a);
  }
  /* alg_aead for AES-CCM-16-64-128 = 10 from RFC 8152 */
  err |= cbor_encode_int(&a, 10);
  /* type: "Key" or "IV" based on deriving a key of the Common IV */
  err |= cbor_encode_text_string(&a, type, strlen(type));
  /* Size of the key/nonce for the AEAD Algorithm used, in bytes */
  err |= cbor_encode_uint(&a, param_len);
  err |= cbor_encoder_close_container(&e, &a);

  if (err != CborNoError) {
    return -1;
  }

  return HKDF_SHA256(salt, salt_len, secret, secret_len, info,
                     cbor_encoder_get_buffer_size(&e, info), param, param_len);
}

#else  /* OC_OSCORE */
typedef int dummy_declaration;
#endif /* !OC_OSCORE */
