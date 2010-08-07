/* 
 * arch_signal.c
 * by WN @ Jul. 07, 2010
 */


#include <interp/arch_signal.h>
#include <common/debug.h>

#include <xasm/syscall.h>
#include <xasm/signal_numbers.h>
#include <xasm/tls.h>
#include <xasm/string.h>
#include <xasm/marks.h>
#include <xasm/processor.h>
#include <xasm/utils.h>
#include <interp/mm.h>
#include <interp/logger.h>

k_sigset_t
arch_replay_mask_signals(void)
{
	k_sigset_t all_mask = {
		.sig = {0, 0}};
	k_sigset_t old_mask = {
		.sig = {0, 0}};

	sigaddset(&all_mask, GDBSERVER_NOTIFICATION);
	sigaddset(&all_mask, SIGINT);
	sigaddset(&all_mask, SIGSEGV);
	sigaddset(&all_mask, SIGFPE);
	sigaddset(&all_mask, SIGILL);

	all_mask.sig[0] = ~all_mask.sig[0];
	all_mask.sig[1] = ~all_mask.sig[1];

	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask,
			4, SIG_BLOCK, &all_mask, &old_mask,
			sizeof(all_mask));
	assert(err == 0);
	return old_mask;
}

k_sigset_t
arch_set_sigmask(k_sigset_t mask)
{
	k_sigset_t old_set;
	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask,
			4, SIG_SETMASK, &mask, &old_set,
			sizeof(mask));
	assert(err == 0);
	return old_set;
}

void
arch_init_signal(void)
{
	struct thread_private_data * tpd = get_tpd();
	uint32_t mask[2] = RECORD_PROCMASK;

	memcpy(tpd->block_sigmask, mask, sizeof(mask));
	int err = INTERNAL_SYSCALL_int80(rt_sigprocmask, 4,
			SIG_SETMASK, mask, &(tpd->unblock_sigmask),
			sizeof(k_sigset_t));
	assert(err == 0);

	for (int i = 1; i <= 64; i++) {

		if ((i == SIGKILL) || (i == SIGSTOP))
			continue;

		int err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
				i, NULL, &(tpd->sigactions[i - 1]),
				sizeof(k_sigset_t));
		assert(err == 0);

		/* install new signal handler for it according to
		 * SA_INFO flag */
		struct k_sigaction * pa = &(tpd->sigactions[i - 1]);
		struct k_sigaction new_action;

		if (pa->sa_flags & SA_RESTORER)
			FATAL(SIGNAL, "doesn't support target reset restorer\n");
		if (pa->sa_flags & SA_ONSTACK)
			FATAL(SIGNAL, "doesn't support signal stack\n");

		new_action.sa_flags = pa->sa_flags | SA_RESTORER;
		if (pa->sa_flags & SA_SIGINFO) {
			new_action.sa_handler = arch_wrapper_rt_sighandler;
			new_action.sa_restorer = arch_wrapper_rt_sigreturn;
		} else {
			new_action.sa_handler = arch_wrapper_sighandler;
			new_action.sa_restorer = arch_wrapper_sigreturn;
		}
		memcpy(&new_action.sa_mask, mask, sizeof(new_action.sa_mask));

		err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
				i, &new_action, NULL, sizeof(k_sigset_t));
		assert(err == 0);
	}

}

static void
signal_terminate(int num, struct thread_private_data * tpd, void * addr)
{
	WARNING(SIGNAL, "terminated by signaled %d\n", num);

	/* see the code of flush logger, we must write this mark by ONCE
	 * to prevent potential log flush */
	struct {
		uint32_t signal_mark;
		void * addr;
		uint32_t terminal_mark;
		int signum;
	} mark = {
		SIGNAL_MARK, addr, SIGNAL_TERMINATE, num
	};
	append_buffer(&mark, sizeof(mark));
	flush_logger();

	/* we needn't clean tls and code cache because all threads
	 * will be killed by this signal. */

	/* raise the signal again to kill myself: */
	/* reinstall sigactions */
	struct k_sigaction act;
	/* DFL action should be 'kill'. */
	/* those signal never terminate process */
	if ((num == SIGSTOP) ||
			(num == SIGTSTP) ||
			(num == SIGTTIN) ||
			(num == SIGTTOU) ||
			(num == SIGURG) ||
			(num == SIGWINCH) ||
			(num == SIGCHLD))
		FATAL(SIGNAL, "wrong signal %d terminates process\n", num);

	memset(&act, '\0', sizeof(act));
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;

	int err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
			num, &act, NULL, sizeof(k_sigset_t));
	assert(err == 0);

	/* block all signals */
	k_sigset_t set;
	memset(&set, (char)0x0, sizeof(set));

	/* unblock this signal */
	k_sigaddset(&set, num);

	err = INTERNAL_SYSCALL_int80(rt_sigprocmask, 4,
			SIG_UNBLOCK, &set, NULL, sizeof(set));
	assert(err == 0);

	/* raise the signal again */
	WARNING(SIGNAL, "will kill the whole thread group...\n");

	err = INTERNAL_SYSCALL_int80(kill, 2, tpd->pid, num);
	FATAL(SIGNAL, "We shouldn't get here!!!\n");
	INTERNAL_SYSCALL_int80(exit, 1, -1);
	/* never return */
}

static void
signal_stop(int num, struct thread_private_data * tpd)
{
	WARNING(SIGNAL, "stopped by signal %d\n", num);
	/* kill itself by an SIGSTOP: */
	int err = INTERNAL_SYSCALL_int80(kill, 2, tpd->pid, SIGSTOP);
	assert(err == 0);
}

/* peip is the address of ip field in frame  */
static void
signal_handler(int num, struct thread_private_data * tpd,
		void * addr, struct k_sigaction * act, void * frame,
		size_t frame_sz, struct pusha_regs * regs,
		void * retcode, struct sigcontext * psc)
{
	DEBUG(SIGNAL, "entering signal handler\n");
	DEBUG(SIGNAL, "ret addr in frame: %p\n", *(void**)frame);
	/* write mark to log */
	struct {
		uint32_t signal_mark;
		void * addr;
		uint32_t terminal_mark;
		int signum;
	} mark = {
		SIGNAL_MARK, addr, SIGNAL_HANDLER, num,
	};
	append_buffer(&mark, sizeof(mark));

	/* see comments in snitchaser_patch.c */
	struct {
		uint32_t signal_mark_2;
		struct pusha_regs regs;
		uint32_t frame_size;
		uintptr_t handler;
	} mark2;
	mark2.signal_mark_2 = SIGNAL_MARK_2;
	mark2.regs = *regs;
	mark2.regs.esp = (uintptr_t)(tpd->old_stack_top);
	mark2.frame_size = frame_sz;
	mark2.handler = (uintptr_t)(act->sa_handler);

#if 0
	/* don't append buffer here,
	 * write a patched frame into log */
	append_buffer(&mark2, sizeof(mark2));
	append_buffer(frame, frame_sz);
	flush_logger();
#endif

	/* for signal handler: we MUST save frame's eip to let sigreturn return to
	 * correct address when recording.  we also MUST save original eip to
	 * enable replayer to resume at correct address. */
	uintptr_t * backup_space = retcode;
	backup_space[0] = psc->ip;
	backup_space[1] = (uintptr_t)(tpd->target);

	/* MUST SAVE tpd->target for sigreturn */
	tpd->target = act->sa_handler;
	psc->ip = (uintptr_t)(addr);

	/* for system call */
	if (tpd->current_syscall_nr != -1) {
		TRACE(SIGNAL, "system call %d is broken by signal %d\n",
				tpd->current_syscall_nr, num);
		assert(tpd->current_syscall_nr < 0xffff);
		psc->__csh = tpd->current_syscall_nr;
	} else {
		psc->__csh = 0xffff;
	}

	/* record syscall info and patched ip info log */
	append_buffer(&mark2, sizeof(mark2));
	append_buffer(frame, frame_sz);
	flush_logger();

	/* for sigprocmask: we save current unblock mask in sigframe
	 * and replace unblock sigmask using tpd->sigactions data */
	psc->__gsh = tpd->unblock_sigmask[0] & 0xffff;
	psc->__fsh = tpd->unblock_sigmask[0] >> 16;
	psc->__esh = tpd->unblock_sigmask[1] & 0xffff;
	psc->__dsh = tpd->unblock_sigmask[1] >> 16;

	/* we MUST save current_block: if not, when signal raise right
	 * after sigreturn, current_block will be the last block of signal
	 * handler, and signal raise at wrong address. */
	/* ss, es and ds are same */
	psc->__ssh = ((uintptr_t)tpd->code_cache.current_block) & 0xffff;
	psc->ss = ((uintptr_t)(tpd->code_cache.current_block) >> 16);

	tpd->unblock_sigmask[0] =
		tpd->sigactions[num - 1].sa_mask.sig[0];
	tpd->unblock_sigmask[1] =
		tpd->sigactions[num - 1].sa_mask.sig[1];

	/* block current signal */
	int _num = num - 1;
	tpd->unblock_sigmask[_num / 64] |= (1UL << (_num % 64));

}

/* if return 1, sigreturn (or rt_sigreturn) (ignore) */
/* if return 2, terminate */
static int
common_wrapper_sighandler(int num, void * frame, size_t frame_sz,
		struct thread_private_data * tpd, void * ori_addr,
		struct pusha_regs * regs, void * retcode,
		struct sigcontext * psc)
{
	/* check whether to terminate */
	struct k_sigaction * act = &(tpd->sigactions[num - 1]);

	if (act->sa_handler == SIG_IGN) {
		/* ignore actions:  */
		if ((num == 32) || (num == 33))
			signal_terminate(num, tpd, ori_addr);
		/* else: sigreturn */
		return 1;
	} else if (act->sa_handler == SIG_DFL) {
		if ((num == SIGSTOP) ||
				(num == SIGTSTP) ||
				(num == SIGTTIN) ||
				(num == SIGTTOU)) {
			signal_stop(num, tpd);
			return 1;
		} else if ((num == SIGURG) ||
				(num == SIGWINCH) ||
				(num == SIGCHLD) ||
				(num == SIGCONT)) {
			return 1;
		} else {
			signal_terminate(num, tpd, ori_addr);
		}
	} else {
		if (!tpd->no_record_signals) {
			signal_handler(num, tpd, ori_addr, act,
					frame, frame_sz, regs, retcode, psc);
		} else {
			/* see arch_signal.S, 3 means to goto tpd->target */
			tpd->target = act->sa_handler;
			return 3;
		}
	}
	return 0;
}

static void *
get_ori_address(struct code_block_t * blk, void * _addr)
{
	uintptr_t addr = (uintptr_t)(_addr);
	uintptr_t block_start = (uintptr_t)(blk->entry);
	/* 11 byte is movl $0xffffffff, %fs:OFFSET_CODE_CACHE_CURRENT_BLOCK,
	 * see branch_template.S */
	uintptr_t compiled_code_start = (uintptr_t)(blk->__code) + 11;
	uintptr_t compiled_code_end = (uintptr_t)(blk->ori_code_end);

	if ((addr <= compiled_code_end) && (addr >= compiled_code_start))
		return (void*)(block_start + (_addr - compiled_code_start));
	return (void*)(block_start + (compiled_code_end - compiled_code_start));
}

int
do_arch_wrapper_sighandler(struct pusha_regs * regs)
{
	struct thread_private_data * tpd = get_tpd();
	struct sigframe * frame = tpd->old_stack_top;
	void * eip = (void*)(frame->sc.ip);
	void * ori_addr = get_ori_address(tpd->code_cache.current_block,
			(void *)eip);
	DEBUG(SIGNAL, "frame at %p\n", frame);
	return common_wrapper_sighandler(frame->sig, frame,
			sizeof(*frame), tpd, ori_addr, regs,
			frame->retcode,
			&(frame->sc));
}

int
do_arch_wrapper_rt_sighandler(struct pusha_regs * regs)
{
	struct thread_private_data * tpd = get_tpd();
	struct rt_sigframe * rt_frame = tpd->old_stack_top;
	void * eip = (void*)rt_frame->uc.uc_mcontext.ip;
	void * ori_addr = get_ori_address(tpd->code_cache.current_block,
			(void *)eip);
	TRACE(SIGNAL, "rt_frame at %p\n", rt_frame);
	return common_wrapper_sighandler(rt_frame->sig, rt_frame,
			sizeof(*rt_frame), tpd, ori_addr, regs,
			rt_frame->retcode,
			&(rt_frame->uc.uc_mcontext));
}

static void
common_wrapper_sigreturn(struct thread_private_data * tpd,
		void * retcode, struct sigcontext * psc)
{
	struct {
		uint32_t mark;
	} sigreturn_mark = {
		SIGRETURN_MARK,
	};

	append_buffer(&sigreturn_mark, sizeof(sigreturn_mark));

	uintptr_t * backup_space = retcode;
	psc->ip = backup_space[0];
	tpd->target = (void*)(backup_space[1]);

	TRACE(SIGNAL, "return to ip 0x%lx\n", psc->ip);
	if (psc->__csh != 0xffff) {
		/* resume system call */
		TRACE(SIGNAL, "resume system call %d\n", psc->__csh);
		tpd->current_syscall_nr = psc->__csh;
		struct {
			uint32_t mark;
			uint32_t nr;
		} syscall_mark2 = {
			SYSCALL_MARK, tpd->current_syscall_nr,
		};
		append_buffer(&syscall_mark2, sizeof(syscall_mark2));
	} else {
		tpd->current_syscall_nr = -1;
	}

	/* restore sigprocmask */
	tpd->unblock_sigmask[0] = (psc->__fsh << 16) | (psc->__gsh);
	tpd->unblock_sigmask[1] = (psc->__dsh << 16) | (psc->__esh);

	/* restore current block */
	tpd->code_cache.current_block = (void*)((psc->ss << 16) | (psc->__ssh));
	psc->ss = psc->ds;
}

void
do_arch_wrapper_sigreturn(void)
{
	struct thread_private_data * tpd = get_tpd();
	struct sigframe * frame = (void*)(tpd->old_stack_top - 4);
	common_wrapper_sigreturn(tpd, frame->retcode, &(frame->sc));
}

void
do_arch_wrapper_rt_sigreturn(void)
{
	struct thread_private_data * tpd = get_tpd();
	struct rt_sigframe * rt_frame = (void*)(tpd->old_stack_top - 4);
	common_wrapper_sigreturn(tpd, rt_frame->retcode,
			&(rt_frame->uc.uc_mcontext));
}

void
arch_restore_signal(void)
{
	struct thread_private_data * tpd = get_tpd();
	int err;

	/* restore all signal handler */
	for (int i = 1; i <= 64; i++) {
		if ((i == SIGKILL) || (i == SIGSTOP))
			continue;
		err = INTERNAL_SYSCALL_int80(rt_sigaction, 4,
				i, &(tpd->sigactions[i - 1]), NULL,
				sizeof(k_sigset_t));
		assert(err == 0);
	}

	/* finally, reset sigprocmask */
	err = INTERNAL_SYSCALL_int80(rt_sigprocmask, 4,
			SIG_SETMASK, &(tpd->unblock_sigmask), NULL,
			sizeof(k_sigset_t));
	assert(err == 0);
}

// vim:ts=4:sw=4

