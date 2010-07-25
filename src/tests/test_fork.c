#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	pid_t pid = fork();
	if (pid == 0) {
		printf("child %d\n", getpid());
		printf("xxxx\n");
		return 0;
	} else {
		printf("parent: %d\n", getpid());
		return 0;
	}
	return 0;
}
