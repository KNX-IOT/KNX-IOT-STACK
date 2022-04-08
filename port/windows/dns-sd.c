#include "dns-sd.h"

#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <string.h>
#include <Windows.h>

intptr_t process_handle = 0;
char subtypes[200];

int
knx_publish_service(char *serial_no, uint32_t iid, uint32_t ia)
{
  (void)serial_no;
  (void)iid;
  (void)ia;

  if (process_handle != 0) {
    TerminateProcess((HANDLE)process_handle, 0);
  }

  if (iid == 0 || ia == 0) {
    sprintf(subtypes, "_knx._udp,_%s,_ia0", serial_no);
    process_handle = _spawnlp(_P_NOWAIT, "dns-sd", "dns-sd", "-R", serial_no,
                              subtypes, "local", "5683", NULL);
  } else {
    sprintf(subtypes, "_knx._udp,_%s,_ia%X-%X", serial_no, iid, ia);
    process_handle = _spawnlp(_P_NOWAIT, "dns-sd", "dns-sd", "-R", serial_no,
                              subtypes, "local", "5683", NULL);
  }
  return 0;
}