/* 
 * interp/replayer_syscall.h
 * by WN @ Jun. 28, 2010
 *
 * syscall replayer helper in interp
 */

#ifndef INTERP_SYSCALL_REPLAYER_H
#define INTERP_SYSCALL_REPLAYER_H

#include <common/defs.h>
#include <xasm/marks.h>

/* 
 * send a mark to host to indicate the begin of a system call
 */
extern void
initiate_syscall_read(void);

/* 
 * send a mark to host to indicate the finish of a system call
 */
extern void
finish_syscall_read(void);


extern void
read_syscall_data(void * ptr, size_t len);

#define def_read_syscall_TYPE(t, tn)	\
	inline static t			\
	read_syscall_##tn(void)		\
	{				\
		t v;			\
		read_syscall_data(&v, sizeof(v));	\
		return v;				\
	}

def_read_syscall_TYPE(void *, ptr)
def_read_syscall_TYPE(uint32_t, u32)
def_read_syscall_TYPE(uint16_t, u16)
def_read_syscall_TYPE(uint8_t, u8)
def_read_syscall_TYPE(int, int)

#undef def_read_syscall_TYPE

#endif

// vim:ts=4:sw=4

