#ifndef __MSNAP_H__
#define __MSNAP_H__

#include <linux/kernel.h>
#include <linux/sched.h>

int is_msnap_target(struct task_struct *t);
extern struct task_struct* msnap_dup_task(struct task_struct *t);
#endif
