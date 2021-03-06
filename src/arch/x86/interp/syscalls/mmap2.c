/* 
 * mmap2.c
 * by WN @ June 1, 2010
 */


#include "syscall_handler.h"
#include <linux/fs.h>
#include <asm/mman.h>
#include <xasm/syscall.h>
#include <common/debug.h>


#define PTR_CORRECT(ptr)	((uintptr_t)(ptr) < 0xc0000000)

#ifdef PRE_LIBRARY
/* nothing to do */
#endif

#ifdef POST_LIBRARY
int
post_mmap2(struct pusha_regs * regs)
{
	TRACE(LOG_SYSCALL, "post mmap2, eax=0x%x, len=%d\n",
			regs->eax, regs->ecx);
	void * ptr = (void*)regs->eax;
	uint32_t port = regs->edx;
	if (PTR_CORRECT(ptr)) {
		size_t len = regs->ecx;
		if ((int)(regs->edi) > 0) {
			/* this is file mapping */
			struct stat64 stat;
			int err = INTERNAL_SYSCALL_int80(fstat64, 2, regs->edi, &stat);
			assert(err == 0);
			long long file_size = stat.st_size;
			long long start_pos = ((unsigned long)(regs->ebp)) * 4096;
			assert(file_size > start_pos);
			unsigned long long aval_size = file_size - start_pos;
			size_t real_size = (len < aval_size) ? (len) : ((size_t)(aval_size));

			len = real_size;
		}

		INT_VAL(len);
		if (port & PROT_READ) {
			BUFFER(ptr, len);
		} else {
			int err = INTERNAL_SYSCALL_int80(mprotect, 3, ptr,
					len, port | PROT_READ);
			assert(err == 0);
			BUFFER(ptr, len);

			err = INTERNAL_SYSCALL_int80(mprotect, 3, ptr,
					len, port);
			assert(err == 0);
		}
	}
	TRACE(LOG_SYSCALL, "end post mmap2\n");
	return 0;
}
#endif

#ifdef REPLAY_LIBRARY
int
replay_mmap2(struct pusha_regs * regs)
{
	TRACE(LOG_SYSCALL, "replay mmap2\n");

	void * ptr = (void*)regs->eax;
	if (PTR_CORRECT(ptr)) {
		/* mmap anon pages */

		uint32_t len = regs->ecx;
		uint32_t prot = regs->edx;
		prot |= PROT_WRITE;

		uint32_t flags = MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE;

		uintptr_t ret;
		/* it is a anon map */
		ret = INTERNAL_SYSCALL_int80(mmap2, 6,
				(uint32_t)(ptr), len, prot,
				flags, -1, 0);
		if (ret != (uintptr_t)(ptr)) {
			ERROR(LOG_SYSCALL, "mmap2 error:\n");
			ERROR(LOG_SYSCALL, "ptr=0x%x\n", (uint32_t)(ptr));
			ERROR(LOG_SYSCALL, "len=0x%x\n", len);
			ERROR(LOG_SYSCALL, "prot=0x%x\n", prot);
			ERROR(LOG_SYSCALL, "flags=0x%x\n", flags);
			FATAL(LOG_SYSCALL, "mmap2 inconsistency: 0x%lx vs 0x%lx\n",
					ret, (uintptr_t)(ptr));
		}
		size_t real_size;
		INT_VAL(real_size);
		BUFFER(ptr, real_size);

		/* reprotect */
		ret = INTERNAL_SYSCALL_int80(mprotect, 3, ptr, len, regs->edx);
		assert(ret == 0);
	}

	return 0;
}
#endif

// vim:ts=4:sw=4

