#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

static void
handler(int num)
{
	printf("signal %d raised\n", num);
	return;
}

int main()
{
	struct sigaction act, oact;
	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	act.sa_restorer = NULL;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGINT);

	int err = sigaction(SIGRTMIN + 3, &act, &oact);

	assert(err == 0);

	printf("oact.sa_handler=%p\n", oact.sa_handler);
	printf("oact.sa_flags=0x%x\n", oact.sa_flags);
	printf("oact.sa_restorer=%p\n", oact.sa_restorer);
	printf("oact.sa_mask[0]=0x%x\n", ((int*)(&oact.sa_mask))[0]);
	printf("oact.sa_mask[1]=0x%x\n", ((int*)(&oact.sa_mask))[1]);

	printf("\n");
	err = sigaction(SIGRTMIN + 3, NULL, &oact);
	assert(err == 0);

	printf("oact.sa_handler=%p\n", oact.sa_handler);
	printf("oact.sa_flags=0x%x\n", oact.sa_flags);
	printf("oact.sa_restorer=%p\n", oact.sa_restorer);
	printf("oact.sa_mask[0]=0x%x\n", ((int*)(&oact.sa_mask))[0]);
	printf("oact.sa_mask[1]=0x%x\n", ((int*)(&oact.sa_mask))[1]);
	return 0;
}

// vim:ts=4:sw=4

