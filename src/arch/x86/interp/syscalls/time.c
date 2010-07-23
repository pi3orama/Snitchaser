/* 
 * time.c
 * by WN @ Jul. 22, 2010
 */

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>

#ifndef PRE_LIBRARY

DEF_HANDLER(time)
{
	TRACE(LOG_SYSCALL, "time\n");
	if (regs->ebx != 0)
		BUFFER((void*)(regs->ebx), sizeof(time_t));
	return 0;
}

#endif

// vim:ts=4:sw=4

