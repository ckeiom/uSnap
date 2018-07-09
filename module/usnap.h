#include <stdlib.h>

void usnap_init(pid_t id)
{
	syscall(332, id);
}

int usnap_store()
{
	return syscall(333);
}

int usnap_restore()
{
	return syscall(334);
}
