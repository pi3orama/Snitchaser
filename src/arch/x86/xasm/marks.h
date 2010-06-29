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

#define SYSCALL_READ_START_MARK	(0xf0f0f0f0)
#define SYSCALL_READ_MARK	(0xe0e0e0e0)
#define SYSCALL_READ_ERROR_MARK	(0xffffffff)
#define SYSCALL_READ_END_MARK	(0x0f0f0f0f)

#endif

