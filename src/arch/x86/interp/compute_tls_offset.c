/* 
 * compute_tls_offset.c
 * by WN @ Mar. 16, 2010
 */

#include <stdio.h>
#include <xasm/tls.h>
#include <xasm/processor.h>

#define PRINT(element, name)	\
	do {						\
		printf("#define OFFSET_" #name "\t(0x%x)\n",\
				TLS_STACK_SIZE - sizeof(struct thread_private_data) + \
				offsetof(struct thread_private_data, element));\
	} while(0);

static void
compute(void)
{

	printf("#define OFFSET_PUSHA_REGS (0x%x)\n",
			TLS_STACK_SIZE -
			sizeof(struct thread_private_data) - 
			sizeof(struct pusha_regs));
	printf("#define OFFSET_TLS_HEAD (0x%x)\n",
			TLS_STACK_SIZE - sizeof(struct thread_private_data));
	PRINT(old_stack_top, OLD_STACK_TOP);
	PRINT(target, TARGET);
	PRINT(reg_saver1, REG_SAVER1);
	PRINT(reg_saver2, REG_SAVER2);
	PRINT(first_branch, FIRST_BRANCH);
	PRINT(real_branch, REAL_BRANCH);
	PRINT(int80_syscall_entry, INT80_SYSCALL_ENTRY);
	PRINT(vdso_syscall_entry, VDSO_SYSCALL_ENTRY);
	PRINT(real_vdso_syscall_entry, REAL_VDSO_SYSCALL_ENTRY);

	PRINT(block_sigmask, BLOCK_SIGMASK);
	PRINT(unblock_sigmask, UNBLOCK_SIGMASK);
	PRINT(current_sigmask, CURRENT_SIGMASK);
	PRINT(sigactions, SIGACTIONS);

	PRINT(logger.check_logger_buffer, LOGGER_CHECK_LOGGER_BUFFER);
	PRINT(logger, LOGGER);
	PRINT(logger.check_buffer_return, LOGGER_CHECK_BUFFER_RETURN);
	PRINT(logger.log_buffer_start, LOGGER_LOG_BUFFER_START);
	PRINT(logger.log_buffer_current, LOGGER_LOG_BUFFER_CURRENT);
	PRINT(logger.log_buffer_end, LOGGER_LOG_BUFFER_END);

	PRINT(code_cache, CODE_CACHE);
	PRINT(code_cache.cache_dict, CODE_CACHE_CACHE_DICT);
	PRINT(code_cache.code_blocks, CODE_CACHE_CODE_BLOCKS);
	PRINT(code_cache.current_block, CODE_CACHE_CURRENT_BLOCK);

	PRINT(tid, PID);
	PRINT(pid, TID);
	PRINT(tnr, TNR);
	PRINT(stack_top, STACK_TOP);
	PRINT(tls_base, TLS_BASE);
	PRINT(no_record_signals, NO_RECORD_SIGNALS);
	PRINT(fs_val, FS_VAL);

	PRINT(current_syscall_nr, CURRENT_SYSCALL_NR);
}

int main()
{
	printf("/* This file is generated by compute_tls_offset */\n");
	printf("/* use %%fs:OFFSET_XXX can access tpd->xxx */\n");
	printf("#ifndef __X86_TLS_OFFSET_H\n");
	printf("#define __X86_TLS_OFFSET_H\n");
	compute();
	printf("#endif\n");
	printf("\n");
	printf("// vim:ts=4:sw=4\n");
	printf("\n");
	return 0;
}

// vim:ts=4:sw=4

