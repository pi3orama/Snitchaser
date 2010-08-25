/* 
 * pread64.c
 * by WN @ Aug. 25, 2010
 */

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>

#ifndef PRE_LIBRARY

DEF_HANDLER(pread64)
{
	TRACE(LOG_SYSCALL, "pread64\n");
	int r = regs->eax;
	if (r > 0)
		BUFFER((void*)(regs->ecx), r);
	return 0;
}

#endif

// vim:ts=4:sw=4

