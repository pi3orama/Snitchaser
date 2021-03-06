
#include "socketcall.h"
#ifndef SYSCALL_PRINTER

int SCOPE
post_socket(int family, int type, int protocol, int retval, const struct syscall_regs * regs)
{
	return 0;
}

int SCOPE
replay_socket(int family, int type, int protocol, int retval, const struct syscall_regs * regs)
{
	return retval;
}


#else

void SCOPE
output_socket(int family, int type, int protocol, int retval)
{
	printf("socket(family=%d, type=%d, protocol=%d):\t%d\n",
			family, type, protocol, retval);
}

#endif

