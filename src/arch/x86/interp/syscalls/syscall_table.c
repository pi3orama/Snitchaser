/* 
 * syscall_table.c
 * by WN @ May. 27, 2010
 */

#include <common/defs.h>
#include <interp/logger.h>
#include <interp/replayer.h>
#include <xasm/unistd_32.h>

/* don't include syscall_handler.h! it is special! */
#include <interp/syscall_replayer.h>

#include "protos.h"
#include "syscall_table.h"

/* trivial pre handler: do nothing */
/* trivial post handler: save eax only */

#define __def_handler(name, pre, post, replay) [__NR_##name] = {pre, post, replay},

#define def_trivial_handler(name)	__def_handler(name, trivial_pre_handler,	\
	trivial_post_handler, trivial_replay_handler)

#define def_handler(name)	__def_handler(name, trivial_pre_handler, post_##name, replay_##name)
#define def_complex_handler(name)	__def_handler(name, pre_##name, post_##name, replay_##name)

int
trivial_pre_handler(struct pusha_regs * regs ATTR_UNUSED)
{
	return 0;
}

int
trivial_post_handler(struct pusha_regs * regs ATTR_UNUSED)
{
	/* don't record anything */
	return 0;
}

int
trivial_replay_handler(struct pusha_regs * regs ATTR_UNUSED)
{
	return 0;
}

struct syscall_table_entry syscall_table[SYSCALL_TABLE_SZ] = {
#include "handlers.h"
};

#undef def_complex_handler
#undef def_handler
#undef def_trivial_handler
#undef __def_handler

// vim:ts=4:sw=4

