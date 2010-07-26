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
	VERBOSE(REPLAYER_TARGET, #xx "=0x%x\n", regs->xx)

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

#if 0
	/* if the syscall is broken by signal and resume
	 * after signal handler, nr != eax */
	assert(nr == (int)regs->eax);
#endif

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
	uint32_t old_eax = regs->eax;
	regs->eax = new_regs.eax;

	if (memcmp(&new_regs, regs, sizeof(*regs)) != 0) {
		/* see logger.S, RESET_SIGPROCMASK. a signal may raise right after
		 * the syscall of rt_sigprocmask. in this situation, registers in
		 * signal frame record sigprocmask's param, but eip is reset to
		 * the real syscall (see arch_signal.c, signal_handler). Therefore,
		 * durning replay, when signal return, eip will be reset to the
		 * real syscall instruction, but registers are kept. */
		/* NOTICE: after the 'int $0x80' in UNBLOCK_SIGPROCMASK,
		 * don't touch any memory! this is unrecordable. */
		VERBOSE(REPLAYER_TARGET,
				"register set is different from original execution\n");
		VERBOSE(REPLAYER_TARGET,
				"This is not a problem if we just finish a signal handler\n");
		VERBOSE(REPLAYER_TARGET,
				"next eip is 0x%x\n", new_eip);
		VERBOSE(REPLAYER_TARGET, "new regs:\n");
		print_regs(&new_regs);
		VERBOSE(REPLAYER_TARGET, "current regs:\n");
		print_regs(regs);
		VERBOSE(REPLAYER_TARGET, "old_eax=0x%x\n", old_eax);
		*regs = new_regs;
	}

	if (syscall_table[nr].replay_handler) {

		initiate_syscall_read();
		syscall_table[nr].replay_handler(regs);
		finish_syscall_read();

		/* in interp/replayer.c */
		TRACE(REPLAYER_TARGET, "replay syscall %d over\n", nr);
		notify_gdbserver();
	} else {
		FATAL(REPLAYER_TARGET, "doesn't support syscall %d\n", nr);
	}
}

// vim:ts=4:sw=4

