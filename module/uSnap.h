#include <stdlib.h>

#define USNAP_MODE_CHECKPOINT 	1
#define USNAP_MODE_RESTART		2

void usnap_init(pid_t id, int mode)
{
	syscall(332, id, mode);
}

int usnap_store()
{
	return syscall(333);
}

int usnap_restore()
{
	return syscall(334);
}
