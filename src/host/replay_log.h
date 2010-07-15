/* 
 * replay_log.h
 * by WN @ Jun. 21, 2010
 *
 * log utils for replayer
 *
 */

#ifndef REPLAY_LOG_H
#define REPLAY_LOG_H

#include <common/defs.h>
#include <stdint.h>

void
open_log(const char * fn);

void
close_log(void);

void
read_log_full(void * buf, size_t size);

size_t
read_log(void * buf, size_t size);

void
uncompress_log(const char * log_fn, const char * out_fn);

#define def_read_TYPE_from_log(t, tn)	\
	inline static t						\
	read_##tn##_from_log(void)			\
	{									\
		t r;							\
		read_log_full(&r, sizeof(r));	\
		return r; 						\
	}

def_read_TYPE_from_log(uintptr_t, ptr)
def_read_TYPE_from_log(int, int)
def_read_TYPE_from_log(uint32_t, u32)
def_read_TYPE_from_log(uint16_t, u16)
def_read_TYPE_from_log(uint8_t, u8)

#undef def_read_TYPE_from_log

#if 0
inline static uintptr_t
read_ptr_from_log(void)
{
	uintptr_t r;
	read_log_full(&r, sizeof(r));
	return r;
}
#endif

bool_t
readahead_log(void * data, size_t sz);

inline static uintptr_t
readahead_log_ptr(void)
{
	uintptr_t ptr;
	if (readahead_log(&ptr, sizeof(ptr)))
		return ptr;
	return 0xffffffff;
}

#endif

// vim:ts=4:sw=4

