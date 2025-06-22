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
#include <linux/utsname.h>
#include <linux/path.h>
#include <linux/mount.h>
#include <linux/dcache.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mutex.h>

//----------------------------------------------------------------------------//

MODULE_LICENSE("GPL");
MODULE_AUTHOR("user1");
MODULE_DESCRIPTION("kernel-docker-test-01");
MODULE_VERSION("0.001");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#endif

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

static void print_module_name(void)
{
	if (THIS_MODULE != NULL) {
		printk("\tCurrent module name: %s\n", THIS_MODULE->name);
		printk("\tCurrent module version = %s\n", THIS_MODULE->version);
	}
	if (THIS_MODULE == NULL) {
		printk("\tCurrent module name: %s\n", module_name(THIS_MODULE));
	}
	printk("\thostname: %s\n", utsname()->nodename);
	printk("\tInitialize module: %s\n", KBUILD_MODNAME);
	return;
}

static void print_file_name(struct file * const file)
{
	printk("\tFile: %s\n", file->f_path.dentry->d_name.name);
	return;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

static DEFINE_MUTEX(val_lock);

enum Val_Operation {
  INC_VAL_OP,
  DEC_VAL_OP,
  GET_VAL_OP
};

static inline long int do_val_operation(
	enum Val_Operation op, struct file *file, uint8_t *res_val)
{
	static uint8_t value = 0;
	long int ret = -1;

	pr_info("\tDevice read()\n");
	print_module_name();
	print_file_name(file);

	mutex_lock(&val_lock);
	uint8_t tmp = 0;
	switch (op) {
	case INC_VAL_OP:
		if ((value == U8_MAX)
			|| __builtin_add_overflow(value, 1, &tmp)) {
			return -EOVERFLOW;
		}
		msleep(100);
		value = tmp;
		ret = 0;
		break;
	case DEC_VAL_OP:
		if ((value == 0) || __builtin_sub_overflow(value, 1, &tmp)) {
			return -EOVERFLOW;
		}
		msleep(150);
		value = tmp;
		ret = 0;
		break;
	case GET_VAL_OP:
		if (sizeof(long int) <= sizeof(*res_val)) {
			return -EOVERFLOW;
		}
		msleep(200);
		*res_val = value;
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&val_lock);

	return ret;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

#define DRV_NAME "example_dev"

static int drv_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;
	pr_info("\tDevice open()\n");
	print_module_name();
	return 0;
}

static int drv_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;
	print_module_name();
	pr_info("\tDevice release()\n");
	return 0;
}

static ssize_t drv_read(
	struct file *file,
	char __user *buf,
	size_t count,
	loff_t *offset)
{
	(void)file;
	pr_info("\tDevice read()\n");
	print_module_name();
	print_file_name(file);
	
	if (buf == NULL || offset == NULL) {
		pr_err("\tDevice read(): Bad parameters!\n");
		return -EINVAL;
	}
	if (count == 0) {
		pr_err("\tDevice read(): Action is not needed!\n");
		return 0;
	}
    
	uint8_t *data = "Hello from the kernel world.";
	size_t datalen = strlen(data) + 1;

	if (count > datalen) {
		count = datalen;
	}

	if (copy_to_user(buf, data, count)) {
		return -EFAULT;
	}

	*offset += count;
	return count;
}

static ssize_t drv_write(
	struct file *file,
	const char __user *buf,
	size_t count,
	loff_t *offset)
{
	(void)file;
	pr_info("\tDevice write()\n");
	print_module_name();
	print_file_name(file);

	if (buf == NULL || offset == NULL) {
		pr_err("\tDevice write(): Bad parameters!\n");
		return -EINVAL;
	}
	if (count == 0) {
		pr_err("\tDevice write(): Action is not needed!\n");
		return 0;
	}

	#define MAX_DATA_LEN (30)
	size_t maxdatalen = MAX_DATA_LEN;
	uint8_t databuf[MAX_DATA_LEN + 1] = {};
	size_t ncopied = 0;

	if (count < maxdatalen) {
		maxdatalen = count;
	}

	ncopied = copy_from_user(databuf, buf, maxdatalen);
	if (ncopied == 0) {
		printk("\tCopied %zd bytes from the user\n", maxdatalen);
	} else {
		printk("\tCould't copy %zd bytes from the user\n", ncopied);
	}
	databuf[maxdatalen] = 0;

	printk("\tData from the user: %s\n", databuf);
	*offset += count;
	return count;
}

static long int drv_ioctl(
	struct file *file,
	unsigned int ioctl_num,
	unsigned long ioctl_param)
{
	(void)file;
	pr_info("\tDevice ioctl()\n");
	print_module_name();
	print_file_name(file);

	//struct my_device_data *my_data =
	//	(struct my_device_data*) file->private_data;
	//my_ioctl_data mid;

	int ret = -1;
	switch (ioctl_num) {
	case 0x1:
		//if( copy_from_user(&mid, (my_ioctl_data *) arg,
		//	sizeof(my_ioctl_data)) )
		//temp = (char *)ioctl_param;
		//if (copy_to_user((uint32_t*) arg, &value, sizeof(value)))
		printk("\tioctl() called with correct command\n");
		break;
	case 0x10:
		printk("\tioctl() Inc value command called\n");
		ret = do_val_operation(INC_VAL_OP, file, NULL);
		if (ret != 0) {
			return ret;
		}
		break;
	case 0x11:
		printk("\tioctl() Dec value command called\n");
		ret = do_val_operation(DEC_VAL_OP, file, NULL);
		if (ret != 0) {
			return ret;
		}
		break;
	case 0x12: {
			printk("\tioctl() Get value command called\n");
			uint8_t tmp = 0;
			ret = do_val_operation(INC_VAL_OP, file, &tmp);
			if (ret != 0) {
				return ret;
			}
			return (long int)tmp;
		}
	default:
		pr_err("\tioctl() called with unknown command\n");
		return -EINVAL;
	}

	return 0;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

static const struct proc_ops proc_fops = {
	.proc_open    = drv_open,
	.proc_release = drv_release,
	.proc_read    = drv_read,
	.proc_write   = drv_write,
};

static int create_proc(void)
{
	struct proc_dir_entry *proc_file_entry_escape = 
		proc_create(DRV_NAME, 0777, NULL, &proc_fops);
	if (proc_file_entry_escape == NULL) {
		pr_err("Can't register /proc\n");
		return -ENOMEM;
	}
	printk("Registered /proc\n");
	return 0;
}

static void remove_proc(void)
{
	remove_proc_entry(DRV_NAME, NULL);
	printk("Removed /proc\n");
	return;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

#define MAX_DEV (2)
dev_t dev;
static struct class *dev_class = NULL;
static int dev_major = 0;
struct cdev drv_cdev[MAX_DEV];

static const struct file_operations dev_fops = {
	.owner          = THIS_MODULE,
	.open           = drv_open,
	.release        = drv_release,
	.read           = drv_read,
	.write          = drv_write,
	.unlocked_ioctl = drv_ioctl,
};

static int dev_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	(void)dev;
	add_uevent_var(env, "DEVMODE=%#o", 0777);
	return 0;
}

static int init_dev(void)
{
	if((alloc_chrdev_region(&dev, 0, MAX_DEV, DRV_NAME"_region")) < 0) {
		pr_err("Cannot allocate major number\n");
		return -1;
	}

	dev_major = MAJOR(dev);

	if(IS_ERR(dev_class = class_create(DRV_NAME"_class"))) {
		pr_err("Cannot create the struct class\n");
		goto err_class;
	}
	dev_class->dev_uevent = dev_uevent;

	for (int i = 0; i < MAX_DEV; ++i) {
		cdev_init(&drv_cdev[i], &dev_fops);
		drv_cdev[i].owner = THIS_MODULE;

		if((cdev_add(&drv_cdev[i], MKDEV(dev_major, i), 1)) < 0) {
			pr_err("Cannot add the device to the system (%d)\n",
				i);
			goto err_device;
		}

		if (IS_ERR(device_create(
			dev_class, NULL, MKDEV(dev_major, i), NULL,
			DRV_NAME"-%d", i))) {
			pr_err("Cannot create the Device-%d\n", i);
			goto err_device;
		}
	}

	printk("Inserted /dev\n");
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
	printk("Destroyed /dev\n");
	return;
}

//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//

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

	print_module_name();
	return 0;
}

static void __exit example_exit(void)
{
	print_module_name();
	destroy_dev();
	remove_proc();

	printk(KERN_ALERT "Goodbye\n");
	return;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

module_init(example_init);
module_exit(example_exit);

//----------------------------------------------------------------------------//
