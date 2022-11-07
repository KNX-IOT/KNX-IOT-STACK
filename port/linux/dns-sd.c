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

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <oc_log.h>
#include <errno.h>
#include <stdint.h>

#include "dns-sd.h"
#include "ipadapter.h"

static pid_t avahi_pid = 0;
static char serial_no_subtype[200];
static char installation_subtype[200];
static char port_str[7];

int
knx_publish_service(char *serial_no, uint32_t iid, uint32_t ia, bool pm)
{
  (void)serial_no;
  (void)iid;
  (void)ia;
  (void)pm;

#ifdef OC_DNS_SD
  if (avahi_pid != 0) {
    // A previously published service advertisement is still running
    // Kill it so that you can start a new one
    kill(avahi_pid, SIGTERM);
  }

  // Start the service advertisement in a new process
  avahi_pid = fork();

  if (avahi_pid == 0) {
    // we are in the child thread - execute Avahi
    // Set up the subtype for the serial number
    // --subtype=_01CAFE1234._sub._knx._udp
    char *serial_format_string = "--subtype=_%s._sub._knx._udp";
    snprintf(serial_no_subtype, sizeof(serial_no_subtype), serial_format_string,
             serial_no);

    char *ia0_subtype = "--subtype=_ia0._sub._knx._udp";

    uint16_t port = get_ip_context_for_device(0)->port;
    snprintf(port_str, sizeof(port), "%d", port);

    int error;
    if (pm) {
      if (!iid || !ia) {
        // No Installation ID or Individual Address

        error =
          execlp("avahi-publish-service", "avahi-publish-service",
                serial_no_subtype,
                ia0_subtype, // programming mode is true - we publish the _ia0 subtype too
                serial_no,   // service name = serial number
                "_knx._udp", // service type
                port_str,    // port
                (char *)NULL);
      } else {
        // --subtype=_ia3333-CA._sub._knx._udp
        char *format_string = "--subtype=_ia%X-%X._sub._knx._udp";
        snprintf(installation_subtype, sizeof(serial_no_subtype), format_string,
                iid, ia);

        error = execlp("avahi-publish-service", "avahi-publish-service",
                      serial_no_subtype,
                      installation_subtype,
                      ia0_subtype, // programming mode is true - publish _ia0 even though we have an installation already
                      serial_no,   // service name = serial number
                      "_knx._udp", // service type
                      port_str,    // port
                      (char *)NULL);
      }
    }
    else
    {
      if (!iid || !ia) {
        error =
          execlp("avahi-publish-service", "avahi-publish-service",
                serial_no_subtype,
                serial_no,
                "_knx._udp",
                port_str,
                (char *)NULL);
      } else {
        // --subtype=_ia3333-CA._sub._knx._udp
        char *format_string = "--subtype=_ia%X-%X._sub._knx._udp";
        snprintf(installation_subtype, sizeof(serial_no_subtype), format_string,
                iid, ia);

        error = execlp("avahi-publish-service", "avahi-publish-service",
                      serial_no_subtype,
                      installation_subtype,
                      serial_no,
                      "_knx._udp",
                      port_str,
                      (char *)NULL);
      }
    }
    if (error == -1) {
      OC_ERR("Failed to execute avahi-publish-service: %s", strerror(errno));
      return -1;
    }
  } else if (avahi_pid > 0) {
    // we are in the parent thread - return successfully
    return 0;
  } else {
    // fork failed
    OC_ERR("Failed to fork Avahi advertisement process, error %s",
           strerror(errno));
    return -1;
  }
#endif /* OC_DNS_SD */

  return 0;
}