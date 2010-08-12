/* 
 * logger.h
 * by WN @ Mar. 17, 2010
 */

#ifndef __LOGGER_H
#define __LOGGER_H

#include <interp/mm.h>
#include <interp/compress.h>

#include <xasm/logger.h>
#include <common/spinlock.h>

#define LOG_PAGES_NR	(1024*10)

#define MAX_LOGGER_SIZE	(LOG_PAGES_NR * PAGE_SIZE - LOGGER_ADDITIONAL_BYTES)

/* 
 * TLS support for logger:
 * %fs:OFFSET_LOG_SPACE: the bytes buffer can hold
 * %fs:OFFSET_LOG_BUFFER: the buffer pointer
 * %fs:OFFSET_LOG_BUFFER_CURRENT: the buffer pointer
 */
#define MAX_LOGGER_FN	(128)
#define MAX_CKPT_FN		(MAX_LOGGER_FN)
struct tls_logger {
	struct spinlock_t logger_lock;
	/* check logger return addr */
	void * check_buffer_return;
	/* check logger nuffer */
	void * check_logger_buffer;

	/* use 3 pointers to simplify asm code */
	/* log buffer should contain at least 4 bytes
	 * additional space */
	void * log_buffer_start;
	void * log_buffer_current;
	void * log_buffer_end;

	struct tls_compress compress;

	/* the logger and checkpoint file */
	char log_fn[MAX_LOGGER_FN];
	char ckpt_fn[MAX_CKPT_FN];
};

/* this tag is flushed into logger file before the real data */
struct log_block_tag {
	uint32_t real_sz;
	uint32_t compressed_sz;
};

/* create logger's buffer and buffer size;
 * open logger file */
extern void
init_logger(struct tls_logger * tl, int pid, int tid);

extern void
reset_logger(struct tls_logger * tl, int pid, int tid);

extern void
reset_ckpt_log_names(bool_t dead);

/* close current log: when one thread exit, close its log */
/* close logger can be called multiple times */
extern void
close_logger(struct tls_logger * tl);

extern void
do_check_logger_buffer(void);

extern void
flush_logger(void);

extern void
flush_all_logger(void);

extern void
append_buffer(void * data, size_t size);

#define def_append_buffer_TYPE(t, tn)	\
	inline static void					\
	append_buffer_##tn(t x)				\
	{									\
		append_buffer(&x, sizeof(x));	\
	}

def_append_buffer_TYPE(void *, ptr)
def_append_buffer_TYPE(uint32_t, u32)
def_append_buffer_TYPE(uint16_t, u16)
def_append_buffer_TYPE(uint8_t, u8)
def_append_buffer_TYPE(int, int)

#undef def_append_buffer_TYPE


/* ********* replay code *********** */
extern void
load_from_buffer(void * data, size_t size);

#define COMPRESSED_DATA_MARK	(0x1)
#define UNCOMPRESSED_DATA_MARK	(0x0)

#endif

// vim:ts=4:sw=4

