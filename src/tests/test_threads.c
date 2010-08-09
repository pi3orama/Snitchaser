#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static int xxx = 0;

static void *
xthread(void * arg)
{
	int s = (int)(arg);

	xxx = s;

	sleep(s);
	printf("in thread %p\n", arg);

	asm volatile (
			"movl $1, %eax\n"
			"movl $1, %ebx\n"
			"int $0x80\n"
			);

	pthread_exit((void*)s);
	return NULL;
}

int main()
{
	pthread_t p1, p2;
	pthread_create(&p1, NULL, xthread, (void*)1);
	pthread_create(&p2, NULL, xthread, (void*)2);
	pthread_join(p2, NULL);
	pthread_join(p1, NULL);

	if (xxx == 2)
		printf("aaaaaaa %d\n", xxx);
	else
		printf("bbbbbbbbbb\n");

	return 0;
}

