SET(SRC
	log_syscalls.h
	log_syscalls.c
	replay_syscalls.c
	handlers.h
	protos.h
	syscall_table.c
	syscall_table.h
	)

SET(SYSCALLS
	syscall_handler.h
	fstat64.c
	mmap2.c
	write.c
	exit_group.c
	exit.c
	rt_sigprocmask.c
	sigprocmask.c
	rt_sigaction.c
	nanosleep.c
	brk.c
	stat64.c
	read.c
	munmap.c
	futex.c
	ioctl.c
	time.c
	_llseek.c
	socketcall.c
	uname.c
	mprotect.c
	fcntl64.c
	_newselect.c
	clock_getres.c
	clock_gettime.c
	getcwd.c
	ugetrlimit.c
	poll.c
	pipe.c
	clone.c
	waitpid.c
	getdents64.c
	epoll_wait.c
	gettimeofday.c
	pread64.c
	prctl.c
	setitimer.c
	)

ADD_LIBRARY(log_syscall STATIC ${SRC})

ADD_LIBRARY(pre_library STATIC ${SYSCALLS})
ADD_LIBRARY(post_library STATIC ${SYSCALLS})
ADD_LIBRARY(replay_library STATIC ${SYSCALLS})


set_interp_properties(log_syscall)
set_interp_properties(pre_library)
set_interp_properties(post_library)
set_interp_properties(replay_library)

GET_TARGET_PROPERTY(pre_ori_cflags pre_library COMPILE_FLAGS)
GET_TARGET_PROPERTY(post_ori_cflags post_library COMPILE_FLAGS)
GET_TARGET_PROPERTY(replay_ori_cflags replay_library COMPILE_FLAGS)

SET_PROPERTY(TARGET pre_library
	PROPERTY COMPILE_DEFINITIONS PRE_LIBRARY)
SET_PROPERTY(TARGET pre_library
	PROPERTY COMPILE_FLAGS "${pre_ori_cflags} ${KERNEL_CFLAGS}")

SET_PROPERTY(TARGET post_library
	PROPERTY COMPILE_DEFINITIONS POST_LIBRARY)
SET_PROPERTY(TARGET post_library
	PROPERTY COMPILE_FLAGS "${post_ori_cflags} ${KERNEL_CFLAGS}")

SET_PROPERTY(TARGET replay_library
	PROPERTY COMPILE_DEFINITIONS REPLAY_LIBRARY)
SET_PROPERTY(TARGET replay_library
	PROPERTY COMPILE_FLAGS "${replay_ori_cflags} ${KERNEL_CFLAGS}")



TARGET_LINK_LIBRARIES(log_syscall pre_library)
TARGET_LINK_LIBRARIES(log_syscall post_library)
TARGET_LINK_LIBRARIES(log_syscall replay_library)

# vim:ts=4:sw=4

