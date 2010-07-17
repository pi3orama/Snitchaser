#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void
handler(int num)
{
	printf("signal %d raised\n", num);
}

int main()
{
	signal(SIGCONT, handler);
	raise(SIGTSTP);
	printf("come back!!!\n");
	return 0;
}

