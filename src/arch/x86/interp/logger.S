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


#define SET_EDX_0	movl $0, %edx
#define SET_EDX_CURRENT_SIGMASK	 \
	movl $OFFSET_CURRENT_SIGMASK, %edx; \
	addl %fs:OFFSET_TLS_BASE, %edx;


/* never change anything in memory. see comments in replay_syscalls.c. */
#define RESET_SIGPROCMASK(type, setoset)	\
	pushf;									\
	subl $20, %esp;							\
	movl %eax, (%esp);						\
	movl %ebx, 4(%esp);						\
	movl %ecx, 8(%esp);						\
	movl %edx, 12(%esp);					\
	movl %esi, 16(%esp);					\
	movl $__NR_rt_sigprocmask, %eax;		\
	movl $SIG_SETMASK, %ebx;				\
	movl $OFFSET_##type##_SIGMASK, %ecx;	\
	addl %fs:OFFSET_TLS_BASE, %ecx;			\
	setoset;								\
	movl $8, %esi;							\
	int $0x80;								\
	movl 16(%esp), %esi;					\
	movl 12(%esp), %edx;					\
	movl 8(%esp), %ecx;						\
	movl 4(%esp), %ebx;						\
	movl (%esp), %eax;						\
	addl $20, %esp;							\
	popf

#define UNBLOCK_SIGPROCMASK RESET_SIGPROCMASK(UNBLOCK, SET_EDX_0)
#define BLOCK_SIGPROCMASK RESET_SIGPROCMASK(BLOCK, SET_EDX_CURRENT_SIGMASK)

/* they are same */
.globl int80_syscall_entry
.globl vdso_syscall_entry
int80_syscall_entry:
vdso_syscall_entry:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call pre_log_syscall_entry
	cmp $0, %eax
	je normal_processing
	cmp $1, %eax
	je post_processing
	cmp $2, %eax
	je nosignal_processing
	cmp $3, %eax
	je clone_processing
	movl $1, %eax
	int $0x80

nosignal_processing:
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	int $0x80
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	jmp post_processing

normal_processing:
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp

	/* reset sigprocmask here */
	UNBLOCK_SIGPROCMASK
	int $0x80
	BLOCK_SIGPROCMASK

	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl

post_processing:
	call post_log_syscall_entry
	cmp $1, %eax
	je untrace_post_ret
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp

	jmpl *%fs:OFFSET_REAL_BRANCH
	nop
untrace_post_ret:
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_TARGET

clone_processing:
	popf
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	int $0x80
	pushf
	cmp $0, %eax
	jl clone_failed_processing
	jne clone_parent_processing
	popf

	/* this is clone_child processing */
	/* here, fs has not been changed, we cannot save esp
	 * in OFFSET_OLD_STACK_TOP. we save it in reg_saver1.
	 */

	movl %esp, %fs:OFFSET_REG_SAVER1
	/* don't reset stack */
	pusha
	pushf
	call clone_post_child
	popf
	popa
	/* fs should have been changed! */
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	/* for untraced clone, fs->real_branch should be set
	 * to 'target' */
	jmpl *%fs:OFFSET_REAL_BRANCH
	nop


clone_parent_processing:
	popf
	pusha
	pushf
	pushl $0x0202
	popfl
	call clone_post_parent
	jmp post_processing

clone_failed_processing:
	popf
	pusha
	pushf
	pushl $0x0202
	popfl
	jmp post_processing


/* don't generate executable stack */
.section        .note.GNU-stack,"",@progbits


// vim:ts=4:sw=4

