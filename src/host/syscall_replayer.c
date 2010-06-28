/* 
 * syscall_replayer.c
 * by WN @ Jun. 28, 2010
 */

#include <common/defs.h>
#include <host/exception.h>

#include <common/debug.h>
#include <host/syscall_replayer.h>
#include <common/replay/socketpair.h>
#include <host/replay_log.h>

void
syscall_read_cycle(void)
{
	uint32_t begin_mark = sock_recv_u32();
	CTHROW_FATAL(begin_mark == SYSCALL_READ_START_MARK,
			EXP_SYSCALL_REPLAYER, "wait for SYSCALL_READ_START_MARK "
			"but receive 0x%x", begin_mark);

#define SEND_BUF_SZ	(4096)

	uint8_t buffer[SEND_BUF_SZ];

	/* loop */
	uint32_t mark = sock_recv_u32();
	while (mark == SYSCALL_READ_MARK) {
		/* receive size */
		size_t size = sock_recv_u32();
		while (size > 0) {
			/* read from log */
			size_t read_size = (size > SEND_BUF_SZ) ? SEND_BUF_SZ : size;
			read_log_full(buffer, read_size);
			sock_send(buffer, read_size);
			size -= read_size;
		}
		mark = sock_recv_u32();
	}

	CTHROW_FATAL(mark == SYSCALL_READ_END_MARK,
			EXP_SYSCALL_REPLAYER, "wait for SYSCALL_READ_END_MARK "
			"but receive -x%x", mark);
}

// vim:ts=4:sw=4

