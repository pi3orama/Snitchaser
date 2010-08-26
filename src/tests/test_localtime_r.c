#include <time.h>
#include <stdio.h>

int main()
{
	struct tm xxx;
	time_t time = 1282829047;
	localtime_r(&time, &xxx);
	printf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %ld, %s\n",
			xxx.tm_sec,
			xxx.tm_min,
			xxx.tm_hour,
			xxx.tm_mday,
			xxx.tm_mon,
			xxx.tm_year,
			xxx.tm_wday,
			xxx.tm_yday,
			xxx.tm_isdst,
			xxx.tm_gmtoff,
			xxx.tm_zone);
	return 0;
}

