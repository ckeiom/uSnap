#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "usnap.h"

int main(int argc, char **argv)
{
	pid_t new_sid;

	if (fork() == 0)
	{
		/* Designated program as a child */
		new_sid = setsid (); // Nominate designated process as group leader
		usnap_init (new_sid); // To inform kernel pgid of designated program
		execvp (argv[1], &argv[1]);
		printf("Exec failed %s\n", argv[1]);
	}
	else
	{
		/* Monitor program */
		sleep (2); // Give target program enough time to exec()
		usnap_store ();
		while(1);
	}
	return 0;
}


