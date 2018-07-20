/*
 * Functions related to io context handling
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/uSnap.h>
#include "blk.h"

static void uSnap_icq_free_icq_rcu(struct rcu_head *head)
{
	//struct io_cq *icq = container_of(head, struct io_cq, __rcu_head);

	//kmem_cache_free(icq->__rcu_icq_cache, icq);
}

static void uSnap_ioc_exit_icq(struct io_cq *icq)
{
	struct elevator_type *et = icq->q->elevator->type;

	if (icq->flags & ICQ_EXITED)
		return;

	if (et->ops.elevator_exit_icq_fn)
		et->ops.elevator_exit_icq_fn(icq);

	icq->flags |= ICQ_EXITED;
}

static void uSnap_ioc_destroy_icq(struct io_cq *icq)
{
	struct io_context *ioc = icq->ioc;
	struct request_queue *q = icq->q;
	struct elevator_type *et = q->elevator->type;

	lockdep_assert_held(&ioc->lock);
	lockdep_assert_held(q->queue_lock);

	radix_tree_delete(&ioc->icq_tree, icq->q->id);
	hlist_del_init(&icq->ioc_node);
	list_del_init(&icq->q_node);

	/*
	 * Both setting lookup hint to and clearing it from @icq are done
	 * under queue_lock.  If it's not pointing to @icq now, it never
	 * will.  Hint assignment itself can race safely.
	 */
	if (rcu_access_pointer(ioc->icq_hint) == icq)
		rcu_assign_pointer(ioc->icq_hint, NULL);

	uSnap_ioc_exit_icq(icq);

	/*
	 * @icq->q might have gone away by the time RCU callback runs
	 * making it impossible to determine icq_cache.  Record it in @icq.
	 */
	icq->__rcu_icq_cache = et->icq_cache;
	call_rcu(&icq->__rcu_head, uSnap_icq_free_icq_rcu);
}

static void uSnap_ioc_release_fn(struct work_struct *work)
{
	struct io_context *ioc = container_of(work, struct io_context,
					      release_work);
	unsigned long flags;

	/*
	 * Exiting icq may call into put_io_context() through elevator
	 * which will trigger lockdep warning.  The ioc's are guaranteed to
	 * be different, use a different locking subclass here.  Use
	 * irqsave variant as there's no spin_lock_irq_nested().
	 */
	spin_lock_irqsave_nested(&ioc->lock, flags, 1);

	while (!hlist_empty(&ioc->icq_list)) {
		struct io_cq *icq = hlist_entry(ioc->icq_list.first,
						struct io_cq, ioc_node);
		struct request_queue *q = icq->q;

		if (spin_trylock(q->queue_lock)) {
			uSnap_ioc_destroy_icq(icq);
			spin_unlock(q->queue_lock);
		} else {
			spin_unlock_irqrestore(&ioc->lock, flags);
			cpu_relax();
			spin_lock_irqsave_nested(&ioc->lock, flags, 1);
		}
	}

	spin_unlock_irqrestore(&ioc->lock, flags);
}


static int uSnap_create_task_io_context(struct task_struct *task, gfp_t gfp_flags, int node)
{
	struct io_context *ioc;
	int ret;

	ioc = &uSnap_kern->io_context;
	/* initialize */
	atomic_long_set(&ioc->refcount, 1);
	atomic_set(&ioc->nr_tasks, 1);
	atomic_set(&ioc->active_ref, 1);
	spin_lock_init(&ioc->lock);
	INIT_RADIX_TREE(&ioc->icq_tree, GFP_ATOMIC | __GFP_HIGH);
	INIT_HLIST_HEAD(&ioc->icq_list);
	INIT_WORK(&ioc->release_work, uSnap_ioc_release_fn);

	/*
	 * Try to install.  ioc shouldn't be installed if someone else
	 * already did or @task, which isn't %current, is exiting.  Note
	 * that we need to allow ioc creation on exiting %current as exit
	 * path may issue IOs from e.g. exit_files().  The exit path is
	 * responsible for not issuing IO after exit_io_context().
	 */
	task_lock(task);
	if (!task->io_context &&
	    (task == current || !(task->flags & PF_EXITING)))
		task->io_context = ioc;

	ret = task->io_context ? 0 : -EBUSY;

	task_unlock(task);

	return ret;
}

/**
 * get_task_io_context - get io_context of a task
 * @task: task of interest
 * @gfp_flags: allocation flags, used if allocation is necessary
 * @node: allocation node, used if allocation is necessary
 *
 * Return io_context of @task.  If it doesn't exist, it is created with
 * @gfp_flags and @node.  The returned io_context has its reference count
 * incremented.
 *
 * This function always goes through task_lock() and it's better to use
 * %current->io_context + get_io_context() for %current.
 */
struct io_context *uSnap_get_task_io_context(struct task_struct *task,
				       gfp_t gfp_flags, int node)
{
	struct io_context *ioc;

	might_sleep_if(gfpflags_allow_blocking(gfp_flags));

	do {
		task_lock(task);
		ioc = task->io_context;
		if (likely(ioc)) {
			get_io_context(ioc);
			task_unlock(task);
			return ioc;
		}
		task_unlock(task);
	} while (!uSnap_create_task_io_context(task, gfp_flags, node));

	return NULL;
}


