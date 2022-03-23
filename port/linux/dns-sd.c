#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <oc_log.h>
#include <errno.h>

static pid_t avahi_pid = 0;

int knx_publish_service()
{
	if (avahi_pid != 0)
	{
		// A previously published service advertisment is still running
		// Kill it so that you can start a new one
		kill(avahi_pid, SIGTERM);
	}

	// Start the service advertisment in a new process
	avahi_pid = fork();

	if (avahi_pid == 0)
	{
		// we are in the child thread - execute Avahi
		execlp("avahi-publish-service", "avahi-publish-service", (char *)NULL);
	}
	else if (avahi_pid > 0)
	{
		// we are in the parent thread - return successfully
		return 0;
	}
	else
	{
		// fork failed
		OC_ERR("Failed to fork Avahi advertisement process, error %d", errno);
		return -1;
	}
	return 0;
}