#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main()
{

	printf("program runs normally\n");
	int i = 0;
	int s = 0;
	while(i < 10000000) {
		s += i++;
		if (i % 100000 == 0)
			printf("i = %d\n", i);
	}
	printf("end with %d\n", s);
	return 0;
}

// vim:ts=4:sw=4

