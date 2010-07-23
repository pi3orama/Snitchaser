/* 
 * ioctl.c
 * by WN @ Jul. 22, 2010
 */

#include "syscall_handler.h"
#include <xasm/syscall.h>
#include <common/debug.h>

#include "ioctl.h"
#include "ioctl_tty.h"
#include "ioctl_blk.h"

#ifndef PRE_LIBRARY

#ifdef POST_LIBRARY
# define TTY_IOCTL post_tty_ioctl
# define BLK_IOCTL post_blk_ioctl
#else
# define TTY_IOCTL replay_tty_ioctl
# define BLK_IOCTL replay_blk_ioctl
#endif

static int
TTY_IOCTL(int fd, int cmd, int arg, struct pusha_regs * regs)
{
	switch (cmd) {
	case TCGETS:
		if (arg != 0)
			BUFFER((void*)arg, sizeof(struct termios));
		return 0;
	case TIOCGWINSZ:
		if (arg != 0)
			BUFFER((void*)arg, sizeof(struct winsize));
		return 0;
	case FIONREAD:
		if (arg != 0)
			BUFFER((void*)arg, sizeof(int));
		return 0;
	case FIONBIO:
		return 0;
	case TCGETA:
		if (arg != 0)
			BUFFER((void*)arg, sizeof(struct termio));
		return 0;
	case TCSETA:
		return 0;
	default:
		FATAL(LOG_SYSCALL, "doesn't know such tty ioctl: 0x%x\n", cmd);
	}
	return 0;
}

static int
BLK_IOCTL(int fd, int cmd, int arg, struct pusha_regs * regs)
{
	if ((int)(regs->eax) < 0)
		return 0;

	if (_IOC_DIR(cmd) == _IOC_READ) {
		assert(arg != 0);
		BUFFER((void*)arg, _IOC_SIZE(cmd));
	}
	return 0;
}


DEF_HANDLER(ioctl)
{
	TRACE(LOG_SYSCALL, "ioctl\n");

	int fd = regs->ebx;
	int cmd = regs->ecx;
	int arg = regs->edx;

	switch (_IOC_TYPE(cmd)) {
	case 'T':
		return TTY_IOCTL(fd, cmd, arg, regs);
	case 0x12:
		return BLK_IOCTL(fd, cmd, arg, regs);
	default:
		FATAL(LOG_SYSCALL, "no such ioctl command: 0x%x\n", cmd);
	}
}
#endif

// vim:ts=4:sw=4

