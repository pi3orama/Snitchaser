/* 
 * ptrace.h
 * by WN @ Apr. 19, 2010
 */

#ifndef __SNITCHASER_PTRACE_H
#define __SNITCHASER_PTRACE_H

#include <sys/cdefs.h>
#include <sys/user.h>
#include <stdint.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <common/defs.h>


/* new_proc_prepare is a callback func, the newly created process
 * will call it immediately after fork() */
extern pid_t
ptrace_execve(char ** argv, char ** environ, char * exec_fn,
		bool_t parent_execve,
		void (*new_proc_prepare)(void * arg), void * arg);

extern void
ptrace_dupmem(pid_t target, void * dst, uintptr_t addr, size_t len);

extern void
ptrace_updmem(pid_t target, const void * src, uintptr_t addr, size_t len);

extern void
ptrace_kill(pid_t target);

extern void
ptrace_detach(pid_t target);

/* return the esp */
extern uintptr_t
ptrace_push(pid_t target, const void * data, size_t len);

extern void
ptrace_goto(pid_t target, uintptr_t eip);

#endif

// vim:ts=4:sw=4

