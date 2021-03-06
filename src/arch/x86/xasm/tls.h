/* 
 * tls.h
 * by WN @ Mar. 09, 2010
 */

#ifndef __X86_TLS_H
#define __X86_TLS_H

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/assert.h>
#include <common/list.h>
#include <xasm/syscall.h>
#include <xasm/signal_helper.h>
#include <interp/dict.h>
#include <interp/logger.h>
#include <interp/code_cache.h>
#include <interp/compress.h>

/* thread thread stack: from 0x3000, each for 3 pages(8k + 4k)
 * the upper 1 page is protected to prevent stack overflow
 * */
/* x86 have at most 8192 ldt entry, so at most 8192 tls.
 * each thread's tls section is predefined:
 * each thread has 2 pages for stack, the thread self's info
 * is located at the bottom of the stack.
 * if we have 8192 (0x2000) threads, the tls stack is from 0x3000 to 0x6003000;
 * normal program always put its first section at 0x8048000, so
 * the stack never clobber code section.
 */

struct thread_private_data {
	/* before we enter TLS, the stack point.
	 * use it for restore stack */
	void * old_stack_top;
	/* used to temporary save some registers */
	uint32_t reg_saver1;
	uint32_t reg_saver2;
	/* when enter TLS code, target indicate the real address of the exit
	 * target. when exiting TLS code, target indicate the target address
	 * in code cache */
	void * target;

	/* func pointers */
	/* first run */
	void * first_branch;
	void * real_branch;
	void * int80_syscall_entry;
	void * vdso_syscall_entry;
	void * real_vdso_syscall_entry;

	struct tls_logger logger;
	struct tls_code_cache_t code_cache;
	/* still need: head address of dict; head address of code cache */
	/* tnr is thread identifier using in snitchaser */
	uint32_t tid, pid;
	unsigned int tnr;
	/* we also need some code cache stuff */
	/* when enter snitchaser's code, stack should be set to it */
	struct thread_private_data * stack_top;

	/* ------------ signal section begin ------------- */
	/* sigprocmask, x86 has 64 signals */
	uint32_t block_sigmask[2];
	uint32_t unblock_sigmask[2];
	/* used for processing sigprocmask and rt_sigprocmask.
	 * see logger.S -- BLOCK_SIGPROCMASK and processing of
	 * those syscalls */
	uint32_t current_sigmask[2];
	struct k_sigaction sigactions[64];
	/* ------------ signal section end --------------- */

	struct list_head list;
	/* the start position of argp, inited when loading and never changes */
	uintptr_t argp_first;
	uintptr_t argp_last;

	/* default is -1, set in init_tls */
	/* also see log_syscall */
	int current_syscall_nr;

	/* configurations */
	bool_t conf_trace_fork;
	bool_t conf_untrace_clone;
	int conf_selection;

	/* for untrace clone, no_record_signals is true */
	bool_t no_record_signals;
	int fs_val;
	volatile int futex_data;

	/* set in append_region (checkpoint.c) when append stack region
	 * to stack */
	void * stack_top_addr;

	/* access: *fs:(0x3000 - 4); stores: the base (lowest)
	 * address of the TLS section */
	void * tls_base;
};

extern void
unset_share_context(void);

extern struct list_head tpd_list_head;

#define TLS_STACK_SIZE		(0x3000)
#define MAX_TLS_ADDR_FS		(TLS_STACK_SIZE)
#define TPD_ADDR_FS			(MAX_TLS_ADDR_FS - sizeof(struct thread_private_data))
#define TNR_TO_STACK(n)		((void*)(((n) + 1) * TLS_STACK_SIZE))
#define STACK_TO_TNR(a)		(((unsigned int)(a)) / TLS_STACK_SIZE - 1)
#define GUARDER_LENGTH		(0x1000)

#ifndef PAGE_SIZE
# define PAGE_SIZE	(4096)
#endif


/* init_tls and clear_tls should be called
 * with thread original stack, not the snitchaser's
 * TLS stack! */
extern void
init_tls(void);

extern struct thread_private_data *
create_new_tls(void);

extern void
copy_init_base_tpd(struct thread_private_data * dst,
		struct thread_private_data * src);

extern void
clone_build_tpd(struct thread_private_data * tpd);

extern void clear_tls(void);
/* for trace fork: updates pid and tid, resets logger */
extern void update_tls(void);

/* only setup the tls area */
extern void replay_init_tls(unsigned int tnr);

extern void lock_tls(void);
extern void unlock_tls(void);

static __AI void *
get_tls_base(void)
{
	void * r;
	asm volatile (
			"movl %%fs:%c[b_addr], %[o]\n"
			: [o] "=r" (r)
			: [b_addr] "i" (TPD_ADDR_FS +
				offsetof(struct thread_private_data, tls_base))
			);
	return r;
}

static __AI struct thread_private_data *
get_tpd(void)
{
	void * r;
	asm volatile (
			"movl %%fs:%c[tpd_addr], %[o]\n"
			: [o] "=r" (r)
			: [tpd_addr] "i" (TPD_ADDR_FS +
				offsetof(struct thread_private_data, stack_top))
			);
	return r;
}

static __AI uint32_t
get_esp(void)
{
	uint32_t r;
	asm volatile ("movl %%esp, %[r]\n"
			: [r] "=rm" (r));
	return r;
}

/* for fork-based checkpoint used */
/* for each tpd structure, unmap all related pages,
 * including logger's page and code cache's pages */
/* it also unmap the tpd stack of other threads. */
void
unmap_tpds_pages(void);

void
sync_sighandler(int sig);

#endif

// vim:ts=4:sw=4

