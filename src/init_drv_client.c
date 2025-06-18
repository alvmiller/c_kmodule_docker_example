#include <linux/module.h>
#include <syscall.h>
#include <stdio.h>
#include <errno.h>

#include "example_dev.ko.h"

const char args[] = "\0";

int main(void)
{
    int result = init_module(example_dev_ko, example_dev_ko_len, args);
    if (result != 0)
    {
        printf("Error: %d (%d)\n", result, errno);
        perror("Result errno: ");
        return -1;
    }

    return 0;
}
