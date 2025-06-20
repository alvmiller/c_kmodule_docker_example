#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/proc_fs.h>
#include <linux/umh.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/err.h>

/*
https://xcellerator.github.io/posts/docker_escape/
https://github.com/xcellerator/linux_kernel_hacking/tree/079d97b8e0b25e437ea4d5aa2fa4e85feff67583/3_RootkitTechniques/3.8_privileged_container_escaping

https://olegkutkov.me/2018/03/14/simple-linux-character-device-driver/
https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html
https://github.com/ichergui/char-device
https://flusp.ime.usp.br/kernel/char-drivers-intro/
https://dev.to/bytehackr/unlocking-the-power-of-linux-device-drivers-1llh
https://www.oreilly.com/library/view/linux-device-drivers/0596005903/ch03.html
https://freebsdfoundation.org/our-work/journal/browser-based-edition/kernel-development/character-device-driver-tutorial/
https://docs.oracle.com/cd/E19683-01/806-5222/character-21002/index.html
https://linuxjourney.com/lesson/dev-directory


Host:
sudo apt-get update && \
sudo apt-get -y install \
	dkms openssl xxd linux-headers-$(uname -r) \
	gcc make build-essential libncurses-dev bison flex libssl-dev libelf-dev dwarves kmod \
	linux-headers-`uname -r`
sudo apt -y install docker.io

Host:
make clean
chmod +x script_docker_host.sh
./script_docker_host.sh
//sudo ./script_docker_host.sh
sudo dmesg -C
sudo insmod example_dev.ko
sudo ./client_host
sudo su
echo Test0 >/dev/example_dev
cat /dev/example_dev
exit
head -c29 /dev/example_dev
echo "Hello from the user" > /dev/example_dev
sudo tail -n5 /var/log/messages
sudo rmmod example_dev
sudo dmesg
sleep 2
#sudo docker run \
#	-v /dev/:/root/dev_ex/:rw \
#	...
sudo docker run \
	-it \
	--privileged --cap-add SYS_MODULE \
	--hostname docker \
	--mount "type=bind,src=$PWD,dst=/root" \
	ubuntu
--
Docker:
cd /root
dmesg -C
./init_drv_client.c
ls -la /proc/example_dev
ls -la /dev/example_dev
./client
dmesg
exit
--
Host:
ls -la /dev/example_dev
lsmod | grep example_dev
sudo rmmod example_dev
make clean
*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("user1");
MODULE_DESCRIPTION("kernel-docker-test-01");
MODULE_VERSION("0.001");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#endif

dev_t dev;
static struct class *dev_class;
static struct cdev drv_cdev;

static int drv_open(struct inode *inode, struct file *file)
{
	pr_info("Device open()\n");
	return 0;
}

static int drv_release(struct inode *inode, struct file *file)
{
	pr_info("Device release()\n");
	return 0;
}

static ssize_t drv_read(
	struct file *file,
	char __user *buf,
	size_t count,
	loff_t *offset)
{
	printk("Device read()\n");
    
	uint8_t *data = "Hello from the kernel world!\n";
	size_t datalen = strlen(data);

	if (count > datalen) {
		count = datalen;
	}

	if (copy_to_user(buf, data, count)) {
		return -EFAULT;
	}

	return count;
}

static ssize_t drv_write(
	struct file *file,
	const char __user *buf,
	size_t count,
	loff_t *offset)
{
	printk("Device write()\n");

	#define MAX_DATA_LEN (30)
	size_t maxdatalen = MAX_DATA_LEN;
	uint8_t databuf[MAX_DATA_LEN];
	size_t ncopied = 0;

	if (count < maxdatalen) {
		maxdatalen = count;
	}

	ncopied = copy_from_user(databuf, buf, maxdatalen);
	if (ncopied == 0) {
		printk("Copied %zd bytes from the user\n", maxdatalen);
	} else {
		printk("Could't copy %zd bytes from the user\n", ncopied);
	}
	databuf[maxdatalen] = 0;

	printk("Data from the user: %s\n", databuf);
	return count;
}

static const struct proc_ops proc_fops = {
	.proc_open    = drv_open,
	.proc_release = drv_release,
	.proc_read    = drv_read,
	.proc_write   = drv_write,
};

static const struct file_operations dev_fops = {
	.owner   = THIS_MODULE,
	.open    = drv_open,
	.release = drv_release,
	.read    = drv_read,
	.write   = drv_write,
};

static int create_proc(void)
{
	struct proc_dir_entry *proc_file_entry_escape = 
		proc_create("example_dev", 0777, NULL, &proc_fops);
	if( proc_file_entry_escape == NULL ) {
		printk(KERN_ALERT "Can't register /proc\n");
		return -ENOMEM;
	}
	printk(KERN_ALERT "Registered /proc\n");
	return 0;
}

static void remove_proc(void)
{
	remove_proc_entry("example_dev", NULL);
	printk(KERN_ALERT "Removed /proc\n");
	return;
}

static int init_dev(void)
{
	if((alloc_chrdev_region(&dev, 0, 1, "example_dev_region")) < 0) {
		pr_info("Cannot allocate major number\n");
		return -1;
	}

	cdev_init(&drv_cdev, &dev_fops);

	if((cdev_add(&drv_cdev, dev, 1)) < 0){
		pr_info("Cannot add the device to the system\n");
		goto err_class;
	}

	//if(IS_ERR(dev_class = class_create(THIS_MODULE, "example_dev"))) {
	if(IS_ERR(dev_class = class_create("example_dev_class"))){
		pr_info("Cannot create the struct class\n");
		goto err_class;
	}

	if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "example_dev"))) {
		pr_info("Cannot create the Device\n");
		goto err_device;
	}

	printk(KERN_ALERT "Inserted /dev\n");
	return 0;

err_device:
	class_destroy(dev_class);
err_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static void destroy_dev(void)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&drv_cdev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_ALERT "Destroyed /dev\n");
}

static int __init example_init(void)
{
	printk(KERN_ALERT "Hello\n");

	int res = create_proc();
	if (res != 0) {
		return res;
	}

	res = init_dev();
	if (res != 0) {
		return res;
	}

	return 0;
}

static void __exit example_exit(void)
{
	printk(KERN_ALERT "Goodbye\n");

	remove_proc();
	destroy_dev();
}

module_init(example_init);
module_exit(example_exit);
