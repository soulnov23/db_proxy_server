#include "net_server.h"
#include "im_log.h"
#include <signal.h>

//kill -USR1 PID
void signal_func(int signum)
{
	if (signum == SIGUSR1)
	{
		net_server::get_instance()->stop();
	}
}

int main(int _Argc, char **_Argv)
{
	signal(SIGUSR1, signal_func);
	net_server::get_instance()->start();
	return 0;
}


