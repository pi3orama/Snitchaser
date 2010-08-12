/* 
 * socketcall.c
 * by WN @ Jul. 22, 2010
 */

#include "syscall_handler.h"
#include <common/debug.h>
#include <xasm/string.h>

#define SYS_SOCKET	1		/* sys_socket(2)		*/
#define SYS_BIND	2		/* sys_bind(2)			*/
#define SYS_CONNECT	3		/* sys_connect(2)		*/
#define SYS_LISTEN	4		/* sys_listen(2)		*/
#define SYS_ACCEPT	5		/* sys_accept(2)		*/
#define SYS_GETSOCKNAME	6		/* sys_getsockname(2)		*/
#define SYS_GETPEERNAME	7		/* sys_getpeername(2)		*/
#define SYS_SOCKETPAIR	8		/* sys_socketpair(2)		*/
#define SYS_SEND	9		/* sys_send(2)			*/
#define SYS_RECV	10		/* sys_recv(2)			*/
#define SYS_SENDTO	11		/* sys_sendto(2)		*/
#define SYS_RECVFROM	12		/* sys_recvfrom(2)		*/
#define SYS_SHUTDOWN	13		/* sys_shutdown(2)		*/
#define SYS_SETSOCKOPT	14		/* sys_setsockopt(2)		*/
#define SYS_GETSOCKOPT	15		/* sys_getsockopt(2)		*/
#define SYS_SENDMSG	16		/* sys_sendmsg(2)		*/
#define SYS_RECVMSG	17		/* sys_recvmsg(2)		*/

#ifndef PRE_LIBRARY

#ifdef POST_LIBRARY
# define DEF_SUB_HANDLER(x)	static int post_##x
# define SUB_HANDLER(x)	post_##x
#else
# define DEF_SUB_HANDLER(x)	static int replay_##x
# define SUB_HANDLER(x)	replay_##x
#endif

/* Argument list sizes for sys_socketcall */
#define AL(x) ((x) * sizeof(unsigned long))
static const unsigned char nargs[18]={
	AL(0),AL(3),AL(3),AL(3),AL(2),AL(3),
	AL(3),AL(3),AL(4),AL(4),AL(4),AL(6),
	AL(6),AL(2),AL(5),AL(5),AL(3),AL(3)
};

DEF_SUB_HANDLER(getsockname)(int fd, uint32_t usockaddr,
		uint32_t usockaddr_len, int retval)
{
	if (retval < 0)
		return 0;

	BUFFER((void*)(usockaddr_len), sizeof(int));
	int len = *((int*)(usockaddr_len));
	assert((len > 0) && (len < 1000));

	/* in fact this is incorrect. the size of buffer 'usockaddr'
	 * may smaller than output 'len' so the address may not be
	 * fully copied. However this is rare case in real programs. */
	if (usockaddr != 0)
		BUFFER((void*)(usockaddr), len);

	return 0;
}

DEF_SUB_HANDLER(recvfrom)(int fd, uint32_t ubuf, uint32_t size,
		uint32_t flags, uint32_t addr, uint32_t paddr_len, int retval)
{
	if (retval < 0)
		return 0;
	BUFFER((void*)(ubuf), retval);
	if (addr != 0) {
		assert(paddr_len != 0);

		BUFFER((void*)paddr_len, sizeof(int));
		int len = *((int*)(paddr_len));

		BUFFER((void*)(addr), len);
	}
	return 0;
}

struct _iovec
{
	uint32_t iov_base;
	uint32_t iov_len;
};

struct _msghdr {
	uint32_t msg_name;	/* Socket name			*/
	uint32_t msg_namelen;	/* Length of name		*/
	uint32_t msg_iov;	/* Data blocks			*/
	uint32_t msg_iovlen;	/* Number of blocks		*/
	uint32_t msg_control;	/* Per protocol magic (eg BSD file descriptor passing) */
	uint32_t msg_controllen;	/* Length of cmsg list */
	uint32_t msg_flags;
};

DEF_SUB_HANDLER(recvmsg)(int fd, uint32_t msg, uint32_t flags, int retval)
{
	if (retval < 0)
		return 0;

	struct _msghdr hdr;
	BUFFER((void*)(msg), sizeof(hdr));
	memcpy(&hdr, (void*)(msg), sizeof(hdr));

	if (hdr.msg_name != 0)
		BUFFER((void*)(hdr.msg_name), hdr.msg_namelen);

	int iov_len = hdr.msg_iovlen;
	int sz_iovs = (sizeof(struct _iovec) * iov_len);

	BUFFER((void*)(hdr.msg_iov), sz_iovs);
	struct _iovec * vecs = (void*)(hdr.msg_iov);

	for (int i = 0; i < iov_len; i++)
		BUFFER((void*)(vecs[i].iov_base), vecs[i].iov_len);
	return 0;
}

DEF_SUB_HANDLER(recv)(int fd, uint32_t ubuf, uint32_t size,
		uint32_t flags, int retval)
{
	/* this is incorrect: sometime even recv fails,
	 * the ubuf is still filled. */
	if (retval <= 0)
		return 0;
	BUFFER((void*)(ubuf), retval);
	return 0;
}

DEF_SUB_HANDLER(getpeername)(int fd, uint32_t usockaddr,
		uint32_t usockaddr_len, int retval)
{
	if (retval < 0)
		return 0;
	if (usockaddr != 0) {
		assert(usockaddr_len != 0);
		BUFFER((void*)usockaddr_len, sizeof(int));
		BUFFER((void*)usockaddr, *(int*)(usockaddr_len));
	}
	return 0;
}

DEF_SUB_HANDLER(accept)(int fd, uintptr_t pupeer_sockaddr,
		uintptr_t pupeer_addrlen, int retval)
{
	if (retval <= 0)
		return 0;
	if (pupeer_sockaddr == 0)
		return 0;
	assert(pupeer_addrlen != 0);
	BUFFER((void*)pupeer_addrlen, sizeof(int));
	int l = *((int*)(pupeer_addrlen));
	BUFFER((void*)pupeer_sockaddr, l);
	return 0;
}

DEF_SUB_HANDLER(socketpair)(int family, int type, int protocol,
		uintptr_t usockvec, int retval)
{
	if (retval >= 0) {
		assert(usockvec != 0);
		BUFFER((void*)(usockvec), 2 * sizeof(int));
	}
	return 0;
}

DEF_SUB_HANDLER(getsockopt)(int fd, int level, int optname,
		uintptr_t optval, uintptr_t optlen, int retval)
{
	if (retval >= 0) {
		assert(optlen != 0);
		BUFFER((int*)(optlen), sizeof(int));
		BUFFER((void*)(optval), *((int*)(optlen)));
	}
	return 0;
}

DEF_HANDLER(socketcall)
{
	int call = regs->ebx;
	uint32_t args = regs->ecx;

	TRACE(LOG_SYSCALL, "socketcall, nr=0x%x\n", call);
	assert((call >= 1) && (call <= SYS_RECVMSG));

	unsigned long a0, a1, a2, a[6];
	uint32_t retval = regs->eax;

	if (nargs[call] > 0) {
		BUFFER((void*)args, nargs[call]);
		memcpy(a, (void*)(args), nargs[call]);
	}

	a0 = a[0];
	a1 = a[1];
	a2 = a[2];

	switch (call) {
	case SYS_GETSOCKNAME:
		return SUB_HANDLER(getsockname)(a0, a1, a2, retval);
	case SYS_RECVFROM:
		return SUB_HANDLER(recvfrom)(a0, a1, a2, a[3], a[4], a[5], retval);
	case SYS_RECVMSG:
		return SUB_HANDLER(recvmsg)(a0, a1, a2, retval);
	case SYS_RECV:
		return SUB_HANDLER(recv)(a0, a1, a2, a[3], retval);
	case SYS_GETPEERNAME:
		return SUB_HANDLER(getpeername)(a0, a1, a2, retval);
	case SYS_ACCEPT:
		return SUB_HANDLER(accept)(a0, a1, a2, retval);
	case SYS_SOCKETPAIR:
		return SUB_HANDLER(socketpair)(a0, a1, a2, a[3], retval);
	case SYS_GETSOCKOPT:
		return SUB_HANDLER(getsockopt)(a0, a1, a2, a[3], a[4], retval);
		/* trivial sockcalls */
	case SYS_CONNECT:
	case SYS_SENDTO:
	case SYS_SOCKET:
	case SYS_BIND:
	case SYS_SETSOCKOPT:
	case SYS_LISTEN:
	case SYS_SHUTDOWN:
	case SYS_SEND:
	case SYS_SENDMSG:
		return 0;
	default:
		FATAL(LOG_SYSCALL, "Unknown socket call: %d\n", call);
	}
	return 0;
}
#endif

// vim:ts=4:sw=4

