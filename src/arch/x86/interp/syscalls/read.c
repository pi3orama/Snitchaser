/* 
 * read.c
 * by WN @ Jul. 21, 2010
 */

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>

#ifndef PRE_LIBRARY

DEF_HANDLER(read)
{
	TRACE(LOG_SYSCALL, "read\n");
	int r = regs->eax;
	if (r > 0)
		BUFFER((void*)(regs->ecx), r);
	return 0;
}

#endif

// vim:ts=4:sw=4

