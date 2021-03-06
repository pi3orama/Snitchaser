/*
 * entry.S
 * by WN @ Jun. 23, 2009
 */

.section .data
.section .text

#define __ALIGN         .align 4,0x90

.type show_help_entry, @function
.global show_help_entry
__ALIGN
show_help_entry:
	call show_help
	movl $1, %eax
	int $0x80
	ret

__ALIGN
syscall_wrapper_entry:
	pushl %esp
	pushl 4(%esp)		/* eip */
	pushl %eax		/* orig_eax */
	pushf
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	call wrapped_syscall

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popf
	addl $0xC, %esp
	ret

.globl __entry
.type __entry, @function
__ALIGN
__entry:
	/* before go int __entry, loader should have been pushed 2 things:
	   (from high addr to low addr (the order of push)) should be:
	   main_entry, old_vdso_entry. */
	pushl %esp
	pushl 8(%esp)		/* eip */
	pushl $0		/* orig_eax */
	pushf
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	call injector_entry
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popf
	addl $16, %esp
	ret

.globl __debug_entry
.type __debug_entry, @function
__ALIGN
__debug_entry:
	/* here, we should have reset all registers, and push eip at
	   the top of the stack */
	/* according to the gcc register saving protocal, caller
	   need to save only 3 registers. however I forget which 3.  */
	/* gs is special, at least in 2.6.25.15, ptrace cannot reset gs
	   for the target. debug_entry should do this */
	/* debug_entry has 3 args:
	 * 1. regs;
	 * 2. stack_user
	 * 3. stack_used.
	 * 2 and 3 are used for libpthread
	 */
	pushl %esp
	pushl 12(%esp)		/* eip */
	pushl $0		/* orig_eax */
	pushf
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	call debug_entry

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popf
	/* pop the sym_stack_used and sym_stack_user */
	addl $20, %esp
	ret

.globl wrapped_sigreturn
.type wrapped_sigreturn, @function
__ALIGN
wrapped_sigreturn:
.LSTART_sigreturn:
	/* adjust stack. the 1st field of struct sigframe is
	   pretcode. when sighandler call 'ret', it return to
	   this address, make esp point to the 2nd field of this
	   struct. */
	subl	$0x4, %esp
	call	do_wrapped_sigreturn
	addl	$0x4, %esp
	popl	%eax
	movl	$0x77,%eax
	int	$0x80
.LEND_sigreturn:
	nop
.size wrapped_sigreturn, .-.LSTART_sigreturn

.globl wrapped_rt_sigreturn
.type wrapped_rt_sigreturn, @function
__ALIGN
wrapped_rt_sigreturn:
.LSTART_rt_sigreturn:
	subl	$0x4, %esp
	call	do_wrapped_rt_sigreturn
	addl	$0x4, %esp
	movl	$0xad,%eax
	int	$0x80
.LEND_rt_sigreturn:
	nop
	.size wrapped_rt_sigreturn, .-.LSTART_rt_sigreturn

.globl wrapped_sighandler
.type wrapped_sighandler, @function
__ALIGN
wrapped_sighandler:
	pushl $0x0123		/* this is placeholder for retcode */
	pushl %esp
	addl $4, (%esp)		/* add saved esp by 4, make the value correct */
	pushl $0		/* eip, useless here */
	pushl $0		/* orig_eax, placeholder */
	pushf
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	/* when make ckpt or other use */
	call do_wrapped_sighandler
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popf
	addl $0xC, %esp
	/* we must reset the retcode in do_wrapped_sighandler. */
	ret

