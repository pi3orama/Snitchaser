/* 
 * sockpair.h
 * by WN @ Jun. 10, 2010
 */

#ifndef SN_SOCKPAIR_H
#define SN_SOCKPAIR_H

/* this file is introduced to define socket pair which is used
 * in communication between gdbserver and target process. We will
 * explain sockpair in detail below.
 *
 * During replay, the target process and the gdbserver needs to
 * communicate with each other. For example, when executing system call,
 * the data is read by gdbserver and send to client using socket.
 * However, we still use signal driving process, that is, each time client
 * requires socket transmission, it kill itself using a SIGRTxx signal.
 *
 * this file define some macro used by socket transmission.
 *
 * */

/* this is the socket pair array, [0] is used in gdbserver(host),
 * [1] is (temporary) used in target (before execve) */
extern int socket_pair_fds[2];

#define HOST_SOCKPAIR_FD	(socket_pair_fds[0])
#define TARGET_SOCKPAIR_FD_TEMP	(socket_pair_fds[1])

/* TARGET_SOCKPAIR_FD is a fixed number used after execution. This 
 * design decision can simplify the libinterp's code. */
#define TARGET_SOCKPAIR_FD	(94)

#define TARGET_START_MARK	"START"
#define TARGET_START_MARK_SZ	sizeof(TARGET_START_MARK)

extern void
sock_send(void * data, size_t len);

extern void
sock_recv(void * data, size_t len);

#define def_sock_sr_TYPE(t, tn)	\
	static inline t sock_recv_##tn(void) {	\
		t r;	\
		sock_recv(&r, sizeof(r))	;\
		return r;	\
	}				\
	static inline void	\
	sock_send_##tn(t x)	\
	{					\
		sock_send(&x, sizeof(x));	\
	}

def_sock_sr_TYPE(bool_t, bool)
def_sock_sr_TYPE(uintptr_t, ptr)
def_sock_sr_TYPE(uint32_t, u32)
def_sock_sr_TYPE(uint16_t, u16)
def_sock_sr_TYPE(uint8_t, u8)

#undef def_sock_sr_TYPE

#if 0
static inline bool_t
sock_recv_bool(void)
{
	bool_t r;
	sock_recv(&r, sizeof(r));
	return r;
}

static inline void
sock_send_bool(bool_t b)
{
	sock_send(&b, sizeof(b));
}
#endif

#endif

// vim:ts=4:sw=4

