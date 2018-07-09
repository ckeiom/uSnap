#ifndef __USNAP_H__
#define __USNAP_H__

#include <linux/kernel.h>
#include <linux/sched.h>

int is_uSnap_target(struct task_struct *t);
extern struct task_struct* uSnap_dup_task(struct task_struct *t);
#endif
