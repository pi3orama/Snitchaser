#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

static void
sighandler1(int signum)
{
	printf("this is sighandler1: %d\n", signum);
	return;
}

static void
sighandler2(int signum)
{
	printf("this is sighandler2: %d\n", signum);
	return;
}

static void
sighandler3(int signum)
{
	printf("this is sighandler3: %d\n", signum);
	return;
}


static void *
threada(void * arg __attribute__((unused)))
{
	sleep(1);
	signal(SIGUSR1, sighandler2);
	sleep(2);
	return NULL;
}

static void *
threadb(void * arg __attribute__((unused)))
{
	signal(SIGUSR1, sighandler3);
	sleep(3);
	return NULL;
}


int main()
{
	signal(SIGUSR1, sighandler1);
	pthread_t p1, p2;

	pthread_create(&p1, NULL, threada, NULL);
	pthread_create(&p2, NULL, threadb, NULL);
	sleep(2);
	kill(getpid(), SIGUSR1);
	pthread_join(p2, NULL);
	pthread_join(p1, NULL);

	return 0;
}

