
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

static int x = 0;

static void
handler(int num)
{
	printf("signal %d\n", num);

	sigset_t set, oset;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, &oset);
	sigprocmask(SIG_BLOCK, NULL, &oset);

	if (x == 0) {
		x = 1;
		raise(SIGUSR1);
	}
	printf("out sighandler\n");
}

int main()
{
	struct sigaction act, oact;

	int err;

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	err = sigaction(SIGUSR1, &act, &oact);

	raise(SIGUSR1);

	printf("out!\n");

	return 0;
}

