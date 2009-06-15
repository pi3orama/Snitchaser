#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

static int global_data = 0;

int main(int argc, char * argv[])
{
	printf("argv[0] is \"%s\", at %p\n", argv[0], &argv[0]);
	/* check for breakpoint */
	printf("addr of main = %p\n", main);
	printf("dup 8 bytes from main: ");
	for (int i = 0; i < 8; i++)
		printf("0x%x ", ((unsigned char*)main)[i]);
	printf("\n");

	printf("I am target\n");
	printf("argc=%d, argv[0]=%s\n",
			argc, argv[0]);
	int i = 1;
	while(1) {
		int c = 'x';
//		c = getchar();
		sleep(1);
		printf("%d, %c, global_data=%d\n", i++, c, global_data);
	}
	return 0;
}

