/* 
 * compute_tls_offset.c
 * by WN @ Mar. 16, 2010
 */

#include <stdio.h>
#include <asm/tls.h>

#define PRINT(element, name)	\
	do {						\
		printf("#define OFFSET_" #name "\t(0x%x)\n",\
				TLS_STACK_SIZE - sizeof(struct thread_private_data) + \
				offsetof(struct thread_private_data, element));\
	} while(0);

static void
compute(void)
{
	PRINT(old_stack_top, OLD_STACK_TOP);
	PRINT(logger_entry, LOGGER_ENTRY);
	PRINT(ud_logger_entry, UD_LOGGER_ENTRY);
	PRINT(ui_logger_entry, UI_LOGGER_ENTRY);
	PRINT(cd_logger_entry, CD_LOGGER_ENTRY);
	PRINT(ci_logger_entry, CI_LOGGER_ENTRY);
	PRINT(log_buffer_sz, LOG_BUFFER_SZ);
	PRINT(log_buffer, LOG_BUFFER);
	PRINT(exit_addr, EXIT_ADDR);
	PRINT(code_cache, CODE_CACHE);
	PRINT(code_cache.target, CODE_CACHE_TARGET);
	PRINT(code_cache.if_taken, CODE_CACHE_IF_TAKEN);
	PRINT(code_cache.cache_dict, CODE_CACHE_CACHE_DICT);
	PRINT(code_cache.code_blocks, CODE_CACHE_CODE_BLOCKS);
	PRINT(code_cache.last_block, CODE_CACHE_LAST_BLOCK);
	PRINT(tid, PID);
	PRINT(pid, TID);
	PRINT(tnr, TNR);
	PRINT(stack_top, STACK_TOP);
	PRINT(tls_base, TLS_BASE);
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

