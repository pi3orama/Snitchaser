/* 
 * debug.c (interp)
 * by WN @ Feb. 22, 2010
 */
#define __DEBUG_C

#include <common/debug.h>
#include <common/assert.h>
#include <xasm/utils.h>
#include <xasm/debug.h>
#include <xasm/syscall.h>
#include <stdarg.h>
#include <unistd.h>

void
dbg_init(void)
{
	INTERNAL_SYSCALL_int80(close, 1, DBG_OUTPUT_FILE_NO);
	INTERNAL_SYSCALL_int80(dup2, 2, STDERR_FILENO, DBG_OUTPUT_FILE_NO);
}

void
dbg_output(enum __debug_level level,
#ifdef SNITCHASER_DEBUG
		enum __debug_component comp,
		const char * file ATTR(unused),
		const char * func,
		int line,
#endif
		char * fmt, ...)
{
	if (fmt == NULL)
		return;
	assert(level < NR_DEBUG_LEVELS);
#ifdef SNITCHASER_DEBUG
	assert(comp < NR_DEBUG_COMPONENTS);
	if (level < __debug_component_levels[comp])
		return;
#endif

#ifdef SNITCHASER_DEBUG
	fdprintf(DBG_OUTPUT_FILE_NO, "[%s %s@%s:%d]:\t",
			(char*)__debug_component_names[comp],
			(char*)__debug_level_names[level],
			func, line);
#else
	fdprintf(DBG_OUTPUT_FILE_NO, "%s:\t", (char*)__debug_level_names[level]);
#endif

	va_list ap;
	va_start(ap, fmt);
	vfdprintf(DBG_OUTPUT_FILE_NO, fmt, ap);
	va_end(ap);
}

void ATTR(noreturn)
__assert_fail (const char *__assertion, const char *__file,
		unsigned int __line, const char *__function)
{
	fdprintf(DBG_OUTPUT_FILE_NO, 
			"** %s:%d: %s: assertion `%s' failed **\n",
			__file, __line, __function, __assertion);
	breakpoint();
	__exit(-1);
}

void ATTR(noreturn)
dbg_fatal(void)
{
	breakpoint();
	__exit(-1);
}

#undef __DEBUG_C

// vim:ts=4:sw=4

