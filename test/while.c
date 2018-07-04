#include <stdio.h>
#include <signal.h>

/* OBJECTIVE: to show Merong [PID] message by sending SIGUSR1 to clone */

void merong(int sig)
{
	printf("[WHILE] Merong %d\n",getpid());
}

int main()
{
	signal(SIGUSR1, merong);

	printf("[WHILE] Start\n");
	while(1);
	return 0;
}
