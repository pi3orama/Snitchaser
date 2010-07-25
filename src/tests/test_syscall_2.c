#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <assert.h>

static void
dup_mem(void * ptr, size_t size)
{
	int i;
	for (i = 0; i < (int)size; i++) {
		printf("%02x ", ((uint8_t*)ptr)[i]);
		if ((i + 1) % 10 == 0)
			printf("\n");
	}
	printf("\n");
}

int main()
{
	struct stat * ptr = malloc(sizeof(struct stat));
	assert(ptr != NULL);

	printf("ptr = %p\n", ptr);
	stat("/etc/passwd", ptr);

	dup_mem(ptr, sizeof(struct stat));


	char * ptr2 = calloc(ptr->st_size + 1, 1);
	assert(ptr2 != NULL);

	FILE * passwd = fopen("/etc/passwd", "r");
	assert(passwd != NULL);

	int err = fread(ptr2, ptr->st_size, 1, passwd);
	assert(err == 1);
	printf("%s\n", ptr2);

	free(ptr2);
	free(ptr);


	return 0;
}

// vim:ts=4:sw=4

