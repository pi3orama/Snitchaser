#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	printf("begin\n");
	pid_t pid = getpid();
	printf("%d\n", pid);
	return 0;
}

