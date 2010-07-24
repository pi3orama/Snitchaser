/* 
 * fcntl64.c
 * by WN @ Jul. 24, 2010
 */


#include <linux/fs.h>

#include "syscall_handler.h"
#include <common/debug.h>

#ifndef PRE_LIBRARY
DEF_HANDLER(fcntl64)
{
	TRACE(LOG_SYSCALL, "fcntl64\n");
	int r = regs->eax;
	if (r < 0)
		return 0;

	int cmd = regs->ecx;
	uintptr_t arg = regs->edx;
	switch (cmd) {
	case F_GETLK64:
		BUFFER((void*)arg, sizeof(struct flock64));
		break;
	case F_GETLK:
		BUFFER((void*)arg, sizeof(struct flock));
		break;
	default:
		break;
	}
	return 0;
}
#endif

// vim:ts=4:sw=4

