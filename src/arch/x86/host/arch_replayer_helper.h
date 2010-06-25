/* 
 * arch_replayer.h
 * by WN @ Jun. 18, 2010
 */

#ifndef X86_ARCH_REPLAYER_H
#define X86_ARCH_REPLAYER_H

#include <sys/user.h>
#include <xasm/processor.h>

#include <unistd.h>

void
ptrace_get_regset(pid_t pid, struct user_regs_struct * urs);

void
ptrace_set_regset(pid_t pid, struct user_regs_struct * urs);

uintptr_t
ptrace_get_reg(pid_t pid, unsigned int offset);

void
ptrace_set_reg(pid_t pid, uintptr_t val, unsigned int offset);

#define def_ptrace_getset_REG(reg)	\
	inline static uintptr_t		\
	ptrace_get_##reg(pid_t pid)	\
	{							\
		return ptrace_get_reg(pid, offsetof(struct user_regs_struct, reg));	\
	}	\
	inline static void			\
	ptrace_set_##reg(pid_t pid, uintptr_t val)	\
	{											\
		ptrace_set_reg(pid, val, offsetof(struct user_regs_struct, reg));	\
	}

def_ptrace_getset_REG(eip)
def_ptrace_getset_REG(eax)
def_ptrace_getset_REG(ebx)
def_ptrace_getset_REG(ecx)
def_ptrace_getset_REG(edx)
def_ptrace_getset_REG(esp)
def_ptrace_getset_REG(ebp)
def_ptrace_getset_REG(esi)
def_ptrace_getset_REG(edi)

#undef def_ptrace_getset_REG

/* if eip == NULL, don't reset eip */
void
arch_restore_registers(pid_t pid, struct pusha_regs * regs, void * eip);

#endif

// vim:ts=4:sw=4

