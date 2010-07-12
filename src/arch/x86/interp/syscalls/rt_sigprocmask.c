/* 
 * sigprocmask.c
 * by WN @ Jun. 01, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>
#include <xasm/tls.h>
#include <xasm/syscall.h>
#include <xasm/string.h>

#ifdef PRE_LIBRARY
/* do nothing */
#endif

#define SIGSET_SZ	(2 * sizeof(uint32_t))

#ifdef POST_LIBRARY
int
post_rt_sigprocmask(struct pusha_regs * regs)
{
	int retval = regs->eax;
	int how = regs->ebx;
	uint32_t * set = (uint32_t *)(regs->ecx);
	uint32_t * oset = (uint32_t *)(regs->edx);

	/* if the call success,
	 * we should:
	 * 1. adjust unblock mask in tpd --> set to tpd->current_mask
	 * 		(see logger.S --> BLOCK_SIGPROCMASK)
	 * 2. reset oset field
	 *
	 * the output set should have
	 * been stored at tpd->current_mask */
	if (retval >= 0) {
		struct thread_private_data * tpd = get_tpd();

		/* oset should be set to old unblock_sigmask */
		if (oset != NULL)
			assert(memcmp(oset, tpd->unblock_sigmask,
						SIGSET_SZ) == 0);

		/* unblock_sigmask should have been changed by this syscall,
		 * the new signal mask should have been stored into
		 * tpd->current_sigmask. see logger.S BLOCK_SIGPROCMASK*/
		memcpy(tpd->unblock_sigmask, tpd->current_sigmask,
				SIGSET_SZ);

		/* store oset */
		if (oset != NULL)
			BUFFER(oset, SIGSET_SZ);
	}
	return 0;
}
#endif

#ifdef REPLAY_LIBRARY
int
replay_rt_sigprocmask(struct pusha_regs * regs)
{
	int retval = regs->eax;
	int how = regs->ebx;
	uint32_t * oset = (uint32_t *)(regs->edx);
	if (retval >= 0) {
		if (oset != NULL)
			BUFFER(oset, SIGSET_SZ);
	}
	return 0;
}
#endif

// vim:ts=4:sw=4

