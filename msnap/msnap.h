#include <stdlib.h>

void msnap_init(pid_t id)
{
	syscall(332, id);
}

int msnap_store()
{
	return syscall(333);
}

int msnap_restore()
{
	return syscall(334);
}
