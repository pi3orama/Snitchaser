#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


int main()
{

	printf("Wow\n");

	int * ptr = (void*)(0x1234);
	*ptr = 10;

	printf("aaa\n");
	printf("bbb\n");
	


	return 0;
}

