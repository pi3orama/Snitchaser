/* 
 * socketcall.c
 * by WN @ Jul. 22, 2010
 */

#include "syscall_handler.h"

/* Argument list sizes for sys_socketcall */
#define AL(x) ((x) * sizeof(unsigned long))
static const unsigned char nargs[18]={
	AL(0),AL(3),AL(3),AL(3),AL(2),AL(3),
	AL(3),AL(3),AL(4),AL(4),AL(4),AL(6),
	AL(6),AL(2),AL(5),AL(5),AL(3),AL(3)
};

#ifndef PRE_LIBRARY
DEF_HANDLER(socketcall)
{

}
#endif

// vim:ts=4:sw=4

