/* 
 * marks.h
 * by WN @ Jun. 25, 2010
 *
 * marks in log file and socket transmission
 *
 */

#ifndef MARKS_H
#define MARKS_H

#define SYSCALL_MAX	(0x1000)
#define SIGNAL_MIN	(0xffffff00)
#define RDTSC_MARK	(0x00001201)
#define NO_SIGNAL_MARK	(0x00001202)
#define SYSCALL_MARK	(0x00001203)
#define SIGNAL_MARK	(0x00001204)
#define SIGNAL_MARK_2	(0x00001205)
#define SIGNAL_TERMINATE	(0x00001206)
#define SIGNAL_HANDLER		(0x00001207)

#define SYSCALL_READ_START_MARK	(0xf0f0f0f0)
#define SYSCALL_READ_MARK	(0xe0e0e0e0)
#define SYSCALL_READ_ERROR_MARK	(0xffffffff)
#define SYSCALL_READ_END_MARK	(0x0f0f0f0f)

/* see comments in xasm/tls.h */
#define VALID_PTR_MIN	(0x6006000)
#define VALID_PTR_MAX	(0xC0000000)
#define IS_VALID_PTR(p)	(((p) < VALID_PTR_MAX) && ((p) > VALID_PTR_MIN))
#define IS_SYSCALL(p)	((p) < SYSCALL_MAX)

#endif

