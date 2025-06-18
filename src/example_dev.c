#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/proc_fs.h>
#include <linux/umh.h>

/*
https://xcellerator.github.io/posts/docker_escape/
https://github.com/xcellerator/linux_kernel_hacking/tree/079d97b8e0b25e437ea4d5aa2fa4e85feff67583/3_RootkitTechniques/3.8_privileged_container_escaping

Host:
sudo apt-get update && \
sudo apt-get -y install \
	dkms openssl xxd linux-headers-$(uname -r) \
	gcc make build-essential libncurses-dev bison flex libssl-dev libelf-dev dwarves kmod \
	linux-headers-`uname -r`
sudo apt -y install docker.io

Host:
make clean
make
chmod +x script_docker_host.sh
./script_docker_host.sh
--
Docker:
cd /root
dmesg -C
./init_drv_client.c
ls -la /proc/example_dev
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
#define HAVE_PROC_OPS
#endif

static int drv_open(struct inode *inode, struct file *file)
{
	pr_info("Device File Opened...!!!\n");
	return 0;
}

static int drv_release(struct inode *inode, struct file *file)
{
	pr_info("Device File Closed...!!!\n");
	return 0;
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops fops = {
	.proc_open    = drv_open,
	.proc_release = drv_release,
};
#else
static const struct file_operations fops = {
	.owner   = THIS_MODULE,
	.open    = drv_open,
	.release = drv_release,
};
#endif

static int __init hello_init(void)
{
    printk(KERN_ALERT "Hello\n");
    struct proc_dir_entry *proc_file_entry_escape = proc_create("example_dev", 0777, NULL, &fops);
    if( proc_file_entry_escape == NULL ) {
        printk(KERN_ALERT "Can't register /proc\n");
        return -ENOMEM;
    }
    printk(KERN_ALERT "Registered /proc\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_ALERT "Goodbye\n");
    remove_proc_entry("example_dev", NULL);
    printk(KERN_ALERT "Removed /proc\n");
}

module_init(hello_init);
module_exit(hello_exit);
