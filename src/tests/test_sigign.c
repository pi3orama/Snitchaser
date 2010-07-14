#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	printf("begin\n");
	kill(getpid(), SIGWINCH);
	printf("end\n");
	return 0;
}

