/* 
 * poll.c
 * by WN @ Jul. 25, 2010
 */

#include <asm/poll.h>

#include "syscall_handler.h"
#include <common/debug.h>

#ifndef PRE_LIBRARY

DEF_HANDLER(poll)
{
	int r = regs->eax;
	if (r <= 0)
		return 0;

	void * ufds = (void*)(regs->ebx);
	uint32_t nfds = regs->ecx;
	BUFFER(ufds, nfds * sizeof(struct pollfd));
	return 0;
}

#endif


// vim:ts=4:sw=4

