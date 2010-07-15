#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	printf("Wow\n");

	int i;
	for (i = 0; i < 0x3; i++)
		;
	i += 20;
	int * ptr = (void*)(i);
	*ptr = 10;

	printf("aaa\n");
	printf("bbb\n");
	return 0;
}

