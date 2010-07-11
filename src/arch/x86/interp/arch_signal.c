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

	for (int i = 1; i <= 64; i++) {
		int err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
				i, NULL, &(tpd->sigactions[i - 1]),
					sizeof(k_sigset_t));
		assert(err == 0);
	}

	memcpy(tpd->block_sigmask, mask, sizeof(mask));
	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask, 4,
			SIG_SETMASK, mask, &(tpd->unblock_sigmask),
			sizeof(k_sigset_t));
	assert(err == 0);
}

// vim:ts=4:sw=4

