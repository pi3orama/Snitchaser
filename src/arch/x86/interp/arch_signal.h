/* 
 * arch_signal.h
 * by WN @ Jul. 07, 2010
 */

#ifndef __ARCH_SIGNAL_H
#define __ARCH_SIGNAL_H

#include <xasm/signal_helper.h>
#include <xasm/processor.h>

k_sigset_t
arch_replay_mask_signals(void);

k_sigset_t
arch_set_sigmask(k_sigset_t old_set);

void        
arch_init_signal(void);

/* defined in arch_signal.S */
void
arch_wrapper_rt_sighandler(void);

void
arch_wrapper_sighandler(void);

void
arch_wrapper_rt_sigreturn(void);

void
arch_wrapper_sigreturn(void);

/* defined in arch_signal.c */
int
do_arch_wrapper_rt_sighandler(struct pusha_regs * regs);

int
do_arch_wrapper_sighandler(struct pusha_regs * regs);

void
do_arch_wrapper_rt_sigreturn(void);

void
do_arch_wrapper_sigreturn(void);

/* for clone use: restore signal handler and sigprocmask */
void
arch_restore_signal(void);

#endif

// vim:ts=4:sw=4

