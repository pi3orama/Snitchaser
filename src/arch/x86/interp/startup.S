/*
 * startup.S
 * by WN @ Mar. 16, 2010
 */

#include <asm_offsets.h>
#include <config.h>

.globl _start
_start:
	/* the entry of interp */
 	push $0
	/* the user entry */
	push $0
	pusha
	pushf
	/* xmain should be a hidden symbol */
	call xmain
	test %eax, %eax
	jne 1f
	popf
	popa
	jmp 2f
	1:
	popf
	popl %edi
	popl %esi
	popl %ebp
	addl $4, %esp
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax
	movl -20(%esp), %esp
	/* begin running */
	2:

	/* check conf_selection */
	pusha
	pushf
	movl %fs:OFFSET_CONF_SELECTION, %eax
	test %eax, %eax
	jz not_selected
	/* selected */
	popf
	popa

	/* tpd->first_branch is set in interp/tls.c,
	 * to first_branch in user_entry.c */
	call *%fs:OFFSET_FIRST_BRANCH
	/* pop the user_entry, stack top is interp_entry */
	addl $4, %esp
	ret

not_selected:
	popf
	popa
	addl $4, %esp
	ret

.globl _xstart
_xstart:
 	push $0
	push $0
	pusha
	pushf
	/* xmain should be a hidden symbol */
	call xmain
	test %eax, %eax
	jne 1f
	popf
	popa
	jmp 2f
	1:
	popf
	popl %edi
	popl %esi
	popl %ebp
	addl $4, %esp
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax
	movl -20(%esp), %esp
	/* begin running */
	2:	/* ret to its original entry */
	addl $4, %esp
	ret

/* replayer start from this entry */
.globl REPLAYER_ENTRY
REPLAYER_ENTRY:
	/* open checkpoint file, restore tls stack */
	/* gs and fs should have been restored in restore_tls_stack */
	call restore_tls_stack

	/* we must save fs:OFFSET_OLD_STACK_TOP, this is because that,
	 * when making checkpoint, the esp saved using "pusha" is incorrect,
	 * it points to tls stack, and the real esp is saved in old_stack_top.
	 * But things is easier than normal case: when replay start, all register
	 * is free.
	 */
	movl %fs:OFFSET_OLD_STACK_TOP, %eax

	/* then we can destroy old_stack_top safely */
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pushl $0x0202
	popfl

	/* gs, fs and floating registers should have
	 * been restored in replayer_main */
	/* ckeckpoint file should be closed in replayer_main */
	/* the registers should be fixed in replayer_main */
	pusha
	pushf

	/* this is saved real esp */
	pushl %eax

	call replayer_main
	popf
	popa
	/* tls->old_stack_top should have been adjusted by replayer_main */
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	/* real_branch should have been set in replayer_main */
	jmpl *%fs:OFFSET_TARGET

/* don't generate executable stack */
.section        .note.GNU-stack,"",@progbits

