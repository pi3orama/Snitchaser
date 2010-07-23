/* 
 * time.c
 * by WN @ Jul. 22, 2010
 */

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>
#include <linux/types.h>

#ifndef PRE_LIBRARY

DEF_HANDLER(_llseek)
{
	TRACE(LOG_SYSCALL, "_llseek\n");
	uintptr_t presult = regs->esi;
	if ((int)(regs->eax) >= 0)
		if (presult != 0)
			BUFFER((void*)(presult), sizeof(loff_t));
	return 0;
}

#endif

// vim:ts=4:sw=4

