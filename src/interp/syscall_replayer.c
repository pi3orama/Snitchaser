/* 
 * syscall_replayer.c
 * by WN @ Jun. 28, 2010
 */

#include <interp/syscall_replayer.h>
#include <common/replay/socketpair.h>

void
read_syscall_data(void * ptr, size_t len)
{
	sock_send_u32(SYSCALL_READ_MARK);
	sock_send_int(len);
	sock_recv(ptr, len);
}

void
initiate_syscall_read(void)
{
	sock_send_u32(SYSCALL_READ_START_MARK);
}

void
finish_syscall_read(void)
{
	sock_send_u32(SYSCALL_READ_END_MARK);
}

// vim:ts=4:sw=4

