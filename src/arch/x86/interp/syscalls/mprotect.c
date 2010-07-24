/* 
 * mprotect.c
 * by WN @ Jul. 24, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>
#include <xasm/syscall.h>

#ifdef PRE_LIBRARY
/* do nothing */
#endif

#ifdef POST_LIBRARY
/* do nothing */
DEF_HANDLER(mprotect)
{
	return 0;
}
#endif

#ifdef REPLAY_LIBRARY
DEF_HANDLER(mprotect)
{
	int r = regs->eax;
	if (r >= 0) {
		INTERNAL_SYSCALL_int80(mprotect, 3,
				regs->ebx, regs->ecx, regs->edx);
	}
	return 0;
}
#endif


// vim:ts=4:sw=4

