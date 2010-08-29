/* 
 * setitimer.c
 * by WN @ Aug. 26, 2010
 */

#include <linux/time.h>

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>

#ifndef PRE_LIBRARY

DEF_HANDLER(setitimer)
{
	TRACE(LOG_SYSCALL, "setitimer\n");
	int r = regs->eax;
	if (r == 0) {
		struct itimerval * ovalue = (void*)(regs->edx);
		if (ovalue != NULL) {
			BUFFER(ovalue, sizeof(struct itimerval));
		}
	}
	return 0;
}

#endif

// vim:ts=4:sw=4

