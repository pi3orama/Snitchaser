/* 
 * replay_syscalls.c
 * by WN @ Jun. 30, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <interp/logger.h>
#include <interp/replayer.h>
#include <xasm/tls.h>
#include <xasm/marks.h>
#include <xasm/string.h>
#include <common/replay/socketpair.h>
#include <interp/syscall_replayer.h>

#include "log_syscalls.h"
#include "syscall_table.h"

static void
print_regs(struct pusha_regs * regs)
{
#define print_reg(xx)	\
	WARNING(REPLAYER_TARGET, #xx "=0x%x\n", regs->xx)

	print_reg(eax);
	print_reg(ebx);
	print_reg(ecx);
	print_reg(edx);
	print_reg(esi);
	print_reg(edi);
	print_reg(ebp);
	print_reg(esp);
#undef print_reg
}

/* called by replay_syscall_helper in logger.S */
void
do_replay_syscall_helper(struct pusha_regs * regs)
{
	/* we never return, so direct reset regs->esp */
	struct thread_private_data * tpd = get_tpd();
	regs->esp = (uintptr_t)(tpd->old_stack_top);
	TRACE(REPLAYER_TARGET, "in do_replay_syscall_helper\n");


	/* check regs */
	struct pusha_regs ori_regs;
	sock_recv(&ori_regs, sizeof(ori_regs));

	if (memcmp(&ori_regs, regs, sizeof(*regs)) != 0) {
		WARNING(REPLAYER_TARGET,
				"register set is different from original execution\n");
		WARNING(REPLAYER_TARGET, "original regs:\n");
		print_regs(&ori_regs);
		WARNING(REPLAYER_TARGET, "current regs:\n");
		print_regs(regs);
		*regs = ori_regs;
	}

	int nr = regs->eax;
	assert((nr >= 0) && (nr < SYSCALL_TABLE_SZ));
	TRACE(REPLAYER_TARGET, "replay syscall %d\n", nr);

	if (syscall_table[nr].replay_handler) {


		initiate_syscall_read();
		syscall_table[nr].replay_handler(regs);
		finish_syscall_read();

		sock_send(regs, sizeof(*regs));

		/* in interp/replayer.c */
		notify_gdbserver();

	} else {
		FATAL(REPLAYER_TARGET, "doesn't support syscall %d\n", nr);
	}
}

// vim:ts=4:sw=4

