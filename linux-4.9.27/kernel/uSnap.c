#include <linux/uSnap.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <asm/syscall.h>


static pid_t uSnap_target = -1;
static struct task_struct* clone;

asmlinkage long sys_uSnap_init(pid_t target_sid)
{
	printk(KERN_INFO"uSnap] Initialized\n");
	uSnap_target = target_sid;
	return 0;
}

asmlinkage long sys_uSnap_store(void)
{
	struct task_struct *t;

	if(uSnap_target < 0)
		return -1;

	printk(KERN_INFO"uSnap] Send SIGSTOP to %d\n", uSnap_target);
	if (kill_pgrp(find_get_pid(uSnap_target), SIGSTOP, 1) != 0)
		goto err_out;

	for_each_process(t)
	{
		if(is_uSnap_target(t))
		{
			printk(KERN_INFO"uSnap] Snap %d %ld\n", t->pid, t->state);
			while(t->state == TASK_RUNNING)
			{
				//printk(KERN_INFO"Msnap] Target still running\n");
				schedule();
			}
			clone = uSnap_dup_task(t);

			if(!clone)
				goto err_out;
			break;
		}
	}


	printk(KERN_INFO"uSnap] Send SIGCONT to %d\n", uSnap_target);
	if (kill_pgrp(find_get_pid(uSnap_target), SIGCONT, 1) != 0)
		goto err_out;

	return 0;

err_out:
	printk(KERN_INFO"uSnap] No such task... %d\n", uSnap_target);
	uSnap_target = -1;
	return -1;
}

asmlinkage long sys_uSnap_exit(void)
{
	printk(KERN_INFO"uSnap] Bye bye...\n");
	uSnap_target = -1;
	return 0;
}

asmlinkage long sys_uSnap_restore(void)
{
	printk(KERN_INFO"uSnap] Wake up\n");
	wake_up_new_task(clone);
	return 0;
}
int is_uSnap_target(struct task_struct *t)
{
	struct pid* group;
	pid_t pgid;

	rcu_read_lock();
	group = task_pgrp(t);
	pgid = pid_vnr(group);
	rcu_read_unlock();

	return (uSnap_target == pgid);
}
