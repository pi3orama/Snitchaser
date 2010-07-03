/*
 * replayer.S
 * by WN @ Jul. 01, 2010
 * entries for replayer
 */

#include <asm_offsets.h>

/*
 * below entries are for gdbserver:
 * gdbserver set OFFSET_TARGET and redirect code to it.
 */
.globl replay_patch_block
replay_patch_block:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_replay_patch_block
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_REAL_BRANCH

.globl replay_unpatch_block
replay_unpatch_block:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_replay_unpatch_block
	popfl
	popa
	movl %fs:OFFSET_OLD_STACK_TOP, %esp
	jmpl *%fs:OFFSET_REAL_BRANCH

.globl replay_is_branch_inst
replay_is_branch_inst:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	call do_replay_is_branch_inst
	/* never comes here */
	int3

/* replay_nop is all nop, gdbserver redirect code here to
 * cleanup waitid's status */
.globl replay_nop
replay_nop:
	nop
	nop

.globl replay_syscall_helper
replay_syscall_helper:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	/* esp is the first argument of do_replay_syscall_helper */
	pushl %esp
	/* defined in log_syscalls.c */
	call do_replay_syscall_helper
	/* never gets here */
	int3

.globl replay_get_next_branch
replay_get_next_branch:
	movl %esp, %fs:OFFSET_OLD_STACK_TOP
	movl %fs:OFFSET_STACK_TOP, %esp
	pusha
	pushf
	pushl $0x0202
	popfl
	/* esp is the first argument of do_replay_syscall_helper */
	pushl %esp
	/* defined in log_syscalls.c */
	call do_replay_get_next_branch
	/* never gets here */
	int3


/* don't generate executable stack */
.section        .note.GNU-stack,"",@progbits

