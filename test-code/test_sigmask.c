#include <signal.h>
#include <stdio.h>

static void handler(int num)
{
	printf("%d\n", num);
	sigset_t set;
	int * p = &set;
	sigprocmask(SIG_BLOCK, NULL, &set);
	printf("0x%x, 0x%x\n", p[0], p[1]);
	return;
}

int main()
{
	struct sigaction act;
	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	sigemptyset(&act.sa_mask);
	sigaddset(SIGRTMIN + 5, &act.sa_mask);
	sigaction(SIGINT, &act, NULL);
	raise(SIGINT);
	return 0;
}

