#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

int main()
{
	sigset_t m1, m2;
	sigfillset(&m1);
	int err = sigprocmask(SIG_BLOCK, &m1, &m2);
	assert(err == 0);
	return 0;
}

