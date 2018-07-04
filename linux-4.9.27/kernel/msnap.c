#include <linux/msnap.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <asm/syscall.h>


static pid_t msnap_target = -1;
static struct task_struct* clone;

asmlinkage long sys_msnap_init(pid_t target_sid)
{
	printk(KERN_INFO"Msnap] Initialized\n");
	msnap_target = target_sid;
	return 0;
}

asmlinkage long sys_msnap_store(void)
{
	struct task_struct *t;

	if( msnap_target < 0 )
		return -1;

	printk(KERN_INFO"Msnap] Send SIGSTOP to %d\n", msnap_target);
	if ( kill_pgrp( find_get_pid(msnap_target), SIGSTOP, 1 ) != 0 )
		goto err_out;


	for_each_process(t)
	{
		if( is_msnap_target(t) )
		{
			printk(KERN_INFO"Msnap] Snap %d %d\n", t->pid, t->state);
			while( t->state == TASK_RUNNING )
			{
				//printk(KERN_INFO"Msnap] Target still running\n");
				schedule();
			}
			clone = msnap_dup_task(t);

			if( !clone )
				goto err_out;
			break;
		}
	}


	printk(KERN_INFO"Msnap] Send SIGCONT to %d\n", msnap_target);
	if ( kill_pgrp( find_get_pid(msnap_target), SIGCONT, 1 ) != 0 )
		goto err_out;

	return 0;

err_out:
	printk(KERN_INFO"Msnap] No such task... %d\n", msnap_target);
	msnap_target = -1;
	return -1;
}

asmlinkage long sys_msnap_exit(void)
{
	printk(KERN_INFO"Msnap] Bye bye...\n");
	msnap_target = -1;
	return 0;
}

asmlinkage long sys_msnap_restore(void)
{
	printk(KERN_INFO"Msnap] Wake up\n");
	wake_up_new_task(clone);
	return 0;
}
int is_msnap_target(struct task_struct *t)
{
	struct pid* group;
	pid_t pgid;

	rcu_read_lock();
	group = task_pgrp(t);
	pgid = pid_vnr(group);
	rcu_read_unlock();

	return (msnap_target == pgid);
}
