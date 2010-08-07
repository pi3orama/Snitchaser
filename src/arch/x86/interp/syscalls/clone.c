/* 
 * clone.c
 * by WN @ Jul. 25, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>
#include <xasm/tls.h>
#include <xasm/syscall.h>
#include <string.h>
#include <interp/arch_signal.h>
#include <interp/checkpoint.h>

/* from: linux/sched */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */
#define CLONE_NEWNS	0x00020000	/* New namespace group? */
#define CLONE_SYSVSEM	0x00040000	/* share system V SEM_UNDO semantics */
#define CLONE_SETTLS	0x00080000	/* create a new TLS for the child */
#define CLONE_PARENT_SETTID	0x00100000	/* set the TID in the parent */
#define CLONE_CHILD_CLEARTID	0x00200000	/* clear the TID in the child */
#define CLONE_DETACHED		0x00400000	/* Unused, ignored */
#define CLONE_UNTRACED		0x00800000	/* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID	0x01000000	/* set the TID in the child */
#define CLONE_STOPPED		0x02000000	/* Start in stopped state */
#define CLONE_NEWUTS		0x04000000	/* New utsname group? */
#define CLONE_NEWIPC		0x08000000	/* New ipcs */
#define CLONE_NEWUSER		0x10000000	/* New user namespace */
#define CLONE_NEWPID		0x20000000	/* New pid namespace */
#define CLONE_NEWNET		0x40000000	/* New network namespace */
#define CLONE_IO		0x80000000	/* Clone io context */

#include <xasm/signal_numbers.h>

static int
__pre_trace_fork(struct thread_private_data * tpd, struct pusha_regs * regs)
{
	/* don't allow signal arise after fork() */
	return 2;
}

static int
__post_trace_fork(struct thread_private_data * tpd, struct pusha_regs * regs)
{
	int r = regs->eax;
	if (r != 0)
		return 0;
	/* child process:
	 *
	 * update tls, remake checkpoint, clear log
	 * */
	update_tls();
	/* generate new checkpoints */
	fork_make_checkpoint(regs, tpd->target);
	return 0;
}


static int
__pre_untrace_fork(struct thread_private_data * tpd, struct pusha_regs * regs)
{
	/* untrace fork allows signal */
	return 0;
}

static int
__post_untrace_fork(struct thread_private_data * tpd, struct pusha_regs * regs)
{
	int r = regs->eax;
	/* this is parent process */
	if (r != 0)
		return 0;

	/* for child process:  */

	/* restore sighandler */
	arch_restore_signal();
	/* returns 1: run untraced */
	return 1;
}

static int
__pre_trace_clone(struct thread_private_data * tpd, struct pusha_regs * regs)
{
	FATAL(LOG_SYSCALL, "unimplemented\n");
	/* goto clone specific pre processing, see logger.S */
	return 3;
}

static int
__post_trace_clone(struct thread_private_data * tpd, struct pusha_regs * regs)
{
	FATAL(LOG_SYSCALL, "unimplemented\n");
	return 0;
}


static int
__pre_untrace_clone(struct thread_private_data * tpd, struct pusha_regs * regs)
{
#warning WORKING HERE!
	FATAL(LOG_SYSCALL, "unimplemented\n");
	/* goto clone specific pre processing, see logger.S */
	return 3;
}

static int
__post_untrace_clone(struct thread_private_data * tpd, struct pusha_regs * regs)
{
	FATAL(LOG_SYSCALL, "unimplemented\n");
	return 0;
}

#ifdef PRE_LIBRARY
int
pre_clone(struct pusha_regs * regs)
{
	/* the argument of clone is special:
	 *
	 * from process.c:
	 *
	 *	sys_clone(unsigned long clone_flags, unsigned long newsp,
	 *  void __user *parent_tid, void __user *child_tid, struct pt_regs *regs)
	 *
	 * form syscall_table_32.S:
	 * .long ptregs_clone
	 *
	 * from entry_32.S:
	 *
	 *	ptregs_clone:
	 *	leal 4(%esp),%eax
	 *	pushl %eax
	 *	pushl PT_EDI(%eax)
	 *	movl PT_EDX(%eax),%ecx
	 *	movl PT_ECX(%eax),%edx
	 *	movl PT_EBX(%eax),%eax
	 *	call sys_clone
	 *	addl $8,%esp
	 *	ret
	 * * */
	struct thread_private_data * tpd = get_tpd();
	uint32_t flags = regs->ebx;
	switch (flags) {
	case (CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD):
		if (tpd->conf_trace_fork) {
			return __pre_trace_fork(tpd, regs);
		} else {
			return __pre_untrace_fork(tpd, regs);
		}
	case (CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|
			CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|
			CLONE_CHILD_CLEARTID):
		if (tpd->conf_trace_fork) {
			return __pre_trace_clone(tpd, regs);
		} else {
			return __pre_untrace_clone(tpd, regs);
		}
	default:
		FATAL(LOG_SYSCALL, "unimplemented flag: 0x%x\n", flags);
	}
	return 0;
}
#endif

#ifdef POST_LIBRARY
int
post_clone(struct pusha_regs * regs)
{
	struct thread_private_data * tpd = get_tpd();
	uint32_t flags = regs->ebx;
	switch (flags) {
	case (CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD):
		if (tpd->conf_trace_fork) {
			return __post_trace_fork(tpd, regs);
		} else {
			return __post_untrace_fork(tpd, regs);
		}
	case (CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|
			CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|
			CLONE_CHILD_CLEARTID):
		if (tpd->conf_trace_fork) {
			return __post_trace_clone(tpd, regs);
		} else {
			return __post_untrace_clone(tpd, regs);
		}
	default:
		FATAL(LOG_SYSCALL, "unimplemented flag: 0x%x\n", flags);
	}
	return 0;
}
#endif


#ifdef REPLAY_LIBRARY
/* for replayer, clone is only a trivial syscall */
int
replay_clone(struct pusha_regs * regs)
{
	return 0;
}
#endif

// vim:ts=4:sw=4

