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

#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <string.h>
#include <Windows.h>

intptr_t process_handle = 0;
char subtypes[200];
static char port_str[7];

int
knx_publish_service(char *serial_no, uint32_t iid, uint32_t ia)
{
  (void)serial_no;
  (void)iid;
  (void)ia;

#ifdef OC_DNS_SD
  if (process_handle != 0) {
    TerminateProcess((HANDLE)process_handle, 0);
  }

  uint16_t port = get_ip_context_for_device(0)->port;
  snprintf(port_str, sizeof(port), "%d", port);

  if (iid == 0 || ia == 0) {
    sprintf(subtypes, "_knx._udp,_%s,_ia0", serial_no);
    process_handle = _spawnlp(_P_NOWAIT, "dns-sd", "dns-sd", "-R", serial_no,
                              subtypes, "local", port_str, NULL);
  } else {
    sprintf(subtypes, "_knx._udp,_%s,_ia%X-%X", serial_no, iid, ia);
    process_handle = _spawnlp(_P_NOWAIT, "dns-sd", "dns-sd", "-R", serial_no,
                              subtypes, "local", port_str, NULL);
  }
#endif /* OC_DNS_SD */

  return 0;
}