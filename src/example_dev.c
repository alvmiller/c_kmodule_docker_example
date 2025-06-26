#include <linux/cdev.h>
#include <linux/dcache.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/mutex.h>
#include <linux/path.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/umh.h>
#include <linux/utsname.h>
#include <linux/version.h>

//----------------------------------------------------------------------------//

MODULE_LICENSE("GPL");
MODULE_AUTHOR("user1");
MODULE_DESCRIPTION("kernel-docker-test-01");
MODULE_VERSION("0.001");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#endif

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

#define GET_CURRENT_FILE_FULL_PATH(_file_)				\
	({								\
		const struct file *const __tmp_file__ = NULL;		\
		typeof(__tmp_file__) _x = (__tmp_file__);		\
		typeof(_file_) _y = (_file_);				\
		(void)(&_x == &_y);					\
		_file_->f_path.dentry->d_name.name;			\
	})

static void print_module_name(void)
{
	if (THIS_MODULE != NULL) {
		printk("\tCurrent module name: %s\n", THIS_MODULE->name);
		printk("\tCurrent module version = %s\n",
			THIS_MODULE->version);
	}
	if (THIS_MODULE == NULL) {
		printk("\tCurrent module name: %s\n",
			module_name(THIS_MODULE));
	}
	printk("\thostname: %s\n", utsname()->nodename);
	printk("\tInitialize module: %s\n", KBUILD_MODNAME);
	return;
}

static void print_file_name(const struct file * const file)
{
	printk("\tFile: %s\n", GET_CURRENT_FILE_FULL_PATH(file));
	return;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

#define FILE_NAME_MAX (100)
struct file_name_full_path_s {
    //char *file_full_path_p;
    char file_full_path[FILE_NAME_MAX];
    struct list_head list;
};

static LIST_HEAD(file_full_path_list_journal);

static int add_file_name_node(const struct file * const file)
{
	if (file == NULL) {
		return -EINVAL;
	}

	struct file_name_full_path_s *new_item = kzalloc(
		sizeof(*new_item), GFP_ATOMIC);
	if (new_item == NULL) {
		return -ENOMEM;
	}

	const size_t dst_size = sizeof(new_item->file_full_path);
	size_t len = strscpy(
		new_item->file_full_path,
		GET_CURRENT_FILE_FULL_PATH(file),
		dst_size);
	if (len == dst_size) {
		kfree(new_item);
		return -ENOMEM;
	}
	new_item->file_full_path[len + 1] = '\0';

	list_add_tail(&new_item->list, &file_full_path_list_journal);

	return 0;
}

static void clear_all_file_name_nodes(void)
{
	struct file_name_full_path_s *item = NULL;
	struct file_name_full_path_s *tmp = NULL;

	list_for_each_entry_safe(
		item,
		tmp,
		&file_full_path_list_journal,
		list) {
		list_del(&item->list);
		kfree(item);
	}

	return;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

// atomic_t value_lock;
static DEFINE_MUTEX(value_lock);

enum Val_Operation {
  INC_VAL_OP = 0x10,
  DEC_VAL_OP = 0x11,
  GET_VAL_OP = 0x12
};

static inline long int do_val_operation(
	enum Val_Operation op,
	const struct file * const file,
	uint8_t *res_val)
{
	static uint8_t value = 0;
	long int ret = -1;

	if (file == NULL) {
		return -EINVAL;
	}
	WARN_ON(!file);
	//WARN_ON(!!file);

	pr_info("\tDevice for ioctl() function\n");
	print_module_name();
	print_file_name(file);

	mutex_lock(&value_lock);
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
		if (res_val == NULL) {
			return -EINVAL;
		}
		msleep(200);
		*res_val = value;
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&value_lock);

	return ret;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

static struct task_struct *ext_thread0 = NULL;

static int thread_function0(void *pv)
{
	(void)pv;

	printk("\t\tThread function...\n");
	while(!kthread_should_stop()) {
		msleep(100);
	}

	return 0;
}

static int start_ext_thread(void);
static int start_ext_thread(void)
{
	ext_thread0 = kthread_run(thread_function0, NULL, "ext-thread0");
	if(ext_thread0 == NULL) {
		pr_err("Cannot create kthread\n");
		return -EBADE;
	}

	printk("Run ext_thread0\n");
	return 0;
}

void stop_ext_thread(void);
void stop_ext_thread(void)
{
	if (ext_thread0 != NULL) {
		int res = kthread_stop(ext_thread0);
		if (res != 0) {
			pr_err("Cannot stop ext_thread0\n");
		} else {
			printk("Stopped ext_thread0\n");
		}
	}

	return;
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

	//struct my_data *my_data = (struct my_data*)file->private_data;
	//if (copy_from_user(&mid, (my_data *)arg,sizeof(my_data)))
	//if (copy_to_user((uint32_t*) arg, &value, sizeof(value)))

	int ret = -1;
	switch (ioctl_num) {
	case 0x1:
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
			ret = do_val_operation(GET_VAL_OP, file, &tmp);
			if (ret != 0) {
				return ret;
			}
			return (long int)tmp;
		}
/*
	case 0x13: {
			printk("\tioctl() Use kthread0\n");
			ret = start_ext_thread();
			if (ret != 0) {
				return ret;
			}
			msleep(400);
			stop_ext_thread();
			break;
		}
*/
	default:
		pr_err("\tioctl() called with unknown command\n");
		return -EINVAL;
	}

	if (add_file_name_node(file) != 0) {
		pr_err("\tioctl() can't add call to list\n");
		; // No Need Real Action
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

static struct task_struct *signal_thread = NULL;

static void allow_signals(void)
{
	allow_signal(SIGTERM);
}

static int signal_thread_fn(void *arg)
{
	(void)arg;

	printk("\t\tSignal kthread function...\n");
	while (!kthread_should_stop()) {
		if (signal_pending(current)) {
			printk("Pending signal SIGTERM in thread\n");
		}
		msleep(100);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

static int start_signal_thread(void);
static int start_signal_thread(void)
{
	signal_thread = kthread_run(signal_thread_fn, NULL, "signal-thread");
	if(signal_thread == NULL) {
		pr_err("Cannot create signal kthread\n");
		return -EBADE;
	}

	printk("Run signal_thread\n");
	return 0;
}

void stop_signal_thread(void);
void stop_signal_thread(void)
{
	if (signal_thread != NULL) {
		int res = kthread_stop(signal_thread);
		if (res != 0) {
			pr_err("Cannot stop signal_thread\n");
		} else {
			printk("Stopped signal_thread\n");
		}
	}

	return;
}

//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

static int __init example_init(void)
{
	printk(KERN_ALERT "Hello\n");

	int res = -1;

	res = create_proc();
	if (res != 0) {
		return res;
	}

	res = init_dev();
	if (res != 0) {
		goto destroy_proc;
	}

	res = start_ext_thread();
	if (res != 0) {
		goto destroy_dev;
	}

	allow_signals();
	res = start_signal_thread();
	if (res != 0) {
		goto stop_ext_thread;
	}

	print_module_name();
	return 0;

stop_ext_thread:
	stop_ext_thread();
destroy_dev:
	destroy_dev();
destroy_proc:
	remove_proc();

	return res;
}

static void __exit example_exit(void)
{
	print_module_name();

	clear_all_file_name_nodes();
	stop_signal_thread();
	stop_ext_thread();
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
