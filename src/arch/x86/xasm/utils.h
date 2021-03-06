/* 
 * xasm/utils.h
 */

#ifndef __ASM_UTILS_H
#define __ASM_UTILS_H

#include <common/defs.h>
#include <xasm/syscall.h>
#include <stdarg.h>

#define FDPRINTF_MAX	(4096)

extern int ATTR(format(printf, 2, 3))
fdprintf(int fd, const char * fmt, ...);

extern int
vfdprintf(int fd, const char * fmt, va_list args);

/* don't call it __printf because I want to avoid a macro name
 * '__printf' defined in kernel's code include/linux/compiler-gcc.h */
extern int ATTR(format(printf, 1, 2))
__xprintf(const char * fmt, ...);

extern void
relocate_interp(void);

/*
 * __exit() stops the whole thread group
 * */
static ATTR(noreturn) inline void
__exit(int n)
{
	INTERNAL_SYSCALL_int80(exit_group, 1, n);
	while(1);
}

#ifndef PAGE_SHIFT
# define PAGE_SHIFT	(12)
#endif

#ifndef PAGE_SIZE
# define PAGE_SIZE	(1 << (PAGE_SHIFT))
#endif

extern void *
alloc_pages(int nr, bool_t exec);

extern void
free_pages(void * start, int nr);
#endif
// vim:ts=4:sw=4

