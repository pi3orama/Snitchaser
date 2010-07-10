/* 
 * arch_signal.h
 * by WN @ Jul. 07, 2010
 */

#ifndef __ARCH_SIGNAL_H
#define __ARCH_SIGNAL_H

#include <xasm/signal_helper.h>

k_sigset_t
arch_replay_mask_signals(void);

k_sigset_t
arch_set_sigmask(k_sigset_t old_set);

void        
arch_init_signal(void);

#endif

// vim:ts=4:sw=4

