#include <stdio.h>
#include <stdlib.h>

int main()
{
	void * x = malloc(15);
	printf("%p, %p, %p\n", x,
			*(void**)(0xb7faed7c),
			*(void**)(0xb7faed80));
	free(x);
	return 0;
}

