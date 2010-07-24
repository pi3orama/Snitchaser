/* 
 * uname.c
 * by WN @ Jul. 24, 2010
 */

#include <linux/utsname.h>
#include "syscall_handler.h"
#include <common/debug.h>

#ifndef PRE_LIBRARY

DEF_HANDLER(uname)
{
	TRACE(LOG_SYSCALL, "uname\n");
	int r = regs->eax;
	if (r >= 0)
		BUFFER((void*)(regs->ebx), sizeof(struct old_utsname));
	return 0;
}

#endif

// vim:ts=4:sw=4

