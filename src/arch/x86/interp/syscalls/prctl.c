/* 
 * prctl.c
 * by WN @ Aug. 25, 2010
 */

#include <linux/sched.h>
#include <linux/prctl.h>

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>
/* TASK_COMM_LEN */

#ifndef PRE_LIBRARY
DEF_HANDLER(prctl)
{
	TRACE(LOG_SYSCALL, "prctl\n");
	int r = regs->eax;
	if (r < 0)
		return 0;
	int option = regs->ebx;
	switch (option) {
	case PR_GET_PDEATHSIG:
	case PR_GET_UNALIGN:
	case PR_GET_FPEMU:
	case PR_GET_FPEXC:
	case PR_GET_ENDIAN:
	case PR_GET_TSC:
		BUFFER((void*)(regs->ecx), sizeof(int));
		return 0;
	case PR_GET_NAME:
		BUFFER(((void*)regs->ecx), TASK_COMM_LEN);
		return 0;
	default:
		return 0;
	}
}
#endif

// vim:ts=4:sw=4

