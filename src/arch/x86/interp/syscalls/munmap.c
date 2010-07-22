/* 
 * munmap.c
 * by WN @ Jul. 21, 2010
 */


#include "syscall_handler.h"
#include <asm/mman.h>
#include <xasm/syscall.h>
#include <common/debug.h>

#ifdef PRE_LIBRARY
/* nothing to do */
#endif

#ifdef POST_LIBRARY
int
post_munmap(struct pusha_regs * regs)
{
	/* do nothing */
	return 0;
}
#endif

#ifdef REPLAY_LIBRARY
int
replay_munmap(struct pusha_regs * regs)
{
	TRACE(LOG_SYSCALL, "replay munmap\n");
	int r = regs->eax;
	if (r > 0) {
		INTERNAL_SYSCALL_int80(munmap, 2,
				regs->ebx, regs->ecx);
	}
	return 0;
}
#endif

// vim:ts=4:sw=4

