/* 
 * sockpair.c
 * by WN @ Jun. 11, 2010
 *
 * implement sockpair interface in host code
 *
 */

#include <common/defs.h>
#include <common/debug.h>
#include <common/replay/socketpair.h>
#include <host/exception.h>
#include <host/gdbserver/snitchaser_patch.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>

void
sock_send(void * data, size_t len)
{
	assert(data != NULL);
	int err;
	while (len > 0) {
		err = send(HOST_SOCKPAIR_FD, data, len, 0);
		if (err < 0)
			THROW_FATAL(EXP_SOCKPAIR_ERROR, "error %d when send data",
					err);
		len -= err;
		data += err;
	}
}

extern struct SN_info SN_info;

void
sock_recv(void * data, size_t len)
{
	assert(data != NULL);
	int err;
	while (len > 0) {
		/* select */
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(HOST_SOCKPAIR_FD, &set);
		err = TEMP_FAILURE_RETRY(
				select(FD_SETSIZE,
					&set, NULL, NULL,
					&tv));
		if (err < 0)
			THROW_FATAL(EXP_SOCKPAIR_ERROR, "select on socket %d failed",
					HOST_SOCKPAIR_FD);
		if (err == 0) {
			/* timeout: check target status */
			siginfo_t siginfo;
			siginfo.si_pid = 0;
			err = waitid(P_PID, SN_info.pid, &siginfo,
					WNOHANG | WEXITED | WSTOPPED | WNOWAIT);
			CTHROW_FATAL(err >= 0, EXP_PTRACE, "wait failed: %s\n",
					strerror(errno));
			if (siginfo.si_pid == 0)
				continue;

			/* target in waitable state, this means the corresponding
			 * 'send' syscall is interrupted by a signal.
			 *
			 * one situation need to take care:
			 * child:  ----------------send----signaled
			 * gdb:    recv--timeout----------------waitid
			 * however, even if we can recv this time, the child received
			 * an (unwanted) signal and should be kill
			 * */
			THROW_FATAL(EXP_GDBSERVER_TARGET_SIGNALED,
					"target is signaled by signal %d", siginfo.si_status);
		}

		err = recv(HOST_SOCKPAIR_FD, data, len, 0);
		if (err < 0)
			THROW_FATAL(EXP_SOCKPAIR_ERROR, "error %d when receive data",
					err);
		len -= err;
		data += err;
	}
}

// vim:ts=4:sw=4

