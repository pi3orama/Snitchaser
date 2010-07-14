/* 
 * arch_signal.c
 * by WN @ Jul. 07, 2010
 */


#include <interp/arch_signal.h>
#include <common/debug.h>

#include <xasm/syscall.h>
#include <xasm/signal_numbers.h>
#include <xasm/tls.h>
#include <xasm/string.h>
#include <xasm/marks.h>

#include <interp/logger.h>

k_sigset_t
arch_replay_mask_signals(void)
{
	k_sigset_t all_mask = {
		.sig = {0, 0}};
	k_sigset_t old_mask = {
		.sig = {0, 0}};

	sigaddset(&all_mask, GDBSERVER_NOTIFICATION);
	sigaddset(&all_mask, SIGINT);
	sigaddset(&all_mask, SIGSEGV);
	sigaddset(&all_mask, SIGFPE);
	sigaddset(&all_mask, SIGILL);

	all_mask.sig[0] = ~all_mask.sig[0];
	all_mask.sig[1] = ~all_mask.sig[1];

	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask,
			4, SIG_BLOCK, &all_mask, &old_mask,
			sizeof(all_mask));
	assert(err == 0);
	return old_mask;
}

k_sigset_t
arch_set_sigmask(k_sigset_t mask)
{
	k_sigset_t old_set;
	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask,
			4, SIG_SETMASK, &mask, &old_set,
			sizeof(mask));
	assert(err == 0);
	return old_set;
}

void
arch_init_signal(void)
{
	struct thread_private_data * tpd = get_tpd();
	uint32_t mask[2] = RECORD_PROCMASK;

	memcpy(tpd->block_sigmask, mask, sizeof(mask));
	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask, 4,
			SIG_SETMASK, mask, &(tpd->unblock_sigmask),
			sizeof(k_sigset_t));
	assert(err == 0);

	for (int i = 1; i <= 64; i++) {

		if ((i == SIGKILL) || (i == SIGSTOP))
			continue;

		int err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
				i, NULL, &(tpd->sigactions[i - 1]),
				sizeof(k_sigset_t));
		assert(err == 0);

		/* install new signal handler for it according to
		 * SA_INFO flag */
		struct k_sigaction * pa = &(tpd->sigactions[i - 1]);
		struct k_sigaction new_action;

		if (pa->sa_flags & SA_RESTORER)
			FATAL(SIGNAL, "doesn't support target reset restorer\n");

		new_action.sa_flags = pa->sa_flags & SA_RESTORER;
		if (pa->sa_flags & SA_SIGINFO) {
			new_action.sa_handler = arch_wrapper_rt_sighandler;
			new_action.sa_restorer = arch_wrapper_rt_sigreturn;
		} else {
			new_action.sa_handler = arch_wrapper_sighandler;
			new_action.sa_restorer = arch_wrapper_sigreturn;
		}
		memcpy(&new_action.sa_mask, mask, sizeof(new_action.sa_mask));

		err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
				i, &new_action, NULL, sizeof(k_sigset_t));
		assert(err == 0);
	}

}

static void
signal_terminate(int num, struct thread_private_data * tpd)
{

	WARNING(SIGNAL, "Never tested code\n");
	WARNING(SIGNAL, "Need signaled eip\n");
	WARNING(SIGNAL, "terminated by signaled %d\n", num);

	flush_logger();
	append_buffer_u32(SIGNAL_MARK);
	append_buffer_int(num);
	append_buffer_u32(SIGNAL_TERMINATE);
	flush_logger();

	/* we needn't clean tls and code cache because all threads
	 * will be killed by this signal. */

	/* raise the signal again to kill myself: */
	/* reinstall sigactions */
	struct k_sigaction act;
	/* DFL action should be 'kill'. */
	/* those signal never terminate process */
	if ((num == SIGSTOP) ||
			(num == SIGTSTP) ||
			(num == SIGTTIN) ||
			(num == SIGTTOU) ||
			(num == SIGURG) ||
			(num == SIGWINCH) ||
			(num == SIGCHLD))
		FATAL(SIGNAL, "wrong signal %d terminates process\n", num);

	memset(&act, '\0', sizeof(act));
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;

	int err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
			num, &act, NULL, sizeof(k_sigset_t));
	assert(err == 0);

	/* block all signals */
	k_sigset_t set;
	memset(&set, (char)0x0, sizeof(set));

	/* unblock this signal */
	k_sigaddset(&set, num);

	err = INTERNAL_SYSCALL_int80(rt_sigprocmask, 4,
			SIG_UNBLOCK, &set, NULL, sizeof(set));
	assert(err == 0);

	/* raise the signal again */
	WARNING(SIGNAL, "will kill the whole thread group...\n");

	err = INTERNAL_SYSCALL_int80(kill, 2, tpd->pid, num);
	FATAL(SIGNAL, "We shouldn't get here!!!\n");
	INTERNAL_SYSCALL_int80(exit, 1, -1);
	/* never return */
}

static void
signal_stop(int num, struct thread_private_data * tpd)
{
	WARNING(SIGNAL, "stopped by signal %d\n", num);
	/* kill itself by an SIGSTOP: */
	int err = INTERNAL_SYSCALL_int80(kill, 2, tpd->pid, SIGSTOP);
	assert(err == 0);
}

/* if return 1, sigreturn (or rt_sigreturn) */
/* if return 2, terminate */
static int
common_wrapper_sighandler(int num, void * frame, size_t frame_sz,
		struct thread_private_data * tpd)
{
	WARNING(SIGNAL, "signal %d: never tested code\n", num);
	/* check whether to terminate */
	struct k_sigaction * act = &(tpd->sigactions[num - 1]);

	if (act->sa_handler == SIG_IGN) {
		/* ignore actions:  */
		if ((num == 32) || (num == 33))
			signal_terminate(num, tpd);
		/* else: ignore */
		return 1;
	} else if (act->sa_handler == SIG_DFL) {
		if ((num == SIGSTOP) ||
				(num == SIGTSTP) ||
				(num == SIGTTIN) ||
				(num == SIGTTOU)) {
			signal_stop(num, tpd);
			return 1;
		} else if ((num == SIGURG) ||
				(num == SIGWINCH) ||
				(num == SIGCHLD)) {
			return 1;
		} else {
			signal_terminate(num, tpd);
		}
	} else {
		FATAL(SIGNAL, "unimplemented\n");
	}
	return 0;
}

int
do_arch_wrapper_sighandler(void)
{
	struct thread_private_data * tpd = get_tpd();
	struct sigframe * frame = tpd->old_stack_top;
	return common_wrapper_sighandler(frame->sig, frame, sizeof(*frame), tpd);
}

int
do_arch_wrapper_rt_sighandler(void)
{
	struct thread_private_data * tpd = get_tpd();
	struct rt_sigframe * rt_frame = tpd->old_stack_top;
	return common_wrapper_sighandler(rt_frame->sig, rt_frame,
			sizeof(*rt_frame), tpd);
}

void
do_arch_wrapper_sigreturn(void)
{
	FATAL(SIGNAL, "unimplemented\n");
}

void
do_arch_wrapper_rt_sigreturn(void)
{
	FATAL(SIGNAL, "unimplemented\n");
}


// vim:ts=4:sw=4

