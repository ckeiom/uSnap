#include <linux/uSnap.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <asm/syscall.h>


int uSnap_mode;
static pid_t uSnap_target = -1;
struct uSnap_kern* uSnap_kern;
void* uSnap_nv_pool;

asmlinkage long sys_uSnap_init(pid_t target_sid, int mode)
{
	printk(KERN_INFO"uSnap] Initialized\n");
	uSnap_target = target_sid;
	uSnap_mode = mode;

	if(!uSnap_nv_pool)
	{
		uSnap_nv_pool = vmalloc(USNAP_NV_POOL_SIZE);
		if(!uSnap_nv_pool)
			printk(KERN_ALERT"uSnap] uSnap NV pool is not initialized\n");
		uSnap_kern = (struct uSnap_kern*)uSnap_nv_pool;	
		return -1;
	}
	return 0;
}

asmlinkage long sys_uSnap_store(void)
{
	struct task_struct *t;
	int ret;

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
			ret = uSnap_store_task(t);
			if(ret < 0)
				goto err_out;
			break;
		}
	}


	printk(KERN_ALERT"uSnap] Send SIGCONT to %d\n", uSnap_target);
	if (kill_pgrp(find_get_pid(uSnap_target), SIGCONT, 1) != 0)
		goto err_out;

	printk(KERN_ALERT"uSnap] DONE\n");
	return 0;

err_out:
	printk(KERN_ALERT"uSnap] No such task... %d\n", uSnap_target);
	uSnap_target = -1;
	return -1;
}

asmlinkage long sys_uSnap_exit(void)
{
	printk(KERN_INFO"uSnap] Bye bye...\n");
	uSnap_target = -1;
	vfree(uSnap_nv_pool);
	return 0;
}

asmlinkage long sys_uSnap_restore(void)
{
	printk(KERN_INFO"uSnap] Wake up\n");
	//wake_up_new_task(clone);
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
