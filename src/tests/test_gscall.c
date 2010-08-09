#include <stdio.h>
#include <stdlib.h>

static void
my_thread(void)
{
	printf("in my_thread\n");
	return;
}

int main()
{
	asm volatile ("movl %0, %%gs:0x234\n" : : "R" (my_thread));
	asm volatile ("call *%gs:0x234\n");
	return 0;
}


