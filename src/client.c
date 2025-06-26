#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>

int main()
{
#if defined (HOST_DEV_PROC_DRV)
	errno = 0;
	ssize_t ret = -1;

	printf("\n");
	int fd_dev = open("/dev/example_dev-0", O_RDWR);
	if(fd_dev < 0) {
		perror("/dev can't be open");
		return -1;
	}
	printf("/dev: Opened\n");

	const char *wr_buf = "Hello from Client.";
	printf("Try to write: %s \n", wr_buf);
	const ssize_t data_size = strlen(wr_buf);
	ssize_t len = data_size;
	char *buf = wr_buf;
	//(void)write(fd_dev, wr_buf, strlen(wr_buf));
	while (len != 0 && ((ret = write(fd_dev, buf, len)) != data_size)) {
		printf("\tError: Not all data had been written (ret = %zd)\n", ret);
		if (ret == -1) {
			perror("\tGot error");
			if (errno == EINTR) {
				continue;
			}
			if (errno == ENOSPC || errno == EDQUOT) {
				printf("\tError: No Space\n");
				break;
			}
			break;
		}
		len -= ret;
		buf += ret;
		printf("\tError: Not full len wrote\n");
	}

	#define RD_BUF_SIZE (29)
	unsigned char rd_buf[RD_BUF_SIZE] = {};
	//(void)read(fd_dev, rd_buf, RD_BUF_SIZE);
	buf = rd_buf;
	len = RD_BUF_SIZE;
	while (len != 0 && (ret = read(fd_dev, buf, len)) != RD_BUF_SIZE) {
		if (ret == -1) {
			perror("\tGot error");
			if (errno == EINTR) {
				continue;
			}
			break;
		}
		len -= ret;
		buf += ret;
		printf("\tError: Not full len read\n");
	}
	printf("Read data: %s\n", rd_buf);

	//if (ioctl(fd, cmd, buffer) < 0)
	ret = ioctl(fd_dev, 0x01);
	if (ret == -1) {
		perror("\tCannot use ioctl()");
	}
	printf("/dev: ioctl() called\n");

	for (int i = 0; i < 6; ++i) {
		ret = ioctl(fd_dev, 0x10);
		if (ret == -1) {
			perror("\tCannot use Inc ioctl()");
		}
		printf("/dev: Inc ioctl() called (%d)\n", i);
	}

	ret = ioctl(fd_dev, 0x11);
	if (ret == -1) {
		perror("\tCannot use Dec ioctl()");
	}
	printf("/dev: Dec ioctl() called\n");

	ret = ioctl(fd_dev, 0x12);
	if (ret == -1) {
		perror("\tCannot use Get ioctl()");
	}
	printf("/dev: Get ioctl() called (%ld)-(%d)\n", ret, errno);

	ret = ioctl(fd_dev, 0x13);
	if (ret == -1) {
		perror("\tCannot use ioctl()");
	}
	printf("/dev: ioctl() called\n");

	close(fd_dev);
	printf("/dev: Closed\n");
#endif

	printf("\n");
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
	printf("\n");

	return 0;
}
