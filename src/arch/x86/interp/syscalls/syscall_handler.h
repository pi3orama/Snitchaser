/* 
 * syscall_handler.h
 * by WN @ May. 31, 2010
 */

#ifndef SYSCALL_HANDLER_H
#define SYSCALL_HANDLER_H

#if 0
#include <common/defs.h>
#include <interp/logger.h>
#include <xasm/processor.h>
#endif

#include <linux/types.h>

/* 
 * include as less files as possible, because we need  kernel structures
 */

/* test compilation flags */
#ifdef PRE_LIBRARY
# ifdef POST_LIBRARY
#  error defines PRE_LIBRARY and POST_LIBRARY together!
# endif
# ifdef REPLAY_LIBRARY
#  error defines PRE_LIBRARY and REPLAY_LIBRARY together!
# endif
#endif
#ifdef POST_LIBRARY
# ifdef REPLAYER_LIBRARY
#  error defines POST_LIBRARY and REPLAYER_LIBRARY together!
# endif
#endif

#ifndef ATTR_UNUSED
# define ATTR_UNUSED __attribute__((unused))
#endif

#ifndef DEFINED_PUSHA_REGS
#define DEFINED_PUSHA_REGS
/* same as xasm/processor.h */
struct pusha_regs {
	uint32_t eflags;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
};
#endif

#ifdef POST_LIBRARY

/* interp/logger.h */
extern void
append_buffer(void * data, size_t size);

# define DEF_HANDLER(name)	int post_##name(struct pusha_regs * regs ATTR_UNUSED)
# define INT_VAL(x)		({int ___x = (int)(x); append_buffer(&___x, sizeof(int)); ___x;})
# define PTR_VAL(x)		({void * ___x = (void*)(x); append_buffer(&___x, sizeof(void *)); ___x;})
# define BUFFER(p, s)	do {	\
	if ((p) != NULL) \
		append_buffer((p), (s)); \
} while(0)
#endif

#ifdef REPLAY_LIBRARY


/* defined in interp/syscall_replayer.h */
extern void
read_syscall_data(void * ptr, size_t len);

# define DEF_HANDLER(name)	int replay_##name(struct pusha_regs * regs ATTR_UNUSED)
# define INT_VAL(x)		({read_syscall_data(&(x), sizeof(int)); (x);})
# define PTR_VAL(x)		({read_syscall_data(&(x), sizeof(void*)); (void*)(x);})
# define BUFFER(p, s)	do {	\
	if ((p) != NULL)	\
		read_syscall_data((p), (s));	\
} while(0)
#endif

#define EAX_AS_INT		(INT_VAL(regs->eax))
#define EAX_AS_PTR		(PTR_VAL(regs->eax))

#endif


// vim:ts=4:sw=4

