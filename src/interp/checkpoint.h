/* 
 * checkpoint.h
 * by WN @ Apr. 06, 2010
 */

#ifndef __CHECKPOINT_H
#define __CHECKPOINT_H

#ifndef __KERNEL__
# include <config.h>
# include <stdint.h>
# include <interp/logger.h>
# include <xasm/processor.h>
#else
# include <linux/types.h>
# include <interp/logger.h>
# include <xasm/processor.h>
#endif

#define CKPT_MAGIC	"SNITCHASER CKPT"
#define CKPT_MAGIC_SZ	(16)

#define MAX_PROC_MAPS_FILE_SZ	(1024*1024)

struct checkpoint_head {
	char magic[CKPT_MAGIC_SZ];
	uintptr_t argp_first;
	uintptr_t argp_last;
	uintptr_t brk;
	uint32_t pid;
	uint32_t tid;
	int tnr;
	struct user_desc thread_area[GDT_ENTRY_TLS_ENTRIES];
	struct reg_state reg_state;
};


struct mem_region {
	uintptr_t start;
	uintptr_t end;
	uint32_t prot;
	uint64_t offset;
	unsigned int fn_sz;
	char fn[];
};

#define region_sz(r)	((r)->end - (r)->start)
#define region_p_real_sz(r)	((int*)((void*)(&(r)[1]) + (r)->fn_sz))
#define region_data(r)	((void*)(region_p_real_sz(r)) + sizeof(int))
#define next_region(r)	((struct mem_region *)(region_data(r) + *region_p_real_sz(r)))

/* MEM_REGIONS_END_MARK should be a number larger than 0xc0000000 and
 * not aligned to PAGE_SIZE */
#define MEM_REGIONS_END_MARK	(0xf4f3f2f1)
#define CKPT_END_MARK	(0x2e444e45)

/* in some situation, such as log flushing, we already be in a child process
 * so needn't fork again. set real_fork to FALSE here. */
void
fork_make_checkpoint(struct pusha_regs * regs, void * eip);

/* make checkpoint whitout fork */
/* make_checkpoint still unmap all unneeded pages, so the caller
 * should guarantee that when calling make_checkpoint, it has already
 * forked and will exit immediately.
 * */
void
make_checkpoint(struct pusha_regs * regs, void * eip);

void
make_dead_checkpoint(struct pusha_regs * regs, void * eip);
#endif

// vim:ts=4:sw=4

