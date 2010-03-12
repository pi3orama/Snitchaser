/* 
 * main program of a interp
 */


#define __LOADER_MAIN_C
#include <config.h>

#include <common/defs.h>
#include <common/debug.h>
#include <common/sys/personality.h>

#include <asm/startup.h>
#include <asm/debug.h>
#include <asm/syscall.h>
#include <asm/vsprintf.h>
#include <asm/utils.h>
#include <asm/tls.h>
#include <interp/auxv.h>
#include <interp/mm.h>

/* reexec reset the personality bit ADDR_NO_RANDOMIZE to make sure
 * the process' memory layout is idential */
static void
reexec(void * old_esp)
{
	TRACE(LOADER, "old_esp = %p\n", old_esp);
	int err = INTERNAL_SYSCALL_int80(personality, 1, 0xffffffff);
	assert(err >= 0);
	TRACE(LOADER, "personality=0x%x\n", err);

	if (err & ADDR_NO_RANDOMIZE) {
		TRACE(LOADER, "ADDR_NO_RANDOMIZE bit has already been set\n");
		return;
	}

	int persona = err | ADDR_NO_RANDOMIZE;
	TRACE(LOADER, "persona should be 0x%x\n", persona);
	err = INTERNAL_SYSCALL_int80(personality, 1, persona);
	assert(err >= 0);

	/* buildup env and cmdline */
	/* iterate over args */
	const char ** new_argv = NULL;
	const char ** new_env = NULL;
	uintptr_t * ptr = old_esp;

	int argc = *(ptr ++);
	TRACE(LOADER, "argc = %d\n", argc);
	new_argv = (const char **)(ptr);
	new_env = new_argv + argc + 1;

	/* execve */
	err = INTERNAL_SYSCALL_int80(execve, 3, new_argv[0], new_argv, new_env);
	FATAL(LOADER, "execve failed: %d\n", err);
	return;
}

extern void * loader(void * oldesp, int * pesp_add);
__attribute__((used, unused)) static int
xmain(volatile struct pusha_regs regs)
{
	relocate_interp();
	void * oldesp = (void*)stack_top(&regs) + sizeof(uintptr_t);
	VERBOSE(LOADER, "oldesp=%p\n", oldesp);
	reexec(oldesp);
	find_auxv(oldesp);

	int esp_add = 0;
	void * retaddr = loader(oldesp, &esp_add);

	stack_top(&regs) += esp_add * sizeof(uintptr_t);
	void ** pretaddr = (void**)(stack_top(&regs));

	/* build first thread local area */
	init_tls();

	struct thread_private_data * tpd = 
		get_tpd();
	DEBUG(LOADER, "pid from tpd: %d; tid from tpd: %d\n",
			tpd->pid, tpd->tid);

	/* init code cache */
	/* redirect control flow to code cache */

	*pretaddr = retaddr;

	return esp_add;
}


#undef __LOADER_MAIN_C

// vim:ts=4:sw=4
