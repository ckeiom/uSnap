#ifndef __USNAP_H__
#define __USNAP_H__

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/mempolicy.h>
#include <linux/nsproxy.h>
#include <linux/mm.h>
#include <linux/rmap.h>
#include <linux/fs_struct.h>
#include <linux/iocontext.h>
#include <linux/mm_types.h>

#define USNAP_NV_POOL_SIZE	20 * 1024 * 1024
#define USNAP_MAX_ANON_VMA	20
struct uSnap_kern
{
	struct cred cred;
	struct nsproxy nsproxy;
	struct mempolicy mempolicy;
	struct mm_struct mm;
	struct vm_area_struct vm_area_struct;
	struct anon_vma anon_vma;
	struct anon_vma_chain anon_vma_chain[USNAP_MAX_ANON_VMA];
	struct anon_vma_chain anon_vma_chain_own;
	struct fs_struct fs_struct;
	struct io_context io_context;
	struct sighand_struct sighand_struct;
	struct signal_struct signal_struct;
	struct task_struct task_struct;
	char stack[THREAD_SIZE];

};

extern struct uSnap_kern* uSnap_kern;
extern void* uSnap_nv_pool;

int is_uSnap_target(struct task_struct *t);
extern struct task_struct* uSnap_dup_task(struct task_struct *t);

int uSnap_copy_creds(struct task_struct *p, struct task_struct *to_copy);
int uSnap_copy_namespaces(struct task_struct *p, struct task_struct *to_copy);
int uSnap_copy_semundo(struct task_struct *p, struct task_struct *to_copy);
struct mempolicy* uSnap_mpol_dup(struct mempolicy* pol, struct task_struct *to_copy);

int uSnap_copy_thread_tls(unsigned long sp, unsigned long arg,
						  struct task_struct *p, unsigned long tls,
						  struct task_struct *to_copy);

int uSnap_anon_vma_fork(struct vm_area_struct *vma, struct vm_area_struct *pvma);

struct io_context* uSnap_get_task_io_context(struct task_struct* task, gfp_t gfp_flags, int node);


/* from kernel/fork.c */

#endif
