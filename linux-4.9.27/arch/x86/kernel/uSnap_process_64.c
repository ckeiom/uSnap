#include <linux/uSnap.h>
#include <linux/ptrace.h>
#include <linux/sched.h>

#include <asm/switch_to.h>

int uSnap_copy_thread_tls(unsigned long sp,
		unsigned long arg, struct task_struct *p, unsigned long tls, struct task_struct *to_copy)
{
	int err;
	struct pt_regs *childregs;
	struct fork_frame *fork_frame;
	struct inactive_task_frame *frame;

	p->thread.sp0 = (unsigned long)task_stack_page(p) + THREAD_SIZE;
	childregs = task_pt_regs(p);
	fork_frame = container_of(childregs, struct fork_frame, regs);
	frame = &fork_frame->frame;
	frame->bp = 0;
	frame->ret_addr = (unsigned long) ret_from_fork;
	p->thread.sp = (unsigned long) fork_frame;
	p->thread.io_bitmap_ptr = NULL;


	p->thread.gsindex = to_copy->thread.gsindex;
	p->thread.gsbase = p->thread.gsindex ? 0 : to_copy->thread.gsbase;
	p->thread.fsindex = to_copy->thread.fsindex;
	p->thread.fsbase = p->thread.fsindex ? 0 : to_copy->thread.fsbase;
	p->thread.es = to_copy->thread.es;
	p->thread.ds = to_copy->thread.ds;
	memset(p->thread.ptrace_bps, 0, sizeof(p->thread.ptrace_bps));

	frame->bx = 0;
	*childregs = *task_pt_regs(to_copy);
	if(childregs->orig_ax >= 0)
	{
		switch(childregs->ax)
		{
			case -ERESTARTNOHAND:
			case -ERESTARTSYS:
			case -ERESTARTNOINTR:
				childregs->ax = childregs->orig_ax;
				childregs->ip -= 2;
				break;
			case -ERESTART_RESTARTBLOCK:
				childregs->ax = -EINTR;
				break;
		}
	}

	if (sp)
		childregs->sp = sp;

	err = -ENOMEM;
	if (unlikely(test_tsk_thread_flag(to_copy, TIF_IO_BITMAP))) {
		p->thread.io_bitmap_ptr = kmemdup(to_copy->thread.io_bitmap_ptr,
						  IO_BITMAP_BYTES, GFP_KERNEL);
		if (!p->thread.io_bitmap_ptr) {
			p->thread.io_bitmap_max = 0;
			return -ENOMEM;
		}
		set_tsk_thread_flag(p, TIF_IO_BITMAP);
	}

	err = 0;

	return err;
}
