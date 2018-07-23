#include <linux/uSnap.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>

static int uSnap_restore_pid(struct task_struct *dst, int pid)
{
	struct pid *new_pid;
	int ret;

	task_active_pid_ns(dst)->last_pid = pid - 1;

	new_pid = alloc_pid(task_active_pid_ns(dst));

	if(!new_pid)
		return -1;

	change_pid(dst, PIDTYPE_PID, new_pid);
	return 0;	
}

int uSnap_restore_task(void)
{
	struct task_struct *dst = current;
	struct task_struct *src = &uSnap_kern->task_struct;
	int ret;

	ret = uSnap_restore_pid(dst, uSnap_kern->pid);

	if(ret < 0)
		goto res_pid_fail;
	
	return 0;

res_pid_fail:
	printk(KERN_ALERT"uSnap] Failed restoring pid\n");
	return -1;
}
