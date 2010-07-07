/* 
 * arch_signal.c
 * by WN @ Jul. 07, 2010
 */


#include <interp/arch_signal.h>
#include <common/debug.h>

#include <xasm/syscall.h>
#include <xasm/signal_numbers.h>

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

void
arch_reset_sigmask(k_sigset_t mask)
{
	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask,
			4, SIG_SETMASK, &mask, NULL,
			sizeof(mask));
	assert(err == 0);
}


// vim:ts=4:sw=4

