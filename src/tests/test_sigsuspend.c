#include <signal.h>
#include <stdio.h>

void handler(int num)
{
	printf("%d\n", num);
	sigset_t mask;
	sigprocmask(SIG_BLOCK, NULL, &mask);
	int *p = (void*)&mask;
	printf("0x%x, 0x%x\n",
			p[0],
			p[1]);
	return;
}

int main()
{
	struct sigaction act;
	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGRTMIN);
	act.sa_flags = SA_RESTART;
	sigaction(SIGINT, &act, NULL);

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN + 10);
	sigsuspend(&mask);
	sigprocmask(SIG_BLOCK, NULL, &mask);
	int *p = (void*)&mask;
	printf("0x%x, 0x%x\n",
			p[0],
			p[1]);
	return 0;
}

