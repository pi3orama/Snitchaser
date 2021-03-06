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

#ifndef __KERNEL__
# include <stdint.h>
#else
# include <linux/types.h>
#endif

#define BE_CONDITIONAL		(1)
#define BE_UNCONDITIONAL	(0)
#define BE_DIRECT			(0)
#define BE_INDIRECT			(2)
#define BE_SYSCALL			(0x10)

enum exit_type {
	EXIT_COND_DIRECT = BE_CONDITIONAL | BE_DIRECT,
	EXIT_COND_INDIRECT = BE_CONDITIONAL | BE_INDIRECT,
	EXIT_UNCOND_DIRECT = BE_UNCONDITIONAL | BE_DIRECT,
	EXIT_UNCOND_INDIRECT = BE_UNCONDITIONAL | BE_INDIRECT,
	EXIT_SYSCALL = BE_SYSCALL,
};

/* code block is saved in codecache */
struct code_block_t {
	/* the real entry of the code */
	/* the key in code cache */
	void * entry;
	/* exit_inst_addr is used for code recompilation */
	void * exit_inst_addr;
	/* a pointer to the first byte after
	 * the unmodified code, used for recompile */
	void * ori_code_end;
	/* if this is a ud block, we replace the code
	 * after recompile_start with a single jmp in the
	 * 2nd pass compilation. */
	void * recompile_start;
	enum exit_type exit_type;
	/* first, check last_entry, if hit, don't compile */
	void * last_target_entry;
	void * last_target_code;
	uint8_t __code[];
};

/* code_cache_t is storged in TLS */
struct tls_code_cache_t {
	struct dict_t * cache_dict;
	struct obj_page_head * code_blocks;
	/* not only ud block should be record.
	 * We MUST save current_block because of
	 * signal handling: When signal arises,
	 * signal handler can know the exact signal point. */
	struct code_block_t * current_block;
	struct code_block_t first_fate_block;
};

extern void
clear_code_cache(struct tls_code_cache_t * cc);

extern void
init_code_cache(struct tls_code_cache_t * cc);
#endif

// vim:ts=4:sw=4

