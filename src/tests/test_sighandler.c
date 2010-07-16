#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void
handler(int num)
{
	printf("signal %d raised\n", num);
	exit(-1);
}

int main()
{
	signal(SIGSEGV, handler);
	int * ptr = (void*)0x1234;
	*ptr = 10;
	return 0;
}

