/* 
 * _newselect.c
 * by WN @ Jul. 24, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>

#define FDS_BITPERLONG	(8*sizeof(long))
#define FDS_LONGS(nr)	(((nr)+FDS_BITPERLONG-1)/FDS_BITPERLONG)
#define FDS_BYTES(nr)	(FDS_LONGS(nr)*sizeof(long))

#ifndef PRE_LIBRARY
DEF_HANDLER(_newselect)
{
	TRACE(LOG_SYSCALL, "_new_select\n");

	int r = regs->eax;
	int n = regs->ebx;
	void * inp = (void*)(regs->ecx);
	void * outp = (void*)(regs->edx);
	void * exp = (void*)(regs->esi);

	int fd_bytes = FDS_BYTES(n);
	if (inp != 0)
		BUFFER(inp, fd_bytes);
	if (outp != 0)
		BUFFER(outp, fd_bytes);
	if (exp != 0)
		BUFFER(exp, fd_bytes);
	return 0;
}
#endif

// vim:ts=4:sw=4

