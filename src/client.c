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
#if defined (HOST_DEV_PROC_DRV)
	int fd_dev = open("/dev/example_dev", O_RDWR);
	if(fd_dev < 0) {
		perror("/dev can't be open");
		return -1;
	}
	printf("/dev: Opened\n");
	const char *wr_buf = "Data from Client";
	printf("Try to write: %s \n", wr_buf);
	(void)write(fd_dev, wr_buf, strlen(wr_buf));
	#define RD_BUF_SIZE (50)
	unsigned char rd_buf[RD_BUF_SIZE] = {};
	(void)read(fd_dev, rd_buf, RD_BUF_SIZE);
	printf("Read data: %s \n", rd_buf);
	close(fd_dev);
	printf("/dev: Closed\n");
#endif

	int fd_proc = open("/proc/example_dev", O_RDWR);
	if(fd_proc < 0) {
		perror("/proc can't be open");
		return -1;
	}
	printf("/proc: Opened\n");
	close(fd_proc);
	printf("/proc: Closed\n");

	FILE *file = fopen("/proc/example_dev", "r+");
	if(file < 0) {
		perror("/proc can't be fopen");
		return -1;
	}
	int fd_tmp = fileno(file);
	printf("/proc: FOpened\n");
	fsync(fd_tmp);
	fclose(file);
	printf("/proc: FClosed\n");

	return 0;
}
