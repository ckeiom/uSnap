#include <stdio.h>
#include <stdlib.h>

int main()
{
	int i;
	printf("[LOOP] Start\n");

	for(i=0; i<10; i++)
	{
		printf("[LOOP] %d pid: %d\n", i, getpid());
		sleep(1);
	}
	return 0;
}
