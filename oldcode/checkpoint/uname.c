#include "syscalls.h"


struct old_utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
};

#ifndef SYSCALL_PRINTER

int SCOPE
post_uname(const struct syscall_regs * regs)
{
	
	write_eax(regs);
	if (regs->eax >= 0) {
		write_mem(regs->ebx, sizeof(struct old_utsname));
	}
	return 0;
}

int SCOPE
replay_uname(const struct syscall_regs * regs)
{
	int32_t eax = read_int32();
	if (eax >= 0) {
		read_mem(regs->ebx, sizeof(struct old_utsname));
	}
	return eax;
}



#else

void
output_uname(int nr)
{
	int32_t ret = read_eax();
	if (ret >= 0)
		skip(sizeof(struct old_utsname));
	printf("uname:\t%d\n", ret);
}
#endif

