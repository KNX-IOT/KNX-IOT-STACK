/*
// Copyright (c) 2020 Intel Corporation
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
OC_MEMB(ctx_s, oc_oscore_context_t, 1);

// checking against receiver...
oc_oscore_context_t *
oc_oscore_find_context_by_kid(oc_oscore_context_t *ctx, size_t device,
                              uint8_t *kid, uint8_t kid_len)
{
  if (!ctx) {
    ctx = (oc_oscore_context_t *)oc_list_head(contexts);
  }

  PRINT("oc_oscore_find_context_by_kid : %d\n  kidlen: %d Kid:", (int)device,
        kid_len);
  OC_LOGbytes_OSCORE(kid, kid_len);

  while (ctx != NULL) {

    PRINT("  ---> : =%s=\n  recvid:", (char *)(ctx->token_id));
    OC_LOGbytes_OSCORE(ctx->recvid, ctx->recvid_len);

    if (kid_len == ctx->recvid_len && memcmp(kid, ctx->recvid, kid_len) == 0) {
      PRINT("  FOUND\n");
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
  char *serial_number = NULL;
#ifdef OC_CLIENT
  /* Search for client cb by token */
  oc_client_cb_t *cb = oc_ri_find_client_cb_by_token(token, token_len);

  if (cb) {
    *request_piv = cb->piv;
    *request_piv_len = cb->piv_len;
    serial_number = cb->endpoint.serial_number;
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
    *request_piv = t->message->endpoint.piv;
    *request_piv_len = t->message->endpoint.piv_len;
    serial_number = t->message->endpoint.serial_number;
#ifdef OC_CLIENT
  }
#endif /* OC_CLIENT */
  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);

  if (serial_number == NULL) {
    OC_ERR(
      "***could not find matching OSCORE context: serial number is NULL***");
    return NULL;
  }

  while (ctx != NULL) {
    //  oc_sec_cred_t *cred = (oc_sec_cred_t *)ctx->cred;
    //  if (memcmp(cred->subjectuuid.id, uuid->id, 16) == 0 &&
    //      ctx->device == device) {
    //    return ctx;
    //   }
    char *ctx_serial_number = ctx->token_id;
    if (strncmp(serial_number, ctx_serial_number, 16) == 0) {
      PRINT("  FOUND\n");
      return ctx;
    }
    ctx = ctx->next;
  }
  return ctx;
}

oc_oscore_context_t *
oc_oscore_find_context_by_serial_number(size_t device, char *serial_number)
{
  (void)device;

  if (strlen(serial_number) == 0) {
    OC_ERR("SERIAL number NULL\n");
    return NULL;
  }
  PRINT("oc_oscore_find_context_by_serial_number '%s'\n", serial_number);

  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);
  while (ctx != NULL) {
    char *ctx_serial_number = ctx->token_id;
    if (strncmp(serial_number, ctx_serial_number, 16) == 0) {
      PRINT("  FOUND\n");
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
oc_oscore_find_context_by_group_id(size_t device, int group_id)
{
  (void)device;

  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_list_head(contexts);

  while (ctx != NULL) {
    oc_auth_at_t *my_entry = oc_get_auth_at_entry(0, ctx->aut_at_index);
    if (my_entry) {
      oc_print_auth_at_entry(0, ctx->aut_at_index);
      for (int i = 0; i < my_entry->ga_len; i++) {

        int64_t group_value = my_entry->ga[i];
        PRINT("   oc_oscore_find_context_by_group_id : %d %d\n", group_id,
              group_value);
        if (group_id == (int)group_value) {
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
oc_oscore_add_context(size_t device, const char *senderid,
                      const char *recipientid, uint64_t ssn, const char *desc,
                      const char *mastersecret, const char *token_id,
                      int auth_at_index,
                      bool from_storage)
{
  oc_oscore_context_t *ctx = (oc_oscore_context_t *)oc_memb_alloc(&ctx_s);

  if (!ctx || (!senderid && !recipientid)) {
    return NULL;
  }

  PRINT("-----oc_oscore_add_context---%s\n", token_id);

  ctx->device = device;
  ctx->ssn = ssn;
  ctx->aut_at_index = auth_at_index;

  PRINT("  device    %d\n", (int)device);
  PRINT("  sender    %s\n", senderid);
  PRINT("  recipient %s\n", recipientid);
  PRINT("  desc      %s\n", desc);
  PRINT("  token_id  %s\n", token_id);
  PRINT("  ssn       %d\n", (int)ssn);
  PRINT("  ms      ");
  int length = strlen(mastersecret);
  for (int i = 0; i < length; i++) {
    PRINT("%02x", (unsigned char)mastersecret[i]);
  }
  PRINT("\n");
  PRINT("  desc    %s\n", desc);
  PRINT("  index   %d\n", auth_at_index);

  /* To prevent SSN reuse, bump to higher value that could've been previously
   * used, accounting for any failed writes to nonvolatile storage.
   */
  if (from_storage) {
    ctx->ssn += OSCORE_SSN_WRITE_FREQ_K + OSCORE_SSN_PAD_F;
  }
  // ctx->cred = cred_entry;
  if (desc) {
    oc_new_string(&ctx->desc, desc, strlen(desc));
  }
  size_t id_len = OSCORE_CTXID_LEN;

  if (senderid) {
    if (oc_conv_hex_string_to_byte_array(senderid, strlen(senderid),
                                         ctx->sendid, &id_len) < 0) {
      goto add_oscore_context_error;
    }

    ctx->sendid_len = (uint8_t)id_len;
  }
  PRINT("SendID:");
  OC_LOGbytes_OSCORE(ctx->sendid, ctx->sendid_len);

  id_len = OSCORE_CTXID_LEN;

  if (recipientid) {
    if (oc_conv_hex_string_to_byte_array(recipientid, strlen(recipientid),
                                         ctx->recvid, &id_len) < 0) {
      goto add_oscore_context_error;
    }

    ctx->recvid_len = (uint8_t)id_len;
  }
  PRINT("RecvID:");
  OC_LOGbytes_OSCORE(ctx->recvid, ctx->recvid_len);

  if (token_id) {
    strncpy((char *)&ctx->token_id, token_id, 16);
  }

  if (mastersecret) {
    strncpy((char *)&ctx->master_secret, mastersecret, 16);
  }

  // oc_sec_cred_t *cred = (oc_sec_cred_t *)cred_entry;

  OC_DBG_OSCORE("### Reading OSCORE context ###");
  if (senderid) {
    OC_DBG_OSCORE("### \t\tderiving Sender key ###");
    if (oc_oscore_context_derive_param(
          ctx->sendid, ctx->sendid_len, ctx->idctx, ctx->idctx_len, "Key",
          (uint8_t *)mastersecret, strlen(mastersecret), NULL, 0, ctx->sendkey,
          OSCORE_KEY_LEN) < 0) {
      OC_ERR("*** error deriving Sender key ###");
      goto add_oscore_context_error;
    }

    OC_DBG_OSCORE("### derived Sender key ###");
  }
  PRINT("SEND_KEY:");
  OC_LOGbytes_OSCORE(ctx->sendkey, OSCORE_KEY_LEN);

  if (recipientid) {
    OC_DBG_OSCORE("### \t\tderiving Recipient key ###");
    if (oc_oscore_context_derive_param(
          ctx->recvid, ctx->recvid_len, ctx->idctx, ctx->idctx_len, "Key",
          (uint8_t *)mastersecret, strlen(mastersecret), NULL, 0, ctx->recvkey,
          OSCORE_KEY_LEN) < 0) {
      OC_ERR("*** error deriving Recipient key ###");
      goto add_oscore_context_error;
    }

    OC_DBG_OSCORE("### derived Recipient key ###");
  }
  PRINT("RCV_KEY:");
  OC_LOGbytes_OSCORE(ctx->recvkey, OSCORE_KEY_LEN);

  OC_DBG_OSCORE("### \t\tderiving Common IV ###");
  if (oc_oscore_context_derive_param(NULL, 0, ctx->idctx, ctx->idctx_len, "IV",
                                     (uint8_t *)mastersecret,
                                     strlen(mastersecret), NULL, 0,
                                     ctx->commoniv, OSCORE_COMMON_IV_LEN) < 0) {
    OC_ERR("*** error deriving Common IV ###");
    goto add_oscore_context_error;
  }
  PRINT("IV:");
  OC_LOGbytes_OSCORE(ctx->commoniv, OSCORE_COMMON_IV_LEN);
  OC_DBG_OSCORE("### derived Common IV ###");

  oc_list_add(contexts, ctx);

  return ctx;

add_oscore_context_error:
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
