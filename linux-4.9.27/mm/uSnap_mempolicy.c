#include <linux/uSnap.h>
#include <linux/cpuset.h>
#include <linux/mempolicy.h>

static void uSnap_mpol_relative_nodemask(nodemask_t *ret, const nodemask_t *orig,
				   const nodemask_t *rel)
{
	nodemask_t tmp;
	nodes_fold(tmp, *orig, nodes_weight(*rel));
	nodes_onto(*ret, tmp, *rel);
}

static inline int uSnap_mpol_store_user_nodemask(const struct mempolicy *pol)
{
	return pol->flags & MPOL_MODE_FLAGS;
}

static int uSnap_mpol_new_interleave(struct mempolicy *pol, const nodemask_t *nodes)
{
	if (nodes_empty(*nodes))
		return -EINVAL;
	pol->v.nodes = *nodes;
	return 0;
}

static int uSnap_mpol_new_preferred(struct mempolicy *pol, const nodemask_t *nodes)
{
	if (!nodes)
		pol->flags |= MPOL_F_LOCAL;	/* local allocation */
	else if (nodes_empty(*nodes))
		return -EINVAL;			/*  no allowed nodes */
	else
		pol->v.preferred_node = first_node(*nodes);
	return 0;
}

static int uSnap_mpol_new_bind(struct mempolicy *pol, const nodemask_t *nodes)
{
	if (nodes_empty(*nodes))
		return -EINVAL;
	pol->v.nodes = *nodes;
	return 0;
}

static void uSnap_mpol_rebind_default(struct mempolicy *pol, const nodemask_t *nodes,
				enum mpol_rebind_step step)
{
}

static void uSnap_mpol_rebind_nodemask(struct mempolicy *pol, const nodemask_t *nodes,
				 enum mpol_rebind_step step)
{
	nodemask_t tmp;

	if (pol->flags & MPOL_F_STATIC_NODES)
		nodes_and(tmp, pol->w.user_nodemask, *nodes);
	else if (pol->flags & MPOL_F_RELATIVE_NODES)
		uSnap_mpol_relative_nodemask(&tmp, &pol->w.user_nodemask, nodes);
	else {
		/*
		 * if step == 1, we use ->w.cpuset_mems_allowed to cache the
		 * result
		 */
		if (step == MPOL_REBIND_ONCE || step == MPOL_REBIND_STEP1) {
			nodes_remap(tmp, pol->v.nodes,
					pol->w.cpuset_mems_allowed, *nodes);
			pol->w.cpuset_mems_allowed = step ? tmp : *nodes;
		} else if (step == MPOL_REBIND_STEP2) {
			tmp = pol->w.cpuset_mems_allowed;
			pol->w.cpuset_mems_allowed = *nodes;
		} else
			BUG();
	}

	if (nodes_empty(tmp))
		tmp = *nodes;

	if (step == MPOL_REBIND_STEP1)
		nodes_or(pol->v.nodes, pol->v.nodes, tmp);
	else if (step == MPOL_REBIND_ONCE || step == MPOL_REBIND_STEP2)
		pol->v.nodes = tmp;
	else
		BUG();

	if (!node_isset(current->il_next, tmp)) {
		current->il_next = next_node_in(current->il_next, tmp);
		if (current->il_next >= MAX_NUMNODES)
			current->il_next = numa_node_id();
	}
}

static void uSnap_mpol_rebind_preferred(struct mempolicy *pol,
				  const nodemask_t *nodes,
				  enum mpol_rebind_step step)
{
	nodemask_t tmp;

	if (pol->flags & MPOL_F_STATIC_NODES) {
		int node = first_node(pol->w.user_nodemask);

		if (node_isset(node, *nodes)) {
			pol->v.preferred_node = node;
			pol->flags &= ~MPOL_F_LOCAL;
		} else
			pol->flags |= MPOL_F_LOCAL;
	} else if (pol->flags & MPOL_F_RELATIVE_NODES) {
		uSnap_mpol_relative_nodemask(&tmp, &pol->w.user_nodemask, nodes);
		pol->v.preferred_node = first_node(tmp);
	} else if (!(pol->flags & MPOL_F_LOCAL)) {
		pol->v.preferred_node = node_remap(pol->v.preferred_node,
						   pol->w.cpuset_mems_allowed,
						   *nodes);
		pol->w.cpuset_mems_allowed = *nodes;
	}
}

static const struct mempolicy_operations {
	int (*create)(struct mempolicy *pol, const nodemask_t *nodes);
	void (*rebind)(struct mempolicy *pol, const nodemask_t *nodes,
			enum mpol_rebind_step step);
}uSnap_mpol_ops[MPOL_MAX];

static const struct mempolicy_operations uSnap_mpol_ops[MPOL_MAX] = {
	[MPOL_DEFAULT] = {
		.rebind = uSnap_mpol_rebind_default,
	},
	[MPOL_INTERLEAVE] = {
		.create = uSnap_mpol_new_interleave,
		.rebind = uSnap_mpol_rebind_nodemask,
	},
	[MPOL_PREFERRED] = {
		.create = uSnap_mpol_new_preferred,
		.rebind = uSnap_mpol_rebind_preferred,
	},
	[MPOL_BIND] = {
		.create = uSnap_mpol_new_bind,
		.rebind = uSnap_mpol_rebind_nodemask,
	},
};



static void uSnap_mpol_rebind_policy(struct mempolicy *pol, const nodemask_t *newmask,
				enum mpol_rebind_step step)
{
	if (!pol)
		return;
	if (!uSnap_mpol_store_user_nodemask(pol) && step == MPOL_REBIND_ONCE &&
	    nodes_equal(pol->w.cpuset_mems_allowed, *newmask))
		return;

	if (step == MPOL_REBIND_STEP1 && (pol->flags & MPOL_F_REBINDING))
		return;

	if (step == MPOL_REBIND_STEP2 && !(pol->flags & MPOL_F_REBINDING))
		BUG();

	if (step == MPOL_REBIND_STEP1)
		pol->flags |= MPOL_F_REBINDING;
	else if (step == MPOL_REBIND_STEP2)
		pol->flags &= ~MPOL_F_REBINDING;
	else if (step >= MPOL_REBIND_NSTEP)
		BUG();

	uSnap_mpol_ops[pol->mode].rebind(pol, newmask, step);
}

static struct mempolicy *__uSnap_mpol_dup(struct mempolicy *old, struct task_struct *to_copy)
{
	struct mempolicy *new = &uSnap_kern->mempolicy;

	/* task's mempolicy is protected by alloc_lock */
	if (old == to_copy->mempolicy) {
		task_lock(to_copy);
		*new = *old;
		task_unlock(to_copy);
	} else
		*new = *old;

	if (current_cpuset_is_being_rebound()) {
		nodemask_t mems = cpuset_mems_allowed(current);
		if (new->flags & MPOL_F_REBINDING)
			uSnap_mpol_rebind_policy(new, &mems, MPOL_REBIND_STEP2);
		else
			uSnap_mpol_rebind_policy(new, &mems, MPOL_REBIND_ONCE);
	}
	atomic_set(&new->refcnt, 1);
	return new;
}

struct mempolicy* uSnap_mpol_dup(struct mempolicy* pol, struct task_struct *to_copy)
{
	if(pol)
		pol = __uSnap_mpol_dup(pol, to_copy);
	return pol;
}
