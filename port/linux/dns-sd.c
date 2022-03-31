#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <oc_log.h>
#include <errno.h>

static pid_t avahi_pid = 0;
static char serial_no_subtype[200];
static char installation_subtype[200];

int
knx_publish_service(char *serial_no, char *iid, char *ia)
{
  if (avahi_pid != 0) {
    // A previously published service advertisment is still running
    // Kill it so that you can start a new one
    kill(avahi_pid, SIGTERM);
  }

  // Start the service advertisment in a new process
  avahi_pid = fork();

  if (avahi_pid == 0) {
    // we are in the child thread - execute Avahi
    // Set up the subtype for the serial number
    // --subtype=_01CAFE1234._sub._knx._udp
    char *serial_format_string = "--subtype=_%s._sub._knx._udp";
    snprintf(serial_no_subtype, sizeof(serial_no_subtype), serial_format_string,
              serial_no);

    int error;
    if (!iid || !ia) {
      // No Installation ID or Individual Address - we are in Programming mode

      error =
        execlp("avahi-publish-service", "avahi-publish-service",
               "--subtype=_ia0._sub._knx._udp", // for uncommissioned devices
               serial_no_subtype,
               serial_no,   // service name = serial number
               "_knx._udp", // service type
               "5683",      // port
               (char *)NULL);
    } else {
      // We have already been commissioned

      // Set up the subtype for IID and IA
      // --subtype=_ia3333-CA._sub._knx._udp

      char *format_string = "--subtype=_ia%s-%s._sub._knx._udp";
      snprintf(installation_subtype, sizeof(serial_no_subtype), format_string,
               iid, ia);

      error = execlp("avahi-publish-service", "avahi-publish-service",
                     serial_no_subtype,
                     serial_no,   // service name = serial number
                     "_knx._udp", // service type
                     "5683",      // port
                     (char *)NULL);
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
  return 0;
}