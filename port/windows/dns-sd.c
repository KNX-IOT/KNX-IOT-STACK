#include "dns-sd.h"

#define WIN32_LEAN_AND_MEAN
#include <process.h>

intptr_t process_handle;

int
knx_publish_service(char *serial_no, uint32_t iid, uint32_t ia)
{
  (void)serial_no;
  (void)iid;
  (void)ia;

  process_handle = _spawnlp(_P_NOWAIT, "dns-sd",
    "dns-sd", "-R", "1234", "_knx._udp,_12345,_ia0", "local", "5447", NULL
  );
  return 0;
}