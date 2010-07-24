/* 
 * fstat64.c
 * by WN @ May. 31, 2010
 */

#include "syscall_handler.h"

#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>

#include <common/debug.h>

#ifndef PRE_LIBRARY
DEF_HANDLER(stat64)
{
	TRACE(LOG_SYSCALL, "stat64\n");
	int r = regs->eax;
	if (r >= 0)
		BUFFER((void*)(regs->ecx), sizeof(struct stat64));
	return 0;
}
#endif

// vim:ts=4:sw=4

