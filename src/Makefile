obj-m := example_dev.o
 
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: default
 
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

test:
	sudo dmesg -C
	sudo insmod example_dev.ko
	sleep 1
	sudo rmmod example_dev.ko
	sudo dmesg
 
clean:
	make -C $(KERNELDIR)  M=$(PWD) clean
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.ko.h ./client ./client_host ./init_drv_client
