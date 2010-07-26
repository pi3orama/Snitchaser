/* 
 * waitpid.c
 * by WN @ Jul. 25, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>

#ifndef PRE_LIBRARY
DEF_HANDLER(waitpid)
{
	if (regs->ecx != 0)
		BUFFER((void*)(regs->ecx), sizeof(int));
	return 0;
}
#endif

// vim:ts=4:sw=4

