/* 
 * getcwd.c
 * by WN @ Jul. 24, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>

#ifndef PRE_LIBRARY
DEF_HANDLER(getcwd)
{
	int r = regs->eax;
	if (r < 0)
		return 0;
	BUFFER((void*)(regs->ebx), r);
	return 0;
}
#endif

// vim:ts=4:sw=4

