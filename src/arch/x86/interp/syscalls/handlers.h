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
def_handler(rt_sigprocmask)
def_handler(sigprocmask)
def_trival_handler(getpid)
//def_complex_handler(rt_sigaction)

// vim:ts=4:sw=4

