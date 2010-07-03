/* 
 * arch_replayer.h
 * by WN @ Jul. 01, 2010
 */

extern void
replay_patch_block(void);

extern void
replay_unpatch_block(void);

/* gdbserver should make sure 'target' in tpd is correct
 * before call it */
extern void
replay_is_branch_inst(void);

extern void
replay_nop(void);

extern void
replay_syscall_helper(void);

extern void
replay_get_next_branch(void);

/* ********* implemented in compiler.c ************ */
extern void
do_replay_patch_block(void);
extern void
do_replay_unpatch_block(void);
extern void
do_replay_is_branch_inst(void);
extern void
do_replay_get_next_branch(void);
/* ********** implemented in syscalls/replay_syscalls.c */
struct pusha_regs;
extern void
do_replay_syscall_helper(struct pusha_regs * regs);

// vim:ts=4:sw=4

