/* 
 * code_cache.h
 * by WN @ Mar. 12, 2010
 */

#ifndef __CODE_CACHE_H
#define __CODE_CACHE_H

#include <config.h>
#include <common/defs.h>
#include <common/list.h>
#include <interp/dict.h>

#include <stdint.h>

#define BE_CONDITIONAL		(1)
#define BE_UNCONDITIONAL	(0)
#define BE_DIRECT			(2)
#define BE_INDIRECT			(0)

enum exit_type {
	EXIT_COND_DIRECT = BE_CONDITIONAL | BE_DIRECT,
	EXIT_COND_INDIRECT = BE_CONDITIONAL,
	EXIT_UNCOND_DIRECT = BE_DIRECT,
	EXIT_UNCOND_INDIRECT = 0,
};

/* code block is saved in codecache */
struct code_block_t {
	/* the real entry of the code */
	/* the key in code cache */
	void * entry;
	/* exit_inst_addr is used for code recompilation */
	void * exit_inst_addr;
	/* a pointer to the first byte after
	 * the unmodified code */
	void * ori_code_end;
	enum exit_type exit_type;
	uint8_t __code[];
};

/* code_cache_t is storged in TLS */
struct tls_code_cache_t {
	struct dict_t * cache_dict;
	struct obj_page_head * code_blocks;
	struct code_block_t * last_ud_block;
};

extern void
clear_code_cache(struct tls_code_cache_t * cc);

extern void
init_code_cache(void);
#endif

// vim:ts=4:sw=4

