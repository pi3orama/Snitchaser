/* 
 * log_syscalls.c
 * by WN @ May. 25, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <interp/logger.h>
#include <xasm/tls.h>
#include <xasm/marks.h>
#include <xasm/string.h>
#include <xasm/unistd.h>
#include <common/replay/socketpair.h>
#include <interp/syscall_replayer.h>

#include "log_syscalls.h"
#include "syscall_table.h"

static int
pre_log_syscall(struct pusha_regs * regs)
{
	TRACE(LOG_SYSCALL, "begin syscall, eax=%d, ebx=%d, eflags=0x%x\n",
			regs->eax, regs->ebx, regs->eflags);

	struct thread_private_data * tpd = get_tpd();

	int nr = regs->eax;
	assert((nr >= 0) && (nr < SYSCALL_TABLE_SZ));

	tpd->current_syscall_nr = nr;
	append_buffer_u32(SYSCALL_MARK);
	append_buffer_u32(nr);

	if (syscall_table[nr].pre_handler) {
		return syscall_table[nr].pre_handler(regs);
	} else {
		FATAL(LOG_SYSCALL, "doesn't support syscall %d\n", nr);
	}
}


static int
post_log_syscall(struct pusha_regs * regs)
{
	struct thread_private_data * tpd = get_tpd();
	int nr = tpd->current_syscall_nr;

	TRACE(LOG_SYSCALL, "post syscall, eax=%u(0x%x), ebx=%u, eflags=0x%x\n",
			regs->eax, regs->eax, regs->ebx, regs->eflags);

	/* for 'clone' in child process, don't append buffer */
	if (!(((nr == __NR_clone) || (nr == __NR_fork)) && (regs->eax == 0))) {
		/* put a 'no-signal-mark' */
		append_buffer_u32(NO_SIGNAL_MARK);
		/* put regs */
		struct pusha_regs real_regs = *regs;
		real_regs.esp = (uintptr_t)(tpd->old_stack_top);
		append_buffer(&real_regs, sizeof(real_regs));
	}

	tpd->current_syscall_nr = -1;

	if (syscall_table[nr].post_handler) {
		return syscall_table[nr].post_handler(regs);
	} else {
		FATAL(LOG_SYSCALL, "doesn't support syscall %d\n", nr);
	}
}

/* precall:
 *
 * return 0: normal_pre_processing
 * return 1: post_processing (don't issue the syscall)
 * return 2: issue syscall, doesn't allow signal
 * */
int
pre_log_syscall_entry(struct pusha_regs regs)
{
	return pre_log_syscall(&regs);
}

/* post call: if return 0, record normally.
 * if return 1, branch to tpd->target, never log. */
int
post_log_syscall_entry(struct pusha_regs regs)
{
	return post_log_syscall(&regs);
}

// vim:ts=4:sw=4

