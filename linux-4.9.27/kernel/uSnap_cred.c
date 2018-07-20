#include <linux/string.h>
#include <linux/uSnap.h>
#include <linux/slab.h>
#include <linux/user_namespace.h>
#include <linux/security.h>

static struct cred *uSnap_prepare_creds(struct task_struct *to_copy)
{
	struct task_struct *task = to_copy;
	const struct cred *old;
	struct cred *new;

	new = &uSnap_kern->cred;

	old = task->cred;
	memcpy(new, old, sizeof(struct cred));

	atomic_set(&new->usage, 1);
	
	get_group_info(new->group_info);
	get_uid(new->user);
	get_user_ns(new->user_ns);

#ifdef CONFIG_KEYS
	key_get(new->session_keyring);
	key_get(new->process_keyring);
	key_get(new->thread_keyring);
	key_get(new->request_key_auth);
#endif

#ifdef CONFIG_SECURITY
	new->security = NULL;
#endif

	if (security_prepare_creds(new, old, GFP_KERNEL) < 0)
		goto error;
	validate_creds(new);
	return new;

error:
	abort_creds(new);
	return NULL;
}

int uSnap_copy_creds(struct task_struct *p, struct task_struct *to_copy)
{
	struct cred *new;

	if(
#ifdef CONFIG_KEYS
		!p->cred->thread_keyring &&
#endif
		1
	   ) 
	{
		p->real_cred = get_cred(p->cred);
		get_cred(p->cred);
		atomic_inc(&p->cred->user->processes);
		return 0;
	}

	new = uSnap_prepare_creds(to_copy);
	if (!new)
		return -ENOMEM;


#ifdef CONFIG_KEYS
	/* new threads get their own thread keyrings if their parent already
	 * had one */
	if (new->thread_keyring) {
		key_put(new->thread_keyring);
		new->thread_keyring = NULL;
	}

	/* The process keyring is only shared between the threads in a process;
	 * anything outside of those threads doesn't inherit.
	 */
	key_put(new->process_keyring);
	new->process_keyring = NULL;
#endif

	atomic_inc(&new->user->processes);
	p->cred = p->real_cred = get_cred(new);
	validate_creds(new);
	return 0;
}
