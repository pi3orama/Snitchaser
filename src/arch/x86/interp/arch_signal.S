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
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_REAL_BRANCH

arch_wrapper_sighandler:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_arch_wrapper_sighandler
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_REAL_BRANCH

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

