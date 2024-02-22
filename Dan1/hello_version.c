#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>

#define MESSAGE_MAX 50
#define DEVICE_NAME "Hello_module"
#define CLASS_NAME "ebb"

/* Add your code here */

static int hello_init(void);
static void hello_exit(void);
static ssize_t hello_read(struct file *flip, char *buff,
				size_t len, loff_t *off);
static ssize_t hello_write(struct file *flip, const char *buff,
				size_t len, loff_t *off);
static int hello_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

static char *whom = "Master";
module_param(whom, charp, 0);

struct timeval t_start;
struct timeval t_end;
struct tm time;
static char *message;
static int size_of_message;
static struct class *ebbcharClass;
static struct device *ebbcharDevice;

static int majorNumber;
static int minorNumber;

static const struct file_operations fops = {
	.read = hello_read,
	.write = hello_write,
	.unlocked_ioctl = hello_ioctl
};

static int hello_init(void)
{
	printk(KERN_INFO "Hello %s. You are currently using Linux %s\n", whom, utsname()->release);
	do_gettimeofday(&t_start);

	message = kmalloc(MESSAGE_MAX, GFP_KERNEL);

	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	minorNumber = 0;

	ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
	printk(KERN_INFO "EBBChar: device class registered\n");

	ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, minorNumber), NULL, DEVICE_NAME);
	pr_info("EBBChar: device created\n");

	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "Unloaded hello module\n");
	do_gettimeofday(&t_end);
	time_to_tm(t_end.tv_sec - t_start.tv_sec, 0, &time);
	printk(KERN_INFO "Kernel is running for: %dH:%dmin:%dsec\n", time.tm_hour, time.tm_min, time.tm_sec);
	kfree(message);

	device_destroy(ebbcharClass, MKDEV(majorNumber, minorNumber));
	class_unregister(ebbcharClass);
	class_destroy(ebbcharClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
}

static ssize_t hello_read(struct file *flip, char *buff, size_t len, loff_t *off)
{
	int size;

	size = copy_to_user(buff, message, size_of_message);

	printk(KERN_INFO "Read: %d\n", size_of_message);
	printk(KERN_INFO "Message: %s", message);

	return 0;
}

static ssize_t hello_write(struct file *flip, const char *buff, size_t len, loff_t *off)
{
	int size;
	size_of_message = len;
	size = copy_from_user(message, buff, len);
	printk(KERN_INFO "Write: %d\n", size_of_message);

	return len;
}

static int hello_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	int i;

	switch (cmd) {
	case 0:
		for (i = 0; i < size_of_message; i++)
			message[i] = tolower(message[i]);
		break;
	case 1:
		for (i = 0; i < size_of_message; i++)
			message[i] = toupper(message[i]);
		break;
	default:
		printk(KERN_INFO "Error, please put 0 or 1\n");
	}

	return 0;
}


module_init(hello_init);
module_exit(hello_exit);


MODULE_LICENSE("GPL");
