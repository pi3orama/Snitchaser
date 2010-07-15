/* 
 * fstat64.c
 * by WN @ May. 31, 2010
 */

#include "syscall_handler.h"

#include <asm/uaccess.h>
#include <asm/unistd.h>

#include <common/debug.h>

struct k_timespec {
	long	ts_sec;
	long	ts_nsec;
};

#ifndef PRE_LIBRARY
DEF_HANDLER(nanosleep)
{
	TRACE(LOG_SYSCALL, "nanosleep\n");
	int r = regs->eax;
	struct k_timespec * ospec = (void*)(regs->ecx);
	BUFFER(ospec, sizeof(*ospec));
	return 0;
}
#endif

// vim:ts=4:sw=4

