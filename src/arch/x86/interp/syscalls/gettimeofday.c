/* 
 * gettimeofday.c
 * by WN @ Aug. 12, 2010
 */

#include "syscall_handler.h"

#include <linux/time.h>
#include <common/debug.h>

#ifndef PRE_LIBRARY
DEF_HANDLER(gettimeofday)
{
	TRACE(LOG_SYSCALL, "gettimeofday\n");
	int r = regs->eax;
	if (r < 0)
		return 0;

	if (regs->ebx != 0)
		BUFFER((void*)(regs->ebx), sizeof(struct timeval));
	if (regs->ecx != 0)
		BUFFER((void*)(regs->ecx), sizeof(struct timezone));
	return 0;
}
#endif

// vim:ts=4:sw=4

