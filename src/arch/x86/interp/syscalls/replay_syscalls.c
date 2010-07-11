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

	print_reg(eflags);
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

	int nr = sock_recv_int();
	assert(nr == (int)regs->eax);

	TRACE(REPLAYER_TARGET, "in replay syscall %d\n", nr);

	/* clear bit 21 of eflags */
	/* from intel manual: 
	 * ID: Identification (bit 21). The ability of a program or procedure to
	 * set or clear this flag indicates support for the CPUID instruction. */
	/* bit 21 in eflags is random when program start */
	struct pusha_regs new_regs;
	sock_recv(&new_regs, sizeof(new_regs));

	uintptr_t new_eip = sock_recv_ptr();

	/* sysenter syscall doesn't portect eflags. see entry_32.S
	 * kernel code */
	regs->eflags = new_regs.eflags;
	regs->eax = new_regs.eax;

	if (memcmp(&new_regs, regs, sizeof(*regs)) != 0) {
		WARNING(REPLAYER_TARGET,
				"register set is different from original execution\n");
		WARNING(REPLAYER_TARGET,
				"next eip is 0x%x\n", new_eip);
		WARNING(REPLAYER_TARGET, "new regs:\n");
		print_regs(&new_regs);
		WARNING(REPLAYER_TARGET, "current regs:\n");
		print_regs(regs);
		*regs = new_regs;
	}

	if (syscall_table[nr].replay_handler) {

		initiate_syscall_read();
		syscall_table[nr].replay_handler(regs);
		finish_syscall_read();

		/* in interp/replayer.c */
		notify_gdbserver();
	} else {
		FATAL(REPLAYER_TARGET, "doesn't support syscall %d\n", nr);
	}
}

// vim:ts=4:sw=4

