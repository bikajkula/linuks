#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/utsname.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/gfp.h>

static char *whom = "Master";
module_param(whom, charp, 0);

static void *hello_buf;
static int hello_bufsize = 100;
static struct cdev hello_cdev;

//S_IRUGO kao fleg ako 0 nece
module_param(hello_bufsize, int, 0);

struct timeval tv;
static dev_t hello_dev;
static ssize_t hello_read(struct file *file, char __user *buf,
	size_t count, loff_t *ppos);
static ssize_t hello_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos);
static long hello_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int err;

const static struct file_operations hello_fops = {
	.read = hello_read,
	.write = hello_write,
	.unlocked_ioctl = hello_ioctl,
};

static int __init hello_init(void)
{
	pr_alert("Hello %s. You are currently using Linux %s\n",
		whom, utsname()->release);
	do_gettimeofday(&tv);
	pr_alert("Seconds from start: %lu\n", tv.tv_sec);

	hello_buf = kcalloc(hello_bufsize, 1, GFP_KERNEL);
	if (hello_buf == NULL) {
		err = -ENOMEM;
	goto err_exit;
	}
	if (alloc_chrdev_region(&hello_dev, 0, 1, "hello")) {
		err = -ENODEV;
	goto err_free_buf;
	}
	cdev_init(&hello_cdev, &hello_fops);
	if (cdev_add(&hello_cdev, hello_dev, 1)) {
		err = -ENODEV;
		goto err_dev_unregister;
	}
	
	pr_alert("Major-Minor: %d - %d\n", MAJOR(hello_dev), MINOR(hello_dev));
	
	return 0;

err_dev_unregister:
	unregister_chrdev_region(hello_dev, 1);

err_free_buf:
	kfree(hello_buf);
err_exit:
	return err;
}

static void __exit hello_exit(void)
{
	pr_alert("Goodbye %s\n", whom);
	do_gettimeofday(&tv);
	pr_alert("Seconds from start: %lu\n", tv.tv_sec);

	cdev_del(&hello_cdev);
	unregister_chrdev_region(hello_dev, 1);
	kfree(hello_buf);
}

static ssize_t hello_read(struct file *file, char __user *buf,
	size_t count, loff_t *ppos)
{
	int remaining_size, transfer_size;

	pr_alert("Hello from read\n");
	remaining_size = hello_bufsize - (int) (*ppos);
	if (remaining_size == 0)
		return 0;

	transfer_size = min(remaining_size, (int) count);
	if (copy_to_user(buf, hello_buf + *ppos, transfer_size))
		return -EFAULT;

	*ppos += transfer_size;
	return transfer_size;

}

static ssize_t hello_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int remaining_bytes;

	pr_alert("Hello from write\n");

	remaining_bytes = hello_bufsize - (*ppos);
	if (count > remaining_bytes)
		return -EIO;

	if (copy_from_user(hello_buf + *ppos, buf, count))
		return -EFAULT;

	*ppos += count;
	return count;
}

static long hello_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int i;
	char* buff = (char*)hello_buf;

	switch (cmd) {
	case 0:
		pr_alert("ioctl command 0\n");
		for(i = 0; i < hello_bufsize; i++){
			if(buff[i] >= 'a' && buff[i]<='z')
				buff[i] = buff[i]-32;
		}
		break;
	case 1:
		pr_alert("ioctl command 1\n");
		for(i = 0; i < hello_bufsize; i++){
			if(buff[i] >= 'A' && buff[i]<='Z')
				buff[i] = buff[i]+32;
		}
		break;
	default:
		return -ENOTTY;
	}
	return 0;
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");


