#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <signal.h>



#define XSZ	(8192)
static int i = 0;

int main()
{
	int fd = open("/etc/passwd", O_RDONLY);
	assert(fd > 0);

	void * ptr = mmap(NULL, XSZ, PROT_READ | PROT_WRITE, 
			MAP_PRIVATE, fd, 0);
	assert(ptr != NULL);
	close(fd);

	pid_t pid = fork();
	assert(pid >= 0);
	if (pid == 0) {
		/* child */
		printf("child pid=%d\n", getpid());
		exit(0);
	} else {
		/* parent */
		printf("parent: child pid=%d\n", pid);
		exit(0);
	}


	return 0;
}

