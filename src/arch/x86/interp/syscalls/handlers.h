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

// vim:ts=4:sw=4

