/* 
 * snitchaser_patch.h
 * by WN @ Jnu. 15, 2010
 *
 * snitchaser patch on gdbserver
 *
 */

#ifndef SNITCHASER_PATCH_H
#define SNITCHASER_PATCH_H

#include <common/defs.h>
#include <xasm/processor.h>
#include <stdint.h>
#include <unistd.h>	/* pid_t */
#include <sys/ptrace.h>

struct SN_info {
	pid_t ori_pid;
	pid_t ori_tid;
	pid_t pid;
	int ori_tnr;
	void * stack_base;
	/* function pointers */
	void * is_branch_inst;
	void * replay_nop;
	void * syscall_helper;
	void * get_next_branch;
};

extern struct SN_info SN_info;

extern void
SN_init(void);

extern void
SN_reset_state(void);

extern int
SN_ptrace_cont(enum __ptrace_request req, pid_t pid,
		uintptr_t addr, uintptr_t data);

/* defined in server.c */
extern int
gdbserver_main(int argc ATTR_UNUSED, char *argv[]);

#endif

// vim:ts=4:sw=4

