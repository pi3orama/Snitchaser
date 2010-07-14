/*
 * arch_signal.S
 * by WN @ Jul. 12, 2010
 */

#include <asm_offsets.h>
#include <xasm/unistd.h>

.globl arch_wrapper_rt_sighandler
.globl arch_wrapper_sighandler
.globl arch_wrapper_rt_sigreturn
.globl arch_wrapper_sigreturn

arch_wrapper_rt_sighandler:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_arch_wrapper_rt_sighandler
	cmpl $1, %eax
	je 1f
	cmpl $2, %eax
	je signal_terminate
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_REAL_BRANCH
	/* if do_arch return non-zero, sigreturn directly */
	1:
	/* call sigreturn */
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	/* see vdso code, __kernel_rt_sigreturn is different
	 * from __kernel_sigreturn */
	addl $4, %esp
	movl $0xad, %eax
	int $0x80
	nop


arch_wrapper_sighandler:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_arch_wrapper_sighandler
	cmpl $1, %eax
	je 1f
	cmpl $2, %eax
	je signal_terminate
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_REAL_BRANCH
	1:
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	/* see vdso code */
	addl $8, %esp
	movl $0x77, %eax
	int $0x80
	nop

signal_terminate:
	movl $1, %eax
	int $0x80

arch_wrapper_rt_sigreturn:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_arch_wrapper_rt_sigreturn
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	movl $__NR_rt_sigreturn, %eax
	int $0x80
	nop

arch_wrapper_sigreturn:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_arch_wrapper_sigreturn
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	movl $__NR_sigreturn, %eax
	int $0x80
	nop




/* don't generate executable stack */
.section        .note.GNU-stack,"",@progbits

