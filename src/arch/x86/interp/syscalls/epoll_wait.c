/* 
 * epoll_wait.c
 * by WN @ Jul. 27, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>

#ifdef __x86_64__
#define EPOLL_PACKED __attribute__((packed))
#else
#define EPOLL_PACKED
#endif

struct epoll_event {
	uint32_t events;
	uint64_t data;
} EPOLL_PACKED;


#ifndef PRE_LIBRARY
DEF_HANDLER(epoll_wait)
{
	int r = regs->eax;
	if (r <= 0)
		return 0;

	uint32_t events = regs->ecx;
	int maxevents = regs->edx;
	assert(r <= maxevents);

	BUFFER((void*)events, r * sizeof(struct epoll_event));
	return 0;
}
#endif

// vim:ts=4:sw=4

