/* 
 * x86/asm/logger.h
 * by WN @ Mar. 16, 2010
 */

#ifndef __X86_ASM_LOGGER_H
#define __X86_ASM_LOGGER_H

#include <config.h>
#include <common/defs.h>
extern ATTR_HIDDEN void
ud_logger_entry(void);

extern ATTR_HIDDEN void
log_branch_target(void);

#endif
