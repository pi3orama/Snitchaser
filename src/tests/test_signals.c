#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void
sighandler(int num)
{
	printf("signal %d\n", num);
	printf("signal %d end\n", num);
}

static void
xxfork(void)
{
	pid_t pid = fork();
	if (pid == 0)
		exit(0);
}

int main()
{
	signal(SIGCHLD, sighandler);
	xxfork();
	xxfork();
	xxfork();
	xxfork();
	xxfork();
	xxfork();
	xxfork();
	xxfork();
	xxfork();
	xxfork();
	sleep(10);
	return 0;
}


