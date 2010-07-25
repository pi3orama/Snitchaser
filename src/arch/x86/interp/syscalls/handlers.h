/* 
 * handlers.h
 * by WN @ May. 27, 2010
 */

/* this file is used for being included into other .c or .h file */

def_handler(fstat64)
def_handler(mmap2)
def_handler(write)
def_complex_handler(exit_group)
def_complex_handler(exit)
def_complex_handler(rt_sigprocmask)
def_handler(sigprocmask)
def_trival_handler(getpid)
def_trival_handler(kill)
def_complex_handler(rt_sigaction)
def_handler(nanosleep)
def_trival_handler(gettid)
def_trival_handler(tgkill)
def_handler(brk)
def_trival_handler(open)
def_trival_handler(close)
def_handler(stat64)
def_handler(read)
def_handler(munmap)
def_handler(futex)
def_handler(ioctl)
def_handler(time)
def_handler(_llseek)
def_handler(socketcall)
def_handler(uname)
def_handler(mprotect)
def_handler(fcntl64)
def_handler(_newselect)
def_handler(clock_getres)
def_handler(clock_gettime)
def_trival_handler(utime)
def_trival_handler(getuid32)
def_handler(getcwd)
def_trival_handler(geteuid32)
def_trival_handler(getegid32)
def_trival_handler(getgid32)
def_handler(ugetrlimit)
def_handler(poll)
def_handler(pipe)
def_complex_handler(clone)
def_handler(waitpid)
def_trival_handler(writev)

// vim:ts=4:sw=4

