/*
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

#include "dns-sd.h"
#include "ipadapter.h"

#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <string.h>
#include <Windows.h>
#include <inttypes.h>

intptr_t process_handle = 0;
char subtypes[64];
char prefixed_serial_no[64];
static char port_str[7];
static char sp_text_record[16] = "";

int
knx_publish_service(char *serial_no, uint64_t iid, uint32_t ia, bool pm)
{
  (void)serial_no;
  (void)iid;
  (void)ia;
  (void)pm;

#ifdef OC_DNS_SD
  if (process_handle != 0) {
    TerminateProcess((HANDLE)process_handle, 0);
  }

  uint16_t port = get_ip_context_for_device(0)->port;
  snprintf(port_str, sizeof(port_str), "%d", port);
  char *pm_subtype;
  if (pm)
    pm_subtype = ",_pm";
  else
    pm_subtype = "";

  snprintf(subtypes, 63, "_knx._udp,_ia%" PRIx64 "-%x%s,_%s", iid, ia,
           pm_subtype, serial_no);

  process_handle = _spawnlp(_P_NOWAIT, "dns-sd", "dns-sd", "-R", serial_no,
                            subtypes, "local", port_str, sp_text_record, NULL);
#endif /* OC_DNS_SD */

  return 0;
}

void
knx_service_sleep_period(uint32_t sp)
{
  if (sp)
    sprintf(sp_text_record, "SP=%d", sp);
  else
    memset(sp_text_record, 0, sizeof(sp_text_record));
}
