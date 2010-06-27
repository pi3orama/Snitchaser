/* 
 * snitchaser_patch.c
 * by WN @ Jun. 16, 2010
 */

#include <common/debug.h>
#include <host/exception.h>

#include <host/gdbserver/snitchaser_patch.h>
#include <host/arch_replayer_helper.h>
#include <host/replay_log.h>
#include <common/replay/socketpair.h>

#include <asm_offsets.h>
#include <xasm/processor.h>

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>

#include <xasm/signal_numbers.h>
#include <xasm/logger_marks.h>
/* target.h depends on server.h */
#include "server.h"
#include "target.h"

struct SN_info SN_info;

static struct pusha_regs * pusha_regs_addr;

static void
target_read_memory(void * memaddr, void * myaddr, size_t len)
{
	TRACE(XGDBSERVER, "read memory: %p %d\n", memaddr, len);
	assert(the_target->read_memory);
	assert(myaddr != NULL);
	int err = read_inferior_memory((CORE_ADDR)(uintptr_t)memaddr, myaddr, len);
	if (err != 0)
		THROW_FATAL(EXP_GDBSERVER_ERROR,
				"read inferior memory error: %d", err);
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
SN_reset_registers(void)
{
	/* first: retrieve registers from tls stack */
	struct pusha_regs regs;

	/* the esp in regs is adjusted by replayer */
	target_read_memory((void*)pusha_regs_addr, &regs, sizeof(regs));

	TRACE(XGDBSERVER, "got registers:\n");
	TRACE(XGDBSERVER, "\teax=0x%x\n", regs.eax);
	TRACE(XGDBSERVER, "\tebx=0x%x\n", regs.ebx);
	TRACE(XGDBSERVER, "\tecx=0x%x\n", regs.ecx);
	TRACE(XGDBSERVER, "\tedx=0x%x\n", regs.edx);
	TRACE(XGDBSERVER, "\tesi=0x%x\n", regs.esi);
	TRACE(XGDBSERVER, "\tedi=0x%x\n", regs.edi);
	TRACE(XGDBSERVER, "\tesp=0x%x\n", regs.esp);
	TRACE(XGDBSERVER, "\tebp=0x%x\n", regs.ebp);

	void * eip;
	target_read_memory(SN_info.stack_base + OFFSET_TARGET,
			&eip, sizeof(eip));
	TRACE(XGDBSERVER, "\teip=%p\n", eip);

	/* restore registers */
	arch_restore_registers(SN_info.pid, &regs, eip);
}

static int
SN_cont(struct user_regs_struct * saved_regs)
{
	TRACE(XGDBSERVER, "ptrace_cont\n");
	THROW_FATAL(EXP_UNIMPLEMENTED, "ptrace_cont is not implemented");
	return 0;
}

static int
ptrace_single_step(bool_t is_branch,
		bool_t is_int3, bool_t is_ud, bool_t is_rdtsc,
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
			WARNING(XGDBSERVER, "NEVER TESTED!!!\n");
			WARNING(XGDBSERVER, "normal code generate unlogged signal %d\n",
					status);
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
#error ...
	}
	return ret;

	THROW_FATAL(EXP_UNIMPLEMENTED, "normal case is not implemented");
}

static int
SN_single_step(struct user_regs_struct * psaved_regs)
{
	/* fetch original eip */
	uintptr_t eip = ptrace_get_eip(SN_info.pid);

	ptrace_set_eip(SN_info.pid, (uintptr_t)SN_info.is_branch_inst);
	target_continue();

	sock_send(&eip, sizeof(eip));

	bool_t res = sock_recv_bool();
	uintptr_t pnext_inst = sock_recv_ptr();

	if (!res) {
		assert(pnext_inst != 0);
		wait_for_replayer_sync();
		return ptrace_single_step(FALSE, FALSE, FALSE, FALSE, pnext_inst,
				psaved_regs);
	}

	/* see compiler.c */
	bool_t is_int3 = sock_recv_bool();
	bool_t is_ud = sock_recv_bool();
	bool_t is_rdtsc = sock_recv_bool();

	bool_t is_int80 = sock_recv_bool();
	bool_t is_vdso_syscall = sock_recv_bool();

	wait_for_replayer_sync();

	if (is_int80 || is_vdso_syscall)
		THROW_FATAL(EXP_UNIMPLEMENTED, "unable to process syscall");

	return ptrace_single_step(TRUE, is_int3, is_ud,
			is_rdtsc, pnext_inst, psaved_regs);
}

int
SN_ptrace_cont(enum __ptrace_request req, pid_t pid,
		uintptr_t addr, uintptr_t data)
{
	assert((req == PTRACE_CONT) || (req == PTRACE_SINGLESTEP));
	if (pid != SN_info.pid)
		return ptrace(req, pid, addr, data);

	struct user_regs_struct saved_urs;
	ptrace_get_regset(SN_info.pid, &saved_urs);

	if (req == PTRACE_SINGLESTEP)
		return SN_single_step(&saved_urs);
	else
		return SN_cont(&saved_urs);

#if 0
	/* get current eip, put it into OFFSET_TARGET, then redirect
	 * code into SN_info.patch_block_func */

	uintptr_t ptr = read_ptr_from_log();
	if (ptr < 0x1000) {
		/* this is system call */
		THROW_FATAL(EXP_UNIMPLEMENTED, "system call %d\n", ptr);
	}

	if (ptr > 0xc0000000) {
		/* this is mark */
		THROW_FATAL(EXP_UNIMPLEMENTED, "mark 0x%x\n", ptr);
	}

	TRACE(XGDBSERVER, "next branch target is 0x%x\n", ptr);

	redirect_

#endif
	THROW_FATAL(EXP_UNIMPLEMENTED, "xxxxx");
	return ptrace(req, pid, addr, data);
}

// vim:ts=4:sw=4

