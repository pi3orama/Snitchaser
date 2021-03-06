/* 
 * x86/xasm/syscall_helper.h
 * by WN @ Apr. 04, 2010
 */

#ifndef __X86_XASM_TYPES_HELPER_H
#define __X86_XASM_TYPES_HELPER_H

#include <linux/time.h>
#include <xasm/processor.h>
#include <xasm/signal_helper.h>
/* struct user_regs_struct */
#include <sys/user.h>
#ifdef PAGE_SIZE
# undef PAGE_SIZE
# define PAGE_SIZE	(4096)
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#endif

// vim:ts=4:sw=4

