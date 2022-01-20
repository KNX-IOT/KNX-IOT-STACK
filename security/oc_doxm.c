/*
// Copyright (c) 2016-2019 Intel Corporation
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

#ifdef OC_SECURITY

#include "oc_doxm.h"
#include "oc_acl_internal.h"
#include "oc_api.h"
#include "oc_core_res.h"
#include "oc_pstat.h"
#include "oc_store.h"
#include "oc_tls.h"
#include <stddef.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#endif

#ifdef OC_DYNAMIC_ALLOCATION
#include "port/oc_assert.h"
#include <stdlib.h>
static oc_sec_doxm_t *doxm;
#else  /* OC_DYNAMIC_ALLOCATION */
static oc_sec_doxm_t doxm[OC_MAX_NUM_DEVICES];
#endif /* !OC_DYNAMIC_ALLOCATION */

typedef struct oc_doxm_owned_cb_s
{
  struct oc_doxm_owned_cb_s *next;
  oc_ownership_status_cb_t cb;
  void *user_data;
} oc_doxm_owned_cb_t;

OC_LIST(oc_doxm_owned_cb_list_t);
OC_MEMB(oc_doxm_owned_cb_s, oc_doxm_owned_cb_t, OC_MAX_DOXM_OWNED_CBS);

void
oc_sec_doxm_free(void)
{
#ifdef OC_DYNAMIC_ALLOCATION
  oc_doxm_owned_cb_t *doxm_cb_item =
    (oc_doxm_owned_cb_t *)oc_list_pop(oc_doxm_owned_cb_list_t);
  while (doxm_cb_item) {
    free(doxm_cb_item);
    doxm_cb_item = (oc_doxm_owned_cb_t *)oc_list_pop(oc_doxm_owned_cb_list_t);
  }
  if (doxm) {
    free(doxm);
  }
#endif /* OC_DYNAMIC_ALLOCATION */
}

void
oc_sec_doxm_init(void)
{
#ifdef OC_DYNAMIC_ALLOCATION
  doxm =
    (oc_sec_doxm_t *)calloc(oc_core_get_num_devices(), sizeof(oc_sec_doxm_t));
  if (!doxm) {
    oc_abort("Insufficient memory");
  }
#endif /* OC_DYNAMIC_ALLOCATION */
}

static void
evaluate_supported_oxms(size_t device)
{
  doxm[device].oxms[0] = OC_OXMTYPE_JW;
  doxm[device].oxms[1] = -1;
  doxm[device].oxms[2] = -1;
  doxm[device].num_oxms = 1;
  if (oc_tls_is_pin_otm_supported(device)) {
    doxm[device].oxms[doxm[device].num_oxms++] = OC_OXMTYPE_RDP;
  }
#ifdef OC_PKI
  if (oc_tls_is_cert_otm_supported(device)) {
    doxm[device].oxms[doxm[device].num_oxms++] = OC_OXMTYPE_MFG_CERT;
  }
#endif /* OC_PKI */
}

void
oc_sec_doxm_default(size_t device)
{
  // invoke the device owned changed cb before the deviceuuid is reset
  if (doxm[device].owned) {
    oc_doxm_owned_cb_t *doxm_cb_item =
      (oc_doxm_owned_cb_t *)oc_list_head(oc_doxm_owned_cb_list_t);
    while (doxm_cb_item) {
      (doxm_cb_item->cb)(&doxm[device].deviceuuid, device, false,
                         doxm_cb_item->user_data);
      doxm_cb_item = doxm_cb_item->next;
    }
  }

  /* In RESET, oxmsel shall be set to (4) "oic.sec.oxm.self" */
  doxm[device].oxmsel = 4;
#ifdef OC_PKI
  doxm[device].sct = 9;
#else  /* OC_PKI */
  doxm[device].sct = 1;
#endif /* !OC_PKI */
#ifdef OC_OSCORE
  doxm[device].sct |= OC_CREDTYPE_OSCORE;
#ifdef OC_CLIENT
  doxm[device].sct |= OC_CREDTYPE_OSCORE_MCAST_CLIENT;
#endif /* OC_CLIENT */
#ifdef OC_SERVER
  doxm[device].sct |= OC_CREDTYPE_OSCORE_MCAST_SERVER;
#endif /* OC_SERVER */
#endif /* OC_OSCORE */
  doxm[device].owned = false;
  memset(doxm[device].devowneruuid.id, 0, 16);
  memset(doxm[device].rowneruuid.id, 0, 16);
  /* Generate a new temporary device UUID */
  oc_device_info_t *d = oc_core_get_device_info(device);
  oc_gen_uuid(&doxm[device].deviceuuid);
  memcpy(d->di.id, doxm[device].deviceuuid.id, 16);
#ifdef OC_SPAKE2PLUS
  oc_free_string(&doxm[device].obtshare);
  oc_new_string(&doxm[device].obtshare, "", 0);

  oc_new_string(&doxm[device].deviceshare, "", 0);
  oc_free_string(&doxm[device].deviceshare);

  oc_new_string(&doxm[device].obtcheck, "", 0);
  oc_free_string(&doxm[device].obtcheck);

  oc_new_string(&doxm[device].devicecheck, "", 0);
  oc_free_string(&doxm[device].devicecheck);

  // oc_string_t spakecontext;
  char *spctx = "OCF Spake2plus,Version:1.0";
  oc_free_string(&doxm[device].spakecontext);
  oc_new_string(&doxm[device].spakecontext, spctx, strlen(spctx));

  // oc_string_t spakesalt;
  oc_free_string(&doxm[device].obtshare);
  oc_new_string(&doxm[device].obtshare, "", 0);

  // int spakeiterations = 0;
#endif
  oc_sec_dump_doxm(device);
}

void
oc_sec_encode_doxm(size_t device, oc_interface_mask_t iface_mask,
                   bool to_storage)
{
  char uuid[37];
  oc_rep_start_root_object();
  if (to_storage || iface_mask & OC_IF_BASELINE) {
    oc_process_baseline_interface(
      oc_core_get_resource_by_index(OCF_SEC_DOXM, device));
  }
  /* oxms */
  if (!to_storage) {
    evaluate_supported_oxms(device);
    oc_rep_set_int_array(root, oxms, doxm[device].oxms, doxm[device].num_oxms);
  }
  /* oxmsel */
  oc_rep_set_int(root, oxmsel, doxm[device].oxmsel);
  /* sct */
  oc_rep_set_int(root, sct, doxm[device].sct);
  /* owned */
  oc_rep_set_boolean(root, owned, doxm[device].owned);
  /* devowneruuid */
  oc_uuid_to_str(&doxm[device].devowneruuid, uuid, OC_UUID_LEN);
  oc_rep_set_text_string(root, devowneruuid, uuid);
  /* deviceuuid */
  oc_uuid_to_str(&doxm[device].deviceuuid, uuid, OC_UUID_LEN);
  oc_rep_set_text_string(root, deviceuuid, uuid);
  /* rowneruuid */
  oc_uuid_to_str(&doxm[device].rowneruuid, uuid, OC_UUID_LEN);
  oc_rep_set_text_string(root, rowneruuid, uuid);
  /* obtshare */
  oc_rep_set_text_string(root, obtshare, oc_string(doxm[device].obtshare));
  /* deviceshare */
  oc_rep_set_text_string(root, deviceshare,
                         oc_string(doxm[device].deviceshare));
  /* obtcheck */
  oc_rep_set_text_string(root, obtcheck, oc_string(doxm[device].obtcheck));
  /* devicecheck */
  oc_rep_set_text_string(root, devicecheck,
                         oc_string(doxm[device].devicecheck));
  /* spakecontext */
  oc_rep_set_text_string(root, spakecontext,
                         oc_string(doxm[device].spakecontext));
  /* spakeiterations */
  oc_rep_set_int(root, spakeiterations, doxm[device].spakeiterations);
  oc_rep_end_root_object();
}

oc_sec_doxm_t *
oc_sec_get_doxm(size_t device)
{
  return &doxm[device];
}

void
get_doxm(oc_request_t *request, oc_interface_mask_t iface_mask, void *data)
{
  (void)data;
  switch (iface_mask) {
  case OC_IF_RW:
  case OC_IF_BASELINE: {
    char *q;
    int ql = oc_get_query_value(request, "owned", &q);
    size_t device = request->resource->device;
    if (ql > 0 &&
        ((doxm[device].owned == 1 && strncasecmp(q, "false", 5) == 0) ||
         (doxm[device].owned == 0 && strncasecmp(q, "true", 4) == 0))) {
      if (request->origin && (request->origin->flags & MULTICAST) == 0) {
        request->response->response_buffer->code =
          oc_status_code(OC_STATUS_BAD_REQUEST);
      } else {
        oc_ignore_request(request);
      }
    } else {
      oc_sec_encode_doxm(device, iface_mask, false);
      oc_send_response(request, OC_STATUS_OK);
    }
  } break;
  default:
    break;
  }
}

bool
oc_sec_decode_doxm(oc_rep_t *rep, bool from_storage, bool doc, size_t device)
{
  oc_sec_pstat_t *ps = oc_sec_get_pstat(device);
  oc_rep_t *t = rep;
  size_t len = 0;
  bool owned_changed = false;

  while (t != NULL) {
    len = oc_string_len(t->name);
    switch (t->type) {
    /* owned */
    case OC_REP_BOOL:
      if (len == 5 && memcmp(oc_string(t->name), "owned", 5) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set owned property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set owned property outside DOC");
            return false;
          }
        }
      } else {
        OC_ERR("oc_doxm: Unknown property %s", oc_string(t->name));
        return false;
      }
      break;
    /* oxmsel and sct */
    /* spakeiterations */
    case OC_REP_INT:
      if (len == 6 && memcmp(oc_string(t->name), "oxmsel", 6) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: Can set oxmsel property only in RFOTM");
            return false;
          } else {
            evaluate_supported_oxms(device);
            int oxm = 0;
            while (oxm < doxm[device].num_oxms) {
              if (doxm[device].oxms[oxm] == (int)t->value.integer) {
                break;
              }
              oxm++;
            }
            if (oxm == doxm[device].num_oxms) {
              OC_ERR("oc_doxm: Attempting to select an unsupported OXM");
              return false;
            }
            if (doc) {
              OC_ERR("oc_doxm: cannot set oxmsel inside DOC");
              return false;
            }
          }
        }
      } else if (from_storage && len == 3 &&
                 memcmp(oc_string(t->name), "sct", 3) == 0) {
      } else if (len == 15 &&
                 memcmp(oc_string(t->name), "spakeiterations", 15) == 0) {
        if (ps->s != OC_DOS_RFOTM) {
          OC_ERR("oc_doxm: Can set spakeiterations property only in RFOTM");
          return false;
        }
        if (!doc) {
          OC_ERR("oc_doxm: cannot set sct property outside DOC");
          return false;
        }
      } else {
        OC_ERR("oc_doxm: Unknown property %s", oc_string(t->name));
        return false;
      }
      break;
    /* deviceuuid, devowneruuid and rowneruuid */
    /* obtshare, deviceshare, obtcheck, devicecheck,
     * spakesalt, spakecontext */
    case OC_REP_STRING:
      if (len == 10 && memcmp(oc_string(t->name), "deviceuuid", 10) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set deviceuuid property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set deviceuuid outside DOC");
            return false;
          }
        }
      } else if (len == 12 &&
                 memcmp(oc_string(t->name), "devowneruuid", 12) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set devowneruuid property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set devowneruuid outside DOC");
            return false;
          }
        }
      } else if (len == 10 &&
                 memcmp(oc_string(t->name), "rowneruuid", 10) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM && ps->s != OC_DOS_SRESET) {
            OC_ERR("oc_doxm: can set rowneruuid property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set rowneruuid outside DOC");
            return false;
          }
        }
      } else if (len == 8 && memcmp(oc_string(t->name), "obtshare", 8) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set obtshare property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set obtshare outside DOC");
            return false;
          }
        }
      } else if (len == 11 &&
                 memcmp(oc_string(t->name), "deviceshare", 11) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set deviceshare property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set deviceshare outside DOC");
            return false;
          }
        }
      } else if (len == 8 && memcmp(oc_string(t->name), "obtcheck", 8) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set obtcheck property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set obtcheck outside DOC");
            return false;
          }
        }
      } else if (len == 11 &&
                 memcmp(oc_string(t->name), "devicecheck", 11) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set devicecheck property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set devicecheck outside DOC");
            return false;
          }
        }
      } else if (len == 8 && memcmp(oc_string(t->name), "obtshare", 8) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set obtshare property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set obtshare outside DOC");
            return false;
          }
        }
      } else if (len == 9 && memcmp(oc_string(t->name), "spakesalt", 9) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set spakesalt property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set spakesalt outside DOC");
            return false;
          }
        }
      } else if (len == 12 &&
                 memcmp(oc_string(t->name), "spakecontext", 12) == 0) {
        if (!from_storage) {
          if (ps->s != OC_DOS_RFOTM) {
            OC_ERR("oc_doxm: can set spakecontext property only in RFOTM");
            return false;
          }
          if (!doc) {
            OC_ERR("oc_doxm: cannot set spakecontext outside DOC");
            return false;
          }
        }
      } else {
        OC_ERR("oc_doxm: unknown property %s", oc_string(t->name));
        return false;
      }
      break;
    /* oxms */
    case OC_REP_INT_ARRAY:
      if (!from_storage && len == 4 &&
          memcmp(oc_string(t->name), "oxms", 4) == 0) {
        OC_ERR("oc_doxm: cannot set oxms property");
        return false;
      }
      break;
    default: {
      if (!((len == 2 && (memcmp(oc_string(t->name), "rt", 2) == 0 ||
                          memcmp(oc_string(t->name), "if", 2) == 0))) &&
          !(len == 4 && memcmp(oc_string(t->name), "oxms", 4) == 0)) {
        OC_ERR("oc_doxm: unknown property %s", oc_string(t->name));
        return false;
      }
    } break;
    }
    t = t->next;
  }

  while (rep != NULL) {
    len = oc_string_len(rep->name);
    switch (rep->type) {
    /* owned */
    case OC_REP_BOOL:
      if (len == 5 && memcmp(oc_string(rep->name), "owned", 5) == 0) {
        doxm[device].owned = rep->value.boolean;
        owned_changed = true;
      }
      break;
    /* oxmsel and sct */
    case OC_REP_INT:
      if (len == 6 && memcmp(oc_string(rep->name), "oxmsel", 6) == 0) {
        doxm[device].oxmsel = (int)rep->value.integer;
        if (!from_storage && doxm[device].oxmsel == OC_OXMTYPE_RDP) {
          oc_tls_generate_random_pin();
        }
      } else if (from_storage && len == 3 &&
                 memcmp(oc_string(rep->name), "sct", 3) == 0) {
        doxm[device].sct = (int)rep->value.integer;
      }
      break;
    /* deviceuuid, devowneruuid and rowneruuid */
    /* obtshare, deviceshare, obtcheck, devicecheck,
     * spakesalt, spakecontext */
    case OC_REP_STRING:
      if (len == 10 && memcmp(oc_string(rep->name), "deviceuuid", 10) == 0) {
        oc_str_to_uuid(oc_string(rep->value.string), &doxm[device].deviceuuid);
        oc_uuid_t *deviceuuid = oc_core_get_device_id(device);
        memcpy(deviceuuid->id, doxm[device].deviceuuid.id, 16);
      } else if (len == 12 &&
                 memcmp(oc_string(rep->name), "devowneruuid", 12) == 0) {
        oc_str_to_uuid(oc_string(rep->value.string),
                       &doxm[device].devowneruuid);
      } else if (len == 10 &&
                 memcmp(oc_string(rep->name), "rowneruuid", 10) == 0) {
        oc_str_to_uuid(oc_string(rep->value.string), &doxm[device].rowneruuid);
      } else if (len == 8 && memcmp(oc_string(rep->name), "obtshare", 8) == 0) {

        oc_string_t *s = &doxm[device].obtshare;
        oc_free_string(s);
        oc_new_string(s, oc_string(rep->value.string),
                      strlen(oc_string(rep->value.string)));
      } else if (len == 12 &&
                 memcmp(oc_string(rep->name), "deviceshare", 12) == 0) {

        oc_string_t *s = &doxm[device].deviceshare;
        oc_free_string(s);
        oc_new_string(s, oc_string(rep->value.string),
                      strlen(oc_string(rep->value.string)));
      } else if (len == 8 && memcmp(oc_string(rep->name), "obtcheck", 8) == 0) {

        oc_string_t *s = &doxm[device].obtcheck;
        oc_free_string(s);
        oc_new_string(s, oc_string(rep->value.string),
                      strlen(oc_string(rep->value.string)));
      } else if (len == 11 &&
                 memcmp(oc_string(rep->name), "devicecheck", 11) == 0) {

        oc_string_t *s = &doxm[device].devicecheck;
        oc_free_string(s);
        oc_new_string(s, oc_string(rep->value.string),
                      strlen(oc_string(rep->value.string)));
      } else if (len == 9 &&
                 memcmp(oc_string(rep->name), "spakesalt", 9) == 0) {

        oc_string_t *s = &doxm[device].spakesalt;
        oc_free_string(s);
        oc_new_string(s, oc_string(rep->value.string),
                      strlen(oc_string(rep->value.string)));
      } else if (len == 12 &&
                 memcmp(oc_string(rep->name), "spakecontext", 12) == 0) {

        oc_string_t *s = &doxm[device].spakecontext;
        oc_free_string(s);
        oc_new_string(s, oc_string(rep->value.string),
                      strlen(oc_string(rep->value.string)));
      }
      break;
    default:
      break;
    }
    rep = rep->next;
  }

  if (owned_changed == true) {
    oc_doxm_owned_cb_t *doxm_cb_item =
      (oc_doxm_owned_cb_t *)oc_list_head(oc_doxm_owned_cb_list_t);
    while (doxm_cb_item) {
      oc_doxm_owned_cb_t *invokee = doxm_cb_item;
      doxm_cb_item = doxm_cb_item->next;
      (invokee->cb)(&doxm[device].deviceuuid, device, doxm[device].owned,
                    invokee->user_data);
    }
  }
  return true;
}

void
post_doxm(oc_request_t *request, oc_interface_mask_t iface_mask, void *data)
{
  (void)iface_mask;
  (void)data;
  oc_tls_peer_t *p = oc_tls_get_peer(request->origin);
  if (oc_sec_decode_doxm(request->request_payload, false, (p) ? p->doc : false,
                         request->resource->device)) {
    oc_send_response(request, OC_STATUS_CHANGED);
    oc_sec_dump_doxm(request->resource->device);
  } else {
    oc_send_response(request, OC_STATUS_BAD_REQUEST);
  }
}

void
oc_add_ownership_status_cb(oc_ownership_status_cb_t cb, void *user_data)
{
  oc_doxm_owned_cb_t *new_doxm_cb = oc_memb_alloc(&oc_doxm_owned_cb_s);
  if (!new_doxm_cb) {
    oc_abort("Insufficient memory");
  }
  new_doxm_cb->cb = cb;
  new_doxm_cb->user_data = user_data;
  oc_list_add(oc_doxm_owned_cb_list_t, new_doxm_cb);
}

void
oc_remove_ownership_status_cb(oc_ownership_status_cb_t cb, void *user_data)
{
  oc_doxm_owned_cb_t *doxm_cb_item =
    (oc_doxm_owned_cb_t *)oc_list_head(oc_doxm_owned_cb_list_t);
  while (doxm_cb_item) {
    if (cb == doxm_cb_item->cb && user_data == doxm_cb_item->user_data) {
      oc_list_remove(oc_doxm_owned_cb_list_t, doxm_cb_item);
      free(doxm_cb_item);
      break;
    }
    doxm_cb_item = doxm_cb_item->next;
  }
}

bool
oc_is_owned_device(size_t device_index)
{
  if (doxm) {
    return doxm[device_index].owned;
  }
  return false;
}
#endif /* OC_SECURITY */
