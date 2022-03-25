#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <oc_log.h>
#include <errno.h>

static pid_t avahi_pid = 0;

int
knx_publish_service(char *serial_no)
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
    int error;
    error = execlp("avahi-publish-service", "avahi-publish-service",
                   //"--subtype=_ia._sub._knx._udp", // subtype present for
                   //uncomissioned devices???
                   serial_no,   // service name = serial number
                   "_knx._udp", // service type
                   "5683",      // port
                   (char *)NULL);

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