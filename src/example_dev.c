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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("user1");
MODULE_DESCRIPTION("kernel-docker-test-01");
MODULE_VERSION("0.001");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#endif

//------------------------------------------------------------------------------

static int drv_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;
	pr_info("Device open()\n");
	return 0;
}

static int drv_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;
	pr_info("Device release()\n");
	return 0;
}

static ssize_t drv_read(
	struct file *file,
	char __user *buf,
	size_t count,
	loff_t *offset)
{
	(void)file;
	(void)offset;
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
	(void)file;
	(void)offset;
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

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

static const struct proc_ops proc_fops = {
	.proc_open    = drv_open,
	.proc_release = drv_release,
	.proc_read    = drv_read,
	.proc_write   = drv_write,
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

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

#define MAX_DEV (2)
dev_t dev;
static struct class *dev_class = NULL;
static int dev_major = 0;
struct cdev drv_cdev[MAX_DEV];

static const struct file_operations dev_fops = {
	.owner   = THIS_MODULE,
	.open    = drv_open,
	.release = drv_release,
	.read    = drv_read,
	.write   = drv_write,
};

static int dev_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	(void)dev;
	add_uevent_var(env, "DEVMODE=%#o", 0777);
	return 0;
}

static int init_dev(void)
{
	if((alloc_chrdev_region(&dev, 0, MAX_DEV, "example_dev_region")) < 0) {
		pr_info("Cannot allocate major number\n");
		return -1;
	}

	dev_major = MAJOR(dev);

	//if(IS_ERR(dev_class = class_create(THIS_MODULE, "example_dev"))) {
	if(IS_ERR(dev_class = class_create("example_dev_class"))) {
		pr_info("Cannot create the struct class\n");
		goto err_class;
	}
	dev_class->dev_uevent = dev_uevent;

	for (int i = 0; i < MAX_DEV; ++i) {
		cdev_init(&drv_cdev[i], &dev_fops);
		drv_cdev[i].owner = THIS_MODULE;

		if((cdev_add(&drv_cdev[i], MKDEV(dev_major, i), 1)) < 0) {
			pr_info("Cannot add the device to the system (%d)\n", i);
			goto err_device;
		}

		if (IS_ERR(device_create(dev_class, NULL, MKDEV(dev_major, i), NULL, "example_dev-%d", i))) {
			pr_info("Cannot create the Device-%d\n", i);
			goto err_device;
		}
	}

	printk(KERN_ALERT "Inserted /dev\n");
	return 0;

err_device:
	class_destroy(dev_class);
err_class:
	unregister_chrdev_region(dev, MAX_DEV);
	return -1;
}

static void destroy_dev(void)
{
	for (int i = 0; i < MAX_DEV; ++i) {
        	device_destroy(dev_class, MKDEV(dev_major, i));
    	}
	class_destroy(dev_class);
	for (int i = 0; i < MAX_DEV; ++i) {
        	cdev_del(&drv_cdev[i]);
    	}
	unregister_chrdev_region(MKDEV(dev_major, 0), MAX_DEV);
	printk(KERN_ALERT "Destroyed /dev\n");
}

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

module_init(example_init);
module_exit(example_exit);
