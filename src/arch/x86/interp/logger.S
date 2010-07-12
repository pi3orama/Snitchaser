/*
 * compiler.S
 * by WN @ Mar. 16, 2010
 */

#include <asm_offsets.h>
#include <xasm/unistd.h>
#include <xasm/signal_numbers.h>


/* see signal_helper.h */
#define SIG_BLOCK          0	/* for blocking signals */
#define SIG_UNBLOCK        1	/* for unblocking signals */
#define SIG_SETMASK        2	/* for setting the signal mask */

.text

.globl check_logger_buffer
check_logger_buffer:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	/* defined in interp/logger.c */
	call do_check_logger_buffer
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_LOGGER_CHECK_BUFFER_RETURN


#define RESET_SIGPROCMASK(type)	\
	pushf;									\
	subl $20, %esp;							\
	movl %eax, (%esp);						\
	movl %ebx, 4(%esp);						\
	movl %ecx, 8(%esp);						\
	movl %edx, 12(%esp);					\
	movl %esi, 16(%esp);					\
	movl $__NR_rt_sigprocmask, %eax;		\
	movl $SIG_SETMASK, %ebx;				\
	movl $OFFSET_##type##_SIGMASK, %ecx;		\
	addl %fs:OFFSET_TLS_BASE, %ecx;			\
	movl $0, %edx;							\
	movl $8, %esi;							\
	int $0x80;								\
	movl 16(%esp), %esi;					\
	movl 12(%esp), %edx;					\
	movl 8(%esp), %ecx;						\
	movl 4(%esp), %ebx;						\
	movl (%esp), %eax;						\
	addl $20, %esp;							\
	popf

.globl int80_syscall_entry
int80_syscall_entry:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call pre_log_syscall_int80
	cmp $0, %eax
	jne 1f

	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp

	/* reset sigprocmask here */
	RESET_SIGPROCMASK(UNBLOCK)
	int $0x80
	RESET_SIGPROCMASK(BLOCK)

	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl


	1: call post_log_syscall_int80
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp

	jmpl *%fs:OFFSET_REAL_BRANCH
	nop

.globl vdso_syscall_entry
vdso_syscall_entry:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call pre_log_syscall_vdso
	cmp $0, %eax
	jne 1f


	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp

	RESET_SIGPROCMASK(UNBLOCK)
	call *%fs:OFFSET_REAL_VDSO_SYSCALL_ENTRY
	RESET_SIGPROCMASK(BLOCK)

	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl


	1: call post_log_syscall_vdso
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp

	jmpl *%fs:OFFSET_REAL_BRANCH
	nop

/* don't generate executable stack */
.section        .note.GNU-stack,"",@progbits


// vim:ts=4:sw=4

