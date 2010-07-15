/*
 * arch_replayer.S
 * by WN @ Jul. 01, 2010
 * entries for replayer
 */

#include <asm_offsets.h>

.text
/*
 * below entries are for gdbserver
 */

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

/* replay_trap in an int3 instruction. gdbserver redirect code
 * here to set wait state */
/* if the process should be terminated, gdbserver needn't reset its eip.
 */
.globl replay_trap
replay_trap:
	int3
	movl $1, %eax
	int $0x80
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

