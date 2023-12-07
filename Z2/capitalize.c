#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>

MODULE_LICENSE("GPL");

#define MOD_NAME "capital"

static int device_init(void);
static void device_exit(void);
static ssize_t device_read(struct file *flip, char *buff,
				size_t len, loff_t *off);
static ssize_t device_write(struct file *flip, char *buff,
				size_t len, loff_t *off);

static char *buffer;
static int size;
static int message_len;

static int majorNumber;

module_param(size, int, 0);

struct timeval t;
struct tm time;

static const struct file_operations fops = {
	.read = device_read,
	.write = device_write,
};


static int device_init(void)
{
	if (size == 0) {
		pr_info("Module init failed, buffer cannot be 0!\n");
		return -1;
	}

	buffer = kmalloc(size, GFP_KERNEL);
	if (!buffer) {
		pr_info("Module init failed, no free space for the buffer\n");
		return -1;
	}

	majorNumber = register_chrdev(0, MOD_NAME, &fops);
	if (majorNumber < 0) {
		pr_info("Module init failed, cant get major number!\n");
		return -1;
	}

	pr_info("MajorNum: %d\n", majorNumber);

	return 0;
}

static void device_exit(void)
{
	kfree(buffer);
	unregister_chrdev(majorNumber, MOD_NAME);
	pr_info("Module removed\n");
}

static ssize_t device_read(struct file *flip,
			char *buff, size_t len, loff_t *off)
{
	int size;

	size = copy_to_user(buff, buffer, message_len);
	buffer[message_len] = '\0';
	do_gettimeofday(&t);
	time_to_tm(t.tv_sec, 0, &time);
	pr_info("Time when read: %dH:%dmin:%dsec\n",
		time.tm_hour, time.tm_min, time.tm_sec);
	pr_info("Buffer: %s", buffer);

	return 0;
}

static ssize_t device_write(struct file *flip,
			char *buff, size_t len, loff_t *off)
{
	int size;

	int i;

	message_len = len;
	size = copy_from_user(buffer, buff, len);
	pr_info("We got: %d\n", message_len);
	for (i = 0; i < message_len; i++) {
		if (buffer[i] >= 97 && buffer[i] <= 122) { /*a = 96, z = 122*/
			buffer[i] -= 32;
		} else if (buffer[i] >= 65 && buffer[i] <= 90) {
			buffer[i] += 32;
		} else {
		}
	}
	return len;
}

module_init(device_init);
module_exit(device_exit);






