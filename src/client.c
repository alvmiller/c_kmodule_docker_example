#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

int main()
{
	int fd0 = open("/proc/example_dev", O_RDWR);
	if(fd0 < 0) {
		perror("Can't open 0");
		return -1;
	}
	printf("Opened 0\n");
	close(fd0);
	printf("Closed 0\n");

	FILE *f = fopen("/proc/example_dev", "r+");
	if(f < 0) {
		perror("Can't open 1");
		return -1;
	}
	int fd1 = fileno(f);
	printf("Opened 1\n");
	fsync(fd1);
	fclose(f);
	printf("Closed 1\n");

	return 0;
}
