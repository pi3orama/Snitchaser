/* 
 * futex.c
 * by WN @ Jul. 21, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>

#include <linux/futex.h>

#ifndef PRE_LIBRARY
DEF_HANDLER(futex)
{
	TRACE(LOG_SYSCALL, "futex\n");
	int r = regs->eax;
	if (r < 0)
		return 0;

	uint32_t uaddr = regs->ebx;
	int op = regs->ecx;
	uint32_t uaddr2 =regs->edi;

	int cmd = op & FUTEX_CMD_MASK;
	BUFFER((void*)uaddr, sizeof(uint32_t));

	switch (cmd) {
	case FUTEX_REQUEUE:
	case FUTEX_CMP_REQUEUE:
	case FUTEX_WAKE_OP:
		BUFFER((void*)uaddr2, sizeof(uint32_t));
	default:
		break;
	}
	return 0;
}
#endif

// vim:ts=4:sw=4

