/* 
 * brk.c
 * by WN @ Jul. 21, 2010
 */

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>

#ifdef PRE_LIBRARY
/* nothing to do */
#endif

#ifdef POST_LIBRARY
int
post_brk(struct pusha_regs * regs)
{
	/* save nothing */
	return 0;
}
#endif

#ifdef REPLAY_LIBRARY
int
replay_brk(struct pusha_regs * regs)
{
	/* reissue this syscall */
	int retval = regs->eax;
	int err = INTERNAL_SYSCALL_int80(brk, 1, regs->ebx);
	CASSERT(REPLAYER_TARGET, err == retval,
			"brk inconsistent: 0x%x should be 0x%x\n", err, retval);
	return 0;
}
#endif

// vim:ts=4:sw=4

