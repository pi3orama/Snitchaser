/* 
 * rt_sigaction.c
 * by WN @ Jul. 12, 2010
 *
 * when user calls rt_sigaction, execute it in pre_handler and
 * restore handler back immediately. pre_handler return non-zero
 * to skip actual syscall. in post handler, save output sigaction.
 */

#include "syscall_handler.h"
#include <common/debug.h>
#include <xasm/tls.h>
#include <xasm/syscall.h>
#include <xasm/string.h>

#include <interp/arch_signal.h>
#include <xasm/signal_numbers.h>

#ifdef PRE_LIBRARY

/* pre handler:
 *
 * returning 1 indicates jmp to post_processing immediately
 * */

int
pre_rt_sigaction(struct pusha_regs * regs)
{
	/* don't execute rt_sigaction in logger.S' code:
	 * return non-zero*/
	int sig = regs->ebx;
	struct k_sigaction * act = (void*)regs->ecx;
	struct k_sigaction * oact = (void*)regs->edx;
	struct k_sigaction act_backup;
	size_t sigsetsize = regs->esi;

	/* act and oact may same */
	if (act != NULL)
		act_backup = *act;

	int ret = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
			sig, act, oact, sigsetsize);

	regs->eax = ret;
	if (ret != 0) {
		/* nothing to do */
		return 1;
	}


	/* collect information and sigaction back */
	struct thread_private_data * tpd = get_tpd();
	struct k_sigaction * tpd_act = &(tpd->sigactions[sig - 1]);

	if (oact != NULL)
		memcpy(oact, tpd_act, sizeof(*tpd_act));

	if (act == NULL)
		return 1;

	/* act not null: reset tpd's sigaction and set real sigaction back */
	struct k_sigaction real_action;
	if (act_backup.sa_flags & SA_RESTORER)
		FATAL(LOG_SYSCALL, "doesn't support target reset restorer\n");
	if (act_backup.sa_flags & SA_ONSTACK)
		FATAL(LOG_SYSCALL, "doesn't support signal stack\n");

	if (act_backup.sa_mask.sig[1] & SET_SIGKILL_REPLACE_MASK) {
		WARNING(LOG_SYSCALL, "try to block SIGKILL_REPLACE in sigaction\n");
		act_backup.sa_mask.sig[1] &= UNSET_SIGKILL_REPLACE_MASK;
	}

	real_action.sa_flags = act_backup.sa_flags | SA_RESTORER;
	if (act_backup.sa_flags & SA_SIGINFO) {
		real_action.sa_handler = arch_wrapper_rt_sighandler;
		real_action.sa_restorer = arch_wrapper_rt_sigreturn;
	} else {
		real_action.sa_handler = arch_wrapper_sighandler;
		real_action.sa_restorer = arch_wrapper_sigreturn;
	}

	real_action.sa_mask.sig[0] = RECORD_PROCMASK_0;
	real_action.sa_mask.sig[1] = RECORD_PROCMASK_1;

	int err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
			sig, &real_action, NULL, sizeof(k_sigset_t));
	assert(err == 0);
	memcpy(tpd_act, &act_backup, sizeof(*tpd_act));

	return 1;
}

#endif

#if (defined POST_LIBRARY) || (defined REPLAY_LIBRARY)
DEF_HANDLER(rt_sigaction)
{
	/* save oact is enough */
	int ret = regs->eax;
	if (ret == 0) {
		struct k_sigaction * oact = (void*)regs->edx;
		if (oact != NULL)
			BUFFER(oact, sizeof(*oact));
	}
	return 0;
}
#endif

// vim:ts=4:sw=4

