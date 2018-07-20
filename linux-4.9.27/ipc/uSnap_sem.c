#include <linux/uSnap.h>

int uSnap_copy_semundo(struct task_struct *tsk, struct task_struct *to_copy)
{

	tsk->sysvsem.undo_list = NULL;
	return 0;
}
