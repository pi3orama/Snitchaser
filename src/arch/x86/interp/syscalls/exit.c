/* 
 * exit.c
 * by WN @ Jun. 01, 2010
 */

#include "syscall_handler.h"

#include <common/debug.h>
#include <interp/logger.h>
#include <interp/checkpoint.h>
#include <xasm/tls.h>

#ifdef PRE_LIBRARY
int
pre_exit(struct pusha_regs * regs)
{
	/* we need to flush log */
	VERBOSE(SYSTEM, "program call exit(%d), flush log\n", regs->ebx);
	flush_logger();

	struct thread_private_data * tpd = get_tpd();
	make_dead_checkpoint(regs, tpd->target);

	/* clear tls */
	asm volatile (
			"movl %[exit_val], %%ebx\n"
			"movl %[old_esp], %%esp\n"
			"call clear_tls\n"
			"movl $1, %%eax\n"
			"int $0x80\n"
			:
			: [exit_val] "mr" (regs->ebx),
			  [old_esp] "mr" (tpd->old_stack_top));

	return 0;
}
#endif

#ifdef POST_LIBRARY
int
post_exit(struct pusha_regs * regs)
{
	/* never come here */
	return 0;
}
#endif

#ifdef REPLAY_LIBRARY
int
replay_exit(struct pusha_regs * regs)
{
	VERBOSE(SYSTEM, "program call exit(%d) and exit\n", regs->ebx);
	__exit(0);
	return 0;
}
#endif

// vim:ts=4:sw=4

