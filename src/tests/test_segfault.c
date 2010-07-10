#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void
signalx(int n)
{
	printf("receive signal %d\n", n);
	exit(1);
	return;
}

int main()
{
	sigset_t new_set, old_set;
	sigfillset(&new_set);
	sigprocmask(SIG_BLOCK, &new_set, &old_set);
	signal(SIGSEGV, signalx);
	int * ptr = (void*)(0x1234);
	*ptr = 10;
	return 0;
}

