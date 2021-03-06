
#include "socketcall.h"
#ifndef SYSCALL_PRINTER

int SCOPE
post_recv(int fd, uint32_t ubuf, uint32_t size,
		uint32_t flags, int retval, const struct syscall_regs * regs)
{
	/* if addr and addr_len error, retval < 0, but
	 * data will still be copied into buffer */
	/* we save garbage data here */
	write_mem(ubuf, size);
	return 0;
}

int SCOPE
replay_recv(int fd, uint32_t ubuf, uint32_t size,
		uint32_t flags, int retval, const struct syscall_regs * regs)
{
	/* if addr and addr_len error, retval < 0, but
	 * data will still be copied into buffer */
	/* we save garbage data here */
	read_mem(ubuf, size);
	return retval;
}


#else

void SCOPE
output_recv(int fd, uint32_t ubuf, uint32_t size,
		uint32_t flags, int retval)
{
	printf("recv(fd=%d, ubuf=0x%x, size=%d, flags=0x%x):\t%d\n",
			fd, ubuf, size, flags, retval);
	skip(size);
}

#endif

