/* 
 * clock_getres.c
 * by WN @ Jul. 24, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>

struct k_timespec {
	long	ts_sec;
	long	ts_nsec;
};

#ifndef PRE_LIBRARY
DEF_HANDLER(clock_getres)
{
	int r = regs->eax;
	if (r < 0)
		return 0;
	uintptr_t tp = regs->ecx;
	BUFFER((void*)(tp), sizeof(struct k_timespec));
	return 0;
}
#endif

// vim:ts=4:sw=4

