/* 
 * tls.c
 * by WN @ Mar. 09, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/spinlock.h>
#include <common/bithacks.h>
#include <common/list.h>
#include <interp/code_cache.h>
#include <interp/logger.h>
#include <interp/auxv.h>
#include <interp/user_entry.h>
/* struct user_desc */
#include <xasm/types_helper.h>
#include <xasm/logger.h>
#include <xasm/compiler.h>
#include <xasm/tls.h>
#include <xasm/syscall.h>
#include <xasm/utils.h>
#include <xasm/string.h>

#include <sys/mman.h>

/* 
 * thread_map is used to indicate the usage of ldt number. the
 * ldt slot is free if the corresponding bit is unset.
 * */
/* all tls related control work is protected by __tls_ctl_lock */

#define MAX_THREADS	(1 << 13)
static DEF_SPINLOCK_UNLOCKED(__tls_ctl_lock);

/* all tls struct is linked on this list. */
LIST_HEAD(tpd_list_head);

/* all static array should have been inited to 0 */
#define SZ_MAP	(MAX_THREADS / sizeof(uint32_t))
static uint32_t thread_map[SZ_MAP];

static bool_t
tls_in_shared_context = TRUE;


void
unset_share_context(void)
{
	tls_in_shared_context = FALSE;
}

void
lock_tls(void)
{
	if (!tls_in_shared_context) {
		spin_lock_fake(&__tls_ctl_lock);
	} else {
		spin_lock(&__tls_ctl_lock);
	}
}

void
unlock_tls(void)
{
	if (!tls_in_shared_context) {
		spin_unlock_fake(&__tls_ctl_lock);
	} else {
		spin_unlock(&__tls_ctl_lock);
	}
}

/* 
 * find_set_free_slot shoule be called at lock context
 */
static int
find_set_free_slot(void)
{
	assert(spin_is_locked(&__tls_ctl_lock));
	for (int i = 0; i < (int)SZ_MAP; i++) {
		if (thread_map[i] != 0xffffffff) {
			int n = i * 32 + last_0_pos(thread_map[i]);
			thread_map[i] = set_last_0(thread_map[i]);
			return n;
		}
	}
	FATAL(TLS, "TLS slot is full\n");
}

#define READ_LDT		(0)
#define WRITE_LDT_OLD		(1)
#define READ_DEFAULT_LDT	(2)
#define WRITE_LDT			(0x11)

static void
write_ldt(struct user_desc * desc)
{
	int err;
	err = INTERNAL_SYSCALL_int80(modify_ldt, 3,
			WRITE_LDT, desc, sizeof(*desc));
	assert(err == 0);
}

static void
clear_tls_slot(unsigned int tnr)
{
	int nr = tnr / 32;
	int n = tnr % 32;
	thread_map[nr] = unset_bit_n(thread_map[nr], n);

	/* clear the ldt slot */
	struct thread_private_data * tpd = get_tpd();
	if (tpd->tnr == tnr) {
		/* clear 'fs' */
		asm volatile ("movw %w0, %%fs\n"
				:
				: "R" (0)
				: "memory");
	}
	struct user_desc desc;
	/* this is an 'empty' desc */
	desc.entry_number = tnr;
	desc.base_addr = 0;
	desc.limit = 0;
	desc.contents = 0;
	desc.read_exec_only = 1;
	desc.seg_32bit = 0;
	desc.limit_in_pages = 0;
	desc.seg_not_present = 1;
	desc.useable = 0;
	write_ldt(&desc);
}

/* 
 * __setup_tls_area will alloc a new thread_private_data and link it
 * into tpd_list_head
 *
 * __setup_tls_area shoule be called at lock context
 */
static struct thread_private_data *
__setup_tls_area(unsigned int tnr)
{

	assert(spin_is_locked(&__tls_ctl_lock));

	/* setup fs: alloc tls stack and init thread private data */
	void * stack_base_addr = TNR_TO_STACK(tnr);

	/* alloc 2 pages from stack_base_addr */
	void * p = (void*)INTERNAL_SYSCALL_int80(mmap2, 6,
			stack_base_addr,
			TLS_STACK_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
			-1, 0);
	assert(p == stack_base_addr);
	/* mprotect */
	int err = INTERNAL_SYSCALL_int80(mprotect, 3,
			p, GUARDER_LENGTH, PROT_NONE);
	assert(err == 0);

	/* setup ldt */
	struct user_desc desc;
	desc.entry_number = tnr;
	desc.base_addr = (unsigned long int)(stack_base_addr);
	desc.limit = TLS_STACK_SIZE;
	desc.seg_32bit = 1;
	desc.contents = 0;
	desc.read_exec_only = 0;
	desc.limit_in_pages = 0;
	desc.seg_not_present = 0;
	desc.useable = 1;
	write_ldt(&desc);

	/* set tpd */
	struct thread_private_data * tpd =
		(void*)(stack_base_addr) +
		TLS_STACK_SIZE -
		sizeof(struct thread_private_data);

	memset(tpd, '\0', sizeof(*tpd));
	tpd->tnr = tnr;
	tpd->stack_top = tpd;
	tpd->tls_base = stack_base_addr;

	/* segregister: 16bits
	 * 
	 * X X X X X X X X X X X X X X X X
	 *                           ^ ^ ^
	 *                           | |-|
	 *        <--- Index ---    TI RPL
	 * 3 is 'user'
	 * */
	tpd->fs_val = (tnr * 8 + 4 + 3);
	tpd->no_record_signals = FALSE;

	/* link tpd into tpd_list_head */
	list_add(&tpd->list, &tpd_list_head);
	return tpd;
}

static struct thread_private_data *
setup_tls_area(unsigned int tnr)
{
	assert(spin_is_locked(&__tls_ctl_lock));
	struct thread_private_data * tpd = __setup_tls_area(tnr);
	/* finally load fs */
	asm volatile ("movw %w0, %%fs\n" :: "R" (tpd->fs_val));
	return tpd;
}


static void
build_tpd(struct thread_private_data * tpd)
{
	tpd->real_branch = real_branch;
	tpd->first_branch = first_branch;

	if (auxv_info.p_sysinfo != NULL) {
		tpd->real_vdso_syscall_entry = *auxv_info.p_sysinfo;
	} else {
		/* we are loaded as a normal shared object */	
		asm volatile("movl %%gs:0x10, %0" :
				"=r" (tpd->real_vdso_syscall_entry));
	}

	tpd->int80_syscall_entry = int80_syscall_entry;
	tpd->vdso_syscall_entry = vdso_syscall_entry;

	/* init code cache */
	init_code_cache(&tpd->code_cache);
	/* init logger */
	init_logger(&tpd->logger, tpd->pid, tpd->tid);
}

void
init_tls(void)
{
	int pid, tid;
	pid = INTERNAL_SYSCALL_int80(getpid, 0);
	tid = INTERNAL_SYSCALL_int80(gettid, 0);
	DEBUG(TLS, "init TLS storage: pid=%d, tid=%d\n",
			tid, pid);

	lock_tls();
	int n = find_set_free_slot();
	DEBUG(TLS, "TLS slot: %d\n", n);

	struct thread_private_data * tpd = setup_tls_area(n);
	DEBUG(TLS, "TPD resides at %p\n", tpd);
	unlock_tls();

	tpd->pid = pid;
	tpd->tid = tid;
	tpd->current_syscall_nr = -1;
	build_tpd(tpd);
}

/* return the value of 'fs' */
struct thread_private_data *
create_new_tls(void)
{
	lock_tls();
	int n = find_set_free_slot();
	VERBOSE(TLS, "TLS slot for currently unused tpd: %d\n", n);
	struct thread_private_data * tpd = __setup_tls_area(n);
	unlock_tls();
	return tpd;
}

void
copy_init_base_tpd(struct thread_private_data * dst,
		struct thread_private_data * src)
{
#define cpy_field(f)	do {(dst->f) = (src->f);}while(0)

	cpy_field(first_branch);
	cpy_field(real_branch);
	cpy_field(int80_syscall_entry);
	cpy_field(vdso_syscall_entry);
	cpy_field(real_vdso_syscall_entry);

	memset(&(dst->logger), '\0', sizeof(dst->logger));
	memset(&(dst->code_cache), '\0', sizeof(dst->code_cache));

	memcpy(dst->block_sigmask, src->block_sigmask,
			sizeof(dst->block_sigmask));
	memcpy(dst->unblock_sigmask, src->unblock_sigmask,
			sizeof(dst->unblock_sigmask));
	memcpy(dst->current_sigmask, src->current_sigmask,
			sizeof(dst->current_sigmask));
	memcpy(dst->sigactions, src->sigactions,
			sizeof(dst->sigactions));

	cpy_field(argp_first);
	cpy_field(argp_last);
	cpy_field(conf_trace_fork);
	cpy_field(conf_trace_clone);
	cpy_field(no_record_signals);

#undef cpy_field
}

void
clone_build_tpd(struct thread_private_data * tpd)
{
	tpd->pid = INTERNAL_SYSCALL_int80(getpid, 0);
	tpd->tid = INTERNAL_SYSCALL_int80(gettid, 0);
	init_code_cache(&tpd->code_cache);
	init_logger(&tpd->logger, tpd->pid, tpd->tid);
}


void
replay_init_tls(unsigned int tnr)
{
	lock_tls();
	setup_tls_area(tnr);
	unlock_tls();
}

static void
unmap_tpd_pages(struct thread_private_data * tpd)
{
	clear_code_cache(&tpd->code_cache);
	close_logger(&tpd->logger);
}

static void
unmap_tpd(struct thread_private_data * tpd)
{
	assert(spin_is_locked(&__tls_ctl_lock));
	int err;
	/* release build don't use tnr */
	unsigned int tnr = tpd->tnr;
	clear_tls_slot(tnr);

	err = INTERNAL_SYSCALL_int80(munmap, 2,
			tpd->tls_base, TLS_STACK_SIZE);
	assert(err == 0);
	DEBUG(TLS, "tls for tnr=%d is cleared\n", tnr);
}

void
clear_tls(void)
{
	struct thread_private_data * tpd = get_tpd();
	unsigned int tnr = tpd->tnr;
	void * stack_base = get_tls_base();
	assert((uint32_t)stack_base == (uint32_t)TNR_TO_STACK(tnr));

	void * cur_esp;
	asm volatile ("movl %%esp, %0" : "=Rm" (cur_esp));
	assert((cur_esp < stack_base) || (cur_esp > stack_base + TLS_STACK_SIZE));

	unmap_tpd_pages(tpd);
	/* unmap this tls */
	lock_tls();
	list_del(&tpd->list);

	/* unmap pages from stack_base to stack_base + TLS_STACK_SIZE */
	/* 
	 * FIXME: after tpd structure unmapped, the stack becomes invalidated.
	 */
	unmap_tpd(tpd);

	unlock_tls();
	return;
}

void
update_tls(void)
{
	/* reset pid and tid, reset filename of ckpt and log,
	 * clear (not flush) log data */

	struct thread_private_data * tpd = get_tpd();
	pid_t pid, tid;
	pid = INTERNAL_SYSCALL_int80(getpid, 0);
	tid = INTERNAL_SYSCALL_int80(gettid, 0);
	DEBUG(TLS, "update TLS storage: pid=%d, tid=%d\n",
			tid, pid);

	tpd->pid = pid;
	tpd->tid = tid;

	reset_logger(&tpd->logger, pid, tid);
}

void
unmap_tpds_pages(void)
{
	struct thread_private_data * pos = NULL, *n;
	struct thread_private_data * my_tpd = get_tpd();

	lock_tls();
	list_for_each_entry_safe(pos, n, &tpd_list_head, list) {
		/* remove tls pages */
		unmap_tpd_pages(pos);
		if (pos != my_tpd) {
			list_del(&pos->list);
			unmap_tpd(pos);
		}
	}
	unlock_tls();
}

// vim:ts=4:sw=4

