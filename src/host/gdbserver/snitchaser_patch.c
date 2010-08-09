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
#include <xasm/unistd_32.h>
/* target.h depends on server.h */
#include "server.h"
#include "target.h"

struct SN_info SN_info;

static struct pusha_regs * pusha_regs_addr;

static void
target_read_memory(uintptr_t memaddr, void * myaddr, size_t len)
{
	SILENT(XGDBSERVER, "read memory: 0x%x %d\n", memaddr, len);
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
	SILENT(XGDBSERVER, "write memory: 0x%x %d\n", memaddr, len);
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
	if (signo != 0) {
		uintptr_t eip = ptrace_get_eip(SN_info.pid);
		CTHROW_FATAL(si.si_status == signo, EXP_GDBSERVER_ERROR,
				"waitid error at 0x%x: si.si_status=%d, not %d",
				eip, si.si_status, signo);
	}
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
adjust_wait_result(bool_t trap, bool_t keep_eip)
{
	uintptr_t ori_eip = ptrace_get_eip(SN_info.pid);
	if (!trap)
		ptrace_set_eip(SN_info.pid, (uintptr_t)SN_info.replay_nop);
	else
		ptrace_set_eip(SN_info.pid, (uintptr_t)SN_info.replay_trap);

	int ret = ptrace(PTRACE_SINGLESTEP, SN_info.pid, 0, 0);
	CTHROW_FATAL(ret == 0, EXP_GDBSERVER_ERROR,
			"PTRACE_SINGLESTEP failed, err=%d", ret);
	
	my_waitid(TRUE, TRUE, SIGTRAP);
	if (keep_eip)
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

			adjust_wait_result(FALSE, TRUE);
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
		adjust_wait_result(FALSE, TRUE);
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
SN_single_step(bool_t * cont_stop);

static int
cont_to_signal(void)
{
	uintptr_t mark = readahead_log_ptr();
	assert(mark == SIGNAL_MARK);

	struct {
		uint32_t signal_mark;
		uintptr_t addr;
		uint32_t terminal_mark;
		int signum;
	} marks;

	readahead_log(&marks, sizeof(marks));

	bool_t cont_stop = FALSE;

	for (;;) {
		uintptr_t curr_eip = ptrace_get_eip(SN_info.pid);
		TRACE(XGDBSERVER, "single step to 0x%x (current: 0x%x)\n",
				marks.addr, curr_eip);
		assert(curr_eip <= marks.addr);
		if (curr_eip != marks.addr) {
			SN_single_step(&cont_stop);
			if (cont_stop)
				return 0;
		} else {
			break;
		}
	}
	SN_single_step(NULL);
	return 0;
}

static void
setup_sighandler(void)
{
	uint32_t m = readahead_log_ptr();
	if (m != SIGNAL_MARK_2)
		THROW_FATAL(EXP_LOG_CORRUPTED, "no SIGNAL_MARK_2 for signal handler");
	/* see arch_signal.c -- signal_handler */
	struct {
		uint32_t signal_mark_2;
		struct pusha_regs regs;
		uint32_t frame_size;
		uintptr_t handler;
	} mark;
	read_log(&mark, sizeof(mark));

	uintptr_t esp = mark.regs.esp;
	void * frame = alloca(mark.frame_size);
	assert(frame != NULL);
	read_log(frame, mark.frame_size);

	/* poke memory */
	target_write_memory(esp, frame, mark.frame_size);

	/* reset registers */
	arch_restore_registers(SN_info.pid, &(mark.regs), mark.handler);

	/* set sigtrap wait state */
	adjust_wait_result(TRUE, TRUE);

	VERBOSE(XGDBSERVER, "transfer to signal handler 0x%x\n", mark.handler);
}

static int
sighandler_return(void)
{
	/* consume SIGRETURN_MARK */
	uint32_t mark = read_u32_from_log();
	assert(mark == SIGRETURN_MARK);

	VERBOSE(XGDBSERVER, "signal handler return\n");
	struct user_regs_struct regs;
	ptrace_get_regset(SN_info.pid, &regs);

	if ((regs.eip != (intptr_t)SN_info.arch_wrapper_sigreturn) &&
			(regs.eip != (intptr_t)SN_info.arch_wrapper_rt_sigreturn))
		THROW_FATAL(EXP_LOG_CORRUPTED,
				"SIGRETURN_MARK appear but eip incorrect:"
				" 0x%x, %p, %p\n", (uint32_t)regs.eip,
				SN_info.arch_wrapper_sigreturn,
				SN_info.arch_wrapper_rt_sigreturn);

	if (regs.eip == (intptr_t)SN_info.arch_wrapper_sigreturn) {
		regs.esp += 4;
		regs.eax = __NR_sigreturn;
	} else {
		regs.eax = __NR_rt_sigreturn;
	}
	regs.eip = (intptr_t)SN_info.replay_int80;

	ptrace_set_regset(SN_info.pid, &regs);
	int err = ptrace(PTRACE_SINGLESTEP, SN_info.pid, 0, 0);
	CTHROW_FATAL(err == 0, EXP_GDBSERVER_ERROR,
			"PTRACE_SINGLESTEP failed, err=%d", err);
	my_waitid(TRUE, TRUE, 0);
	adjust_wait_result(TRUE, TRUE);
	return 0;
}

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
		TRACE(XGDBSERVER, "ptr = 0x%x\n", ptr);
		/* 0xffffffff means the end of log */
		if ((!IS_VALID_PTR(ptr)) &&
				(ptr != SYSCALL_MARK) &&
				(ptr != 0xffffffff))
		{
			/* NOTICE: a signal may arise at another block! for example:
			 *
			 * (current eip):
			 * movl xx, xx
			 * jmp 1f
			 * nop
			 * 1: xxxx	(raise a signal).
			 *
			 * However, we doesn't allow this situation. when each code block
			 * begin, the compiled code will set tpd->current_block before
			 * anything.
			 * */

			if (ptr == SIGNAL_MARK) {
				DEBUG(XGDBSERVER,
						"see signal mark, "
						"single step to signaled instruction\n");
				return cont_to_signal();
			} else if (ptr == SIGNAL_MARK_2) {
				DEBUG(XGDBSERVER, "move to signal handler\n");
				setup_sighandler();
				return 0;
			} else if (ptr == SIGRETURN_MARK) {
				return sighandler_return();
			} else if (ptr == RDTSC_MARK) {
				/* do nothing */
			} else {
				THROW_FATAL(EXP_UNIMPLEMENTED,
						"read from log, next mark is 0x%x",	ptr);
			}
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
			SN_single_step(NULL);
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
		bool_t cont_stop = FALSE;
		SN_single_step(&cont_stop);
		if (cont_stop)
			return 0;
	}

	return 0;
}



static int
signal_inst(uintptr_t eip, uint32_t terminal_mark, int signum);
/* 
 * return value: if signal arise, return signal number
 * in normal case, return 0
 */
static int
single_step_syscall(uintptr_t eip, uintptr_t new_eip,
		struct user_regs_struct * psaved_regs)
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
#if 0
	/* if the system call is broken by signal, after sigreturn,
	 * eax will be the return value of the system call, not
	 * the 'nr'. */
	if (psaved_regs->eax != nr)
		THROW_FATAL(EXP_FILE_CORRUPTED,
				"should be syscall %d, not %d", nr, (int)psaved_regs->eax);
#endif

	/* check for no-signal-mark */
	mark = readahead_log_ptr();
	if (mark != NO_SIGNAL_MARK) {
		if (mark != SIGNAL_MARK) {
			if (mark != 0xffffffff)
				THROW_FATAL(EXP_LOG_CORRUPTED, "mark 0x%x follows syscall mark",
						mark);
			else
				THROW(EXP_LOG_END, "log is end after syscall mark %d", nr);
		}
		struct {
			uint32_t mark;
			uintptr_t addr;
			uint32_t terminal_mark;
			int signal;
		} marks;
		readahead_log(&marks, sizeof(marks));
		VERBOSE(SIGNAL, "syscall %d (0x%x) broken by signal %d\n", nr, eip,
				marks.signal);
		TRACE(SIGNAL, "addr=0x%x, terminal_mark=0x%x, signal=%d\n",
				marks.addr, marks.terminal_mark, marks.signal);
		signal_inst(eip, marks.terminal_mark, marks.signal);
		return marks.signal;
	}

	TRACE(XGDBSERVER, "NO_SIGNAL_MARK ok\n");

	/* consume mark */
	mark = read_u32_from_log();
	
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
	adjust_wait_result(FALSE, TRUE);

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
signal_inst(uintptr_t eip, uint32_t terminal_mark, int signum)
{
	/* 
	 * the order of signal marks:
	 * 	SIGNAL_MARK,
	 * 	EIP,
	 * 	TERMINAL_MARK (whether process is terminated or move
	 * 					to signal handler)
	 *  SIGNUM,
	 *
	 *  if process is terminated, don't append anything to log and replayer
	 *  will end automatically.
	 *
	 *  if we use signal handler, following marks should be:
	 *  SIGNAL_MARK_2, registers, FRAMESIZE, SIGFRAME, sighandler's eip
	 *
	 * SIGNAL_MARK_2 is important. when gdb use ptrace continue or
	 * ptrace single step, snitchaser_patch can readahead and find this mark, it
	 * can then form a signal frame and direct target process to signal handler.
	 *  */

	struct {
		uint32_t signal_mark;
		uintptr_t addr;
		uint32_t terminal_mark;
		int signum;
	} marks;

	/* consume the marks */
	read_log(&marks, sizeof(marks));
	assert((marks.terminal_mark == terminal_mark) &&
			(marks.signum == signum));

	if (marks.addr != eip) {
		WARNING(XGDBSERVER, "addrs in mark: 0x%x; current eip is 0x%x\n",
				marks.addr, eip);
	}

	/* generate a SIGTRAP then return 0 */
	/* don't keep eip */
	adjust_wait_result(TRUE, TRUE);

	return 0;
}

static int
SN_single_step(bool_t * cont_stop)
{

	if (cont_stop != NULL)
		*cont_stop = FALSE;

	struct user_regs_struct saved_regs;
	ptrace_get_regset(SN_info.pid, &saved_regs);

	/* fetch original eip */
	uintptr_t eip = saved_regs.eip;

	/* readahead log, whether this instruction is
	 * interrupted by a signal? */
	/* FIXME */
	uint32_t mark = readahead_log_ptr();
	if (mark == SIGNAL_MARK) {
		struct {
			uint32_t signal_mark;
			uintptr_t addr;
			uint32_t terminal_mark;
			int signum;
		} marks;
		readahead_log(&marks, sizeof(marks));
		if (marks.addr == eip) {
			VERBOSE(XGDBSERVER, "inst 0x%x interrupted by signal %d\n",
					eip, marks.signum);
			*cont_stop = TRUE;
			return signal_inst(eip, marks.terminal_mark, marks.signum);
		}
	} else if (mark == SIGNAL_MARK_2) {
		setup_sighandler();
		ptrace_get_regset(SN_info.pid, &saved_regs);
		eip = saved_regs.eip;

		if (cont_stop != NULL)
			*cont_stop = TRUE;
		return 0;
	} else if (mark == SIGRETURN_MARK) {
		sighandler_return();
		if (cont_stop != NULL)
			*cont_stop = TRUE;
		return 0;
	}

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

		int signum = single_step_syscall(eip, new_eip, &saved_regs);
		if (signum != 0) {
			if (cont_stop != NULL)
				*cont_stop = TRUE;
		}
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
		return SN_single_step(NULL);
	else
		return SN_cont();
}

// vim:ts=4:sw=4

