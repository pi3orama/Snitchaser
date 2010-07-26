#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

static void
sighandler(int num)
{
	printf("signal %d\n", num);
	printf("signal %d end\n", num);
}

static int xxx = 0;

static pid_t
xxfork(void)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &set, NULL);
	xxx ++;
	pid_t pid = fork();
	if (pid == 0) {
		exit(xxx);
	}
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return pid;
}

int main()
{
	signal(SIGCHLD, sighandler);
	pid_t pids[10];
	int i = 0;
	for (i = 0; i < 10; i++)
		pids[i] = xxfork();

	for (i = 0; i < 10; i++) {
		int status;
		int err = waitpid(pids[i], &status, 0);
		printf("%d -- %d, %d(%s)\n", pids[i], err,
				status, strerror(errno));
	}
	return 0;
}


