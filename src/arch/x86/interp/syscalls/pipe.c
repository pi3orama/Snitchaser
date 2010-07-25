/* 
 * pipe.c
 * by WN @ Jul. 25, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>

#ifndef PRE_LIBRARY
DEF_HANDLER(pipe)
{
	int r = regs->eax;
	if (r < 0)
		return 0;
	BUFFER((void*)(regs->ebx), 2 * sizeof(unsigned long));
	return 0;
}
#endif

// vim:ts=4:sw=4

