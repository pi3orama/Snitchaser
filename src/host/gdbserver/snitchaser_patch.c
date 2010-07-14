/* 
 * snitchaser_patch.c
 * by WN @ Jun. 16, 2010
 */

#include <common/debug.h>
#include <host/exception.h>

#include <host/gdbserver/snitchaser_patch.h>
#include <host/arch_replayer_helper.h>
#include <host/replay_log.h>
#include <host/syscall_replayer.h>
#include <common/replay/socketpair.h>

#include <asm_offsets.h>
#include <xasm/processor.h>

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>

#include <xasm/signal_numbers.h>
#include <xasm/marks.h>
/* target.h depends on server.h */
#include "server.h"
#include "target.h"

struct SN_info SN_info;

static struct pusha_regs * pusha_regs_addr;

static void
target_read_memory(uintptr_t memaddr, void * myaddr, size_t len)
{
	DEBUG(XGDBSERVER, "read memory: 0x%x %d\n", memaddr, len);
	assert(the_target->read_memory);
	assert(myaddr != NULL);
	int err = read_inferior_memory((CORE_ADDR)(uintptr_t)memaddr, myaddr, len);
	if (err != 0)
		THROW_FATAL(EXP_GDBSERVER_ERROR,
				"read inferior memory error: %d", err);
}

static void
target_write_memory(uintptr_t memaddr, void * myaddr, size_t len)
{
	DEBUG(XGDBSERVER, "write memory: 0x%x %d\n", memaddr, len);
	assert(the_target->write_memory);
	assert(myaddr != NULL);
	int err = write_inferior_memory((CORE_ADDR)(uintptr_t)memaddr, myaddr, len);
	if (err != 0)
		THROW_FATAL(EXP_GDBSERVER_ERROR,
				"write inferior memory error: %d", err);
}


static void
target_continue(void)
{
	int err;
	errno = 0;
	err = ptrace(PTRACE_CONT, SN_info.pid, 0, 0);
	ETHROW_FATAL(EXP_GDBSERVER_ERROR, "unable to continue");
}

static int
my_waitid(bool_t nowait, bool_t should_trap, int signo)
{
	int err;
	siginfo_t si;

	int flag = WEXITED | WSTOPPED;
	if (nowait)
		flag |= WNOWAIT;
	signal(SIGINT, SIG_IGN);
	errno = 0;
	err = waitid(P_PID, SN_info.pid, &si, flag);
	signal(SIGINT, SIG_DFL);

	ETHROW_FATAL(EXP_GDBSERVER_ERROR, "waitid failed with %d", err);

	SILENT(XGDBSERVER, "my_waitid: si.si_code=%d, si.si_status=0x%d\n",
			si.si_code, si.si_status);

	if (should_trap)
		CTHROW_FATAL(si.si_code == CLD_TRAPPED, EXP_GDBSERVER_ERROR,
				"waitid error: si.si_code=%d, not CLD_TRAPPED", si.si_code);
	if (signo != 0)
		CTHROW_FATAL(si.si_status == signo, EXP_GDBSERVER_ERROR,
				"waitid error: si.si_status=%d, not %d",
				si.si_status, signo);
	return si.si_status;
}

static void
wait_for_replayer_sync(void)
{
	my_waitid(FALSE, TRUE, GDBSERVER_NOTIFICATION);
#if 0
	int err;
	siginfo_t si;

	signal(SIGINT, SIG_IGN);
	errno = 0;
	err = waitid(P_PID, SN_info.pid, &si, WEXITED | WSTOPPED);
	signal(SIGINT, SIG_DFL);

	ETHROW_FATAL(EXP_GDBSERVER_ERROR, "waitid failed with %d", err);

	CTHROW_FATAL(si.si_code == CLD_TRAPPED, EXP_GDBSERVER_ERROR,
			"waitid error: si.si_code=%d", si.si_code);
	CTHROW_FATAL(si.si_status == GDBSERVER_NOTIFICATION, EXP_GDBSERVER_ERROR,
			"waitid error: si.si_status=%d", si.si_status);
#endif
}

static void
adjust_wait_result(void)
{
	uintptr_t ori_eip = ptrace_get_eip(SN_info.pid);
	ptrace_set_eip(SN_info.pid, (uintptr_t)SN_info.replay_nop);
	int ret = ptrace(PTRACE_SINGLESTEP, SN_info.pid, 0, 0);
	CTHROW_FATAL(ret == 0, EXP_GDBSERVER_ERROR,
			"PTRACE_SINGLESTEP failed, err=%d", ret);
	
	my_waitid(TRUE, TRUE, SIGTRAP);
	ptrace_set_eip(SN_info.pid, ori_eip);
}

void
SN_init(void)
{
	pusha_regs_addr = SN_info.stack_base + OFFSET_PUSHA_REGS;
}

void
SN_reset_state(void)
{
	/* first: retrieve registers from tls stack */
	struct pusha_regs regs;

	/* the esp in regs is adjusted by replayer */
	target_read_memory((uintptr_t)pusha_regs_addr, &regs, sizeof(regs));

	DEBUG(XGDBSERVER, "got registers:\n");
	DEBUG(XGDBSERVER, "\teax=0x%x\n", regs.eax);
	DEBUG(XGDBSERVER, "\tebx=0x%x\n", regs.ebx);
	DEBUG(XGDBSERVER, "\tecx=0x%x\n", regs.ecx);
	DEBUG(XGDBSERVER, "\tedx=0x%x\n", regs.edx);
	DEBUG(XGDBSERVER, "\tesi=0x%x\n", regs.esi);
	DEBUG(XGDBSERVER, "\tedi=0x%x\n", regs.edi);
	DEBUG(XGDBSERVER, "\tesp=0x%x\n", regs.esp);
	DEBUG(XGDBSERVER, "\tebp=0x%x\n", regs.ebp);
	DEBUG(XGDBSERVER, "\teflags=0x%x\n", regs.eflags);

	uintptr_t eip;
	target_read_memory((uintptr_t)(SN_info.stack_base + OFFSET_TARGET),
			&eip, sizeof(eip));
	DEBUG(XGDBSERVER, "\teip=0x%x\n", eip);

	/* restore registers */
	arch_restore_registers(SN_info.pid, &regs, eip);
}

static int
ptrace_single_step(bool_t is_branch,
		bool_t is_int3, bool_t is_ud, bool_t is_rdtsc,
		bool_t is_call, size_t call_sz, bool_t is_ret,
		uintptr_t pnext_inst,
		struct user_regs_struct * psaved_regs)
{
	assert(psaved_regs != NULL);

	TRACE(XGDBSERVER, "ptrace_singlestep: eip=0x%x,"
			" branch:%d, int3:%d, ud:%d, rdtsc:%d\n",
			(unsigned int)psaved_regs->eip,
			is_branch, is_int3, is_ud, is_rdtsc);

	ptrace_set_regset(SN_info.pid, psaved_regs);

	if (is_rdtsc) {
		int ret = ptrace(PTRACE_SINGLESTEP, SN_info.pid, 0, 0);
		my_waitid(TRUE, TRUE, SIGTRAP);

		/* read log:
		 * 4 bytes for rdtsc mark,
		 * 4 bytes for EAX,
		 * 4 bytes for EDX */
		uint32_t mark = read_u32_from_log();
		if (mark != RDTSC_MARK)
			THROW_FATAL(EXP_FILE_CORRUPTED, "inst is rdtsc but mark is 0x%x",
					mark);
		uint32_t eax = read_u32_from_log();
		uint32_t edx = read_u32_from_log();

		/* only reset eax and edx, psaved_regs contain eflags and eip */
		ptrace_set_eax(SN_info.pid, eax);
		ptrace_set_edx(SN_info.pid, edx);
		/* we use NOWAIT in waitid, gdbserver can wait again */
		return ret;
	}

	if ((!is_branch) || is_int3 || is_ud) {
		/* for int3:
		 * it is a breakpoint, and isn't inserted by us.
		 * normally single step
		 *
		 * for ud:
		 * log don't contain its record, normally single step
		 * */
		int ret = ptrace(PTRACE_SINGLESTEP, SN_info.pid, 0, 0);
		/* my_waitid, check whether SIGTRAP? */
		int status = my_waitid(TRUE, TRUE, 0);
		if (status != SIGTRAP) {
			WARNING(XGDBSERVER, "NEVER TESTED CODE!!!\n");
			WARNING(XGDBSERVER,
					"normal code generate unlogged signal %d at 0x%x\n",
					status, (uintptr_t)psaved_regs->eip);

			if (status == SIGINT)
				THROW_FATAL(EXP_GDBSERVER_TARGET_SIGNALED,
						"target signaled by sigint");

			WARNING(XGDBSERVER, "reset eip to 0x%x\n", pnext_inst);
			ptrace_set_eip(SN_info.pid, pnext_inst);

			adjust_wait_result();
			return 0;
		}
		return ret;
	}

	int ret = ptrace(PTRACE_SINGLESTEP, SN_info.pid, 0, 0);
	int status = my_waitid(TRUE, TRUE, 0);

	if (status != SIGTRAP) {
		WARNING(XGDBSERVER, "NEVER TESTED CODE!!!\n");
		WARNING(XGDBSERVER, "receive signal %d at 0x%x\n", status,
				(uintptr_t)psaved_regs->eip);
		if (status == SIGINT)
			THROW_FATAL(EXP_GDBSERVER_TARGET_SIGNALED,
					"target signaled by sigint");

		/* for call and ret, we should adjust esp */
		if (is_call) {
			ptrace_set_esp(SN_info.pid, psaved_regs->esp - 4);
			/* push return addr */
			uintptr_t ret_addr = psaved_regs->eip + call_sz;
			target_write_memory(psaved_regs->esp - 4, &ret_addr,
					sizeof(ret_addr));
		}
		if (is_ret)
			ptrace_set_esp(SN_info.pid, psaved_regs->esp + 4);

		ret = 0;
		adjust_wait_result();
	}

	uintptr_t eip = ptrace_get_eip(SN_info.pid);
	/* read from log */
	uintptr_t real_eip = read_ptr_from_log();

	TRACE(XGDBSERVER, "branch to 0x%x, should be 0x%x\n",
			eip, real_eip);

	if (eip != real_eip) {
		WARNING(XGDBSERVER, "branch inconsistent at 0x%x: "
				"should be 0x%x, not %x\n",
				(uintptr_t)psaved_regs->eip, real_eip, eip);
		ptrace_set_eip(SN_info.pid, real_eip);
	}
	return ret;
}

static int
SN_single_step(void);

static int
SN_cont(void)
{
	TRACE(XGDBSERVER, "ptrace_cont\n");

	/* when cont:
	 * 0. read ahead from log, if next mark is signal mark, unimplement
	 * 1. use interp helper to find the next branch;
	 * 2. helper returns the address of that instruction;
	 * 3. patch that inst: replace it by an int3;
	 * 4. wait;
	 * (4.1: whether eip is previous patched instruction?
	 *       if it is, normal case, turn to step 5;
	 *       if not: abnormal case, turn to step x0;)
	 * 5. unpatch code and reset eip;
	 * 6. if the original inst is an int3, ptrace cont then return;
	 * 7. if not, single step and wait;
	 * 8. goto step 0;
	 *
	 * x0: signal arise:
	 *     1. unpatch code;
	 *     2. if original inst is int3, goto int3, ptrace continue;
	 *     3. if the original inst is not int3, single step then goto 0;
	 * */

	for (;;) {
		/* readahead and check mark */
		uintptr_t ptr = readahead_log_ptr();
		if ((!IS_VALID_PTR(ptr)) && (ptr != SYSCALL_MARK)) {
			THROW_FATAL(EXP_UNIMPLEMENTED, "read from log, next mark is 0x%x",
					ptr);
			/* NOTICE: a signal may arise at another block! for example:
			 *
			 * (current eip):
			 * movl xx, xx
			 * jmp 1f
			 * nop
			 * 1: xxxx	(raise a signal)
			 * */
		}

		struct user_regs_struct saved_regs;
		ptrace_get_regset(SN_info.pid, &saved_regs);

		uintptr_t eip = saved_regs.eip;

		TRACE(XGDBSERVER, "new block begin at 0x%x\n", eip);

		/* find the next branch instruction */
		ptrace_set_eip(SN_info.pid, (uintptr_t)(SN_info.get_next_branch));
		target_continue();

		sock_send_ptr(eip);
		uintptr_t branch_start = sock_recv_ptr();
		wait_for_replayer_sync();

		if (!IS_VALID_PTR(branch_start))
			THROW_FATAL(EXP_GDBSERVER_ERROR, "interp report next branch at 0x%x",
					branch_start);

		TRACE(XGDBSERVER, "branch_start at 0x%x\n", branch_start);

		/* restore regs */
		ptrace_set_regset(SN_info.pid, &saved_regs);

		/* save and patch */
		uint8_t ori_inst;
		target_read_memory(branch_start, &ori_inst, sizeof(ori_inst));

		/* 0xcc is int3 */
		uint8_t new_inst = 0xcc;
		target_write_memory(branch_start, &new_inst, sizeof(new_inst));

		/* continue and wait */
		/* don't set NOWAIT! */
		target_continue();
		int signal = my_waitid(FALSE, TRUE, 0);

		/* target trapped */
		ptrace_get_regset(SN_info.pid, &saved_regs);
		uintptr_t new_eip = saved_regs.eip;
		TRACE(XGDBSERVER, "trapped at 0x%x\n", new_eip);

		/* unpatch code */
		target_write_memory(branch_start, &ori_inst, sizeof(ori_inst));

		if (new_eip != branch_start + 1) {
			/* may be signal arises */
			WARNING(XGDBSERVER, "NEVER TESTED CODE!!!\n");
			WARNING(XGDBSERVER,
					"target stopped at 0x%x, not 0x%x, signal: %d\n",
					new_eip, branch_start, signal);
			if (signal == SIGINT)
				THROW_FATAL(EXP_GDBSERVER_TARGET_SIGNALED,
						"target signaled by SIGINT");

			/* single step and wait */
			SN_single_step();
			my_waitid(FALSE, TRUE, SIGTRAP);

			/* again!! */
			continue;
		}

		/* target stopped at my breakpoint */
		/* reset eip! */
		ptrace_set_eip(SN_info.pid, branch_start);

		if (ori_inst == 0xcc)
			return ptrace(PTRACE_CONT, SN_info.pid, 0, 0);

		/* single step then continue */
		SN_single_step();
	}

	return 0;
}

/* 
 * return value: if signal arise, return signal number
 * in normal case, return 0
 */
static int
single_step_syscall(uintptr_t new_eip, struct user_regs_struct * psaved_regs)
{
	assert(psaved_regs != NULL);
	ptrace_set_regset(SN_info.pid, psaved_regs);

	TRACE(XGDBSERVER, "in single_step_syscall, eip=0x%lx, esp=0x%lx\n",
			psaved_regs->eip, psaved_regs->esp);

	uint32_t mark = read_u32_from_log();
	if (mark != SYSCALL_MARK)
		THROW_FATAL(EXP_FILE_CORRUPTED,
				"mark should be 0x%x but actually 0x%x",
				SYSCALL_MARK, mark);

	int nr = read_int_from_log();
	TRACE(XGDBSERVER, "this is syscall %d\n", nr);
	if (psaved_regs->eax != nr)
		THROW_FATAL(EXP_FILE_CORRUPTED,
				"should be syscall %d, not %d", nr, (int)psaved_regs->eax);

	/* check for no-signal-mark */
	mark = read_u32_from_log();
	if (mark != NO_SIGNAL_MARK)
		THROW_FATAL(EXP_UNIMPLEMENTED, "syscall is broken by signal, "
				"unimplemented");
	TRACE(XGDBSERVER, "NO_SIGNAL_MARK ok\n");
	
	/* redirect eip */
	ptrace_set_eip(SN_info.pid, (uintptr_t)(SN_info.syscall_helper));
	target_continue();

	/* see do_replay_syscall_helper in
	 * arch/x86/interp/syscalls/replay_syscalls.c */
	sock_send_int(nr);
	struct pusha_regs new_regs;
	read_log_full(&new_regs, sizeof(new_regs));
	sock_send(&new_regs, sizeof(new_regs));
	/* for debug use */
	sock_send_ptr(new_eip);

	syscall_read_cycle();

	wait_for_replayer_sync();

	/* restore registers */
	arch_restore_registers(SN_info.pid, &new_regs, new_eip);
	adjust_wait_result();

#if 0
	struct pusha_regs ori_regs, new_regs;
	read_log_full(&ori_regs, sizeof(ori_regs));
	TRACE(XGDBSERVER, "esp in ori_regs is 0x%x\n",
			ori_regs.esp);

	TRACE(XGDBSERVER, "single step syscall, next inst is 0x%x\n", new_eip);

	/* then check for no-signal-mark */
	mark = read_u32_from_log();
	if (mark != NO_SIGNAL_MARK)
		THROW_FATAL(EXP_UNIMPLEMENTED, "syscall is broken by signal, "
				"unimplemented");
	TRACE(XGDBSERVER, "NO_SIGNAL_MARK ok\n");

	/* redirect eip */
	ptrace_set_eip(SN_info.pid, (uintptr_t)(SN_info.syscall_helper));
	target_continue();

	/* see do_replay_syscall_helper in
	 * arch/x86/interp/syscalls/replay_syscalls.c */
	sock_send(&ori_regs, sizeof(ori_regs));

	/* begin syscall cycle */
	syscall_read_cycle();

	sock_recv(&new_regs, sizeof(new_regs));

	wait_for_replayer_sync();

	/* restore registers */
	arch_restore_registers(SN_info.pid, &new_regs, new_eip);
	adjust_wait_result();
#endif
	return 0;
}

static int
SN_single_step(void)
{

	struct user_regs_struct saved_regs;
	ptrace_get_regset(SN_info.pid, &saved_regs);

	/* fetch original eip */
	uintptr_t eip = saved_regs.eip;

	ptrace_set_eip(SN_info.pid, (uintptr_t)SN_info.is_branch_inst);
	target_continue();

	sock_send(&eip, sizeof(eip));

	bool_t res = sock_recv_bool();
	uintptr_t pnext_inst = sock_recv_ptr();

	if (!res) {
		assert(pnext_inst != 0);
		wait_for_replayer_sync();
		return ptrace_single_step(FALSE, FALSE,
				FALSE, FALSE, FALSE, 0, FALSE,
				pnext_inst,
				&saved_regs);
	}

	/* see compiler.c */
	bool_t is_int3 = sock_recv_bool();
	bool_t is_ud = sock_recv_bool();
	bool_t is_rdtsc = sock_recv_bool();

	bool_t is_int80 = sock_recv_bool();
	bool_t is_vdso_syscall = sock_recv_bool();
	bool_t is_call = sock_recv_bool();
	size_t call_sz = 0;
	if (is_call)
		call_sz = (size_t)(sock_recv_u32());
	bool_t is_ret = sock_recv_bool();

	wait_for_replayer_sync();

	if (is_int80 || is_vdso_syscall) {
		uintptr_t new_eip;
		/* 2 and 7 are the size of the syscall instruction */
		if (is_int80) {
			new_eip = eip + 2;
		} else {
			new_eip = eip + 7;
		}

		int signum = single_step_syscall(new_eip, &saved_regs);
		if (signum != 0)
			THROW_FATAL(EXP_UNIMPLEMENTED, "syscall is broken by signal, "
					"doesn't support");
		return 0;
	}

	return ptrace_single_step(TRUE, is_int3, is_ud,
			is_rdtsc, is_call, call_sz,
			is_ret, pnext_inst, &saved_regs);
}

int
SN_ptrace_cont(enum __ptrace_request req, pid_t pid,
		uintptr_t addr, uintptr_t data)
{
	assert((req == PTRACE_CONT) || (req == PTRACE_SINGLESTEP));
	if (pid != SN_info.pid)
		return ptrace(req, pid, addr, data);

	if (req == PTRACE_SINGLESTEP)
		return SN_single_step();
	else
		return SN_cont();
}

// vim:ts=4:sw=4

