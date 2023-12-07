#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/utsname.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/ctype.h>

static char* whom = "master";
static struct timeval tv;

static void* hello_buf;
static int hello_bufsize = 100;
static int hello_count = 1;
static dev_t hello_dev;
static struct cdev hello_cdev;

module_param(whom, charp, 0); 
/*

flagovi za razlicite tipove

module_param(myShort, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myShort, "A short integer.");

module_param(myInt, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(myInt, "An integer.");

module_param(myLong, long, S_IRUSR);
MODULE_PARM_DESC(myLong, "A long integer.");

module_param(myString, charp, 0000);
MODULE_PARM_DESC(myString, "A character string.");

module_param_array(myIntArray, int, &argc, 0000);
MODULE_PARM_DESC(myIntArray, "An array of integers."); za niz; argc je neka promenljiva tipa int

*/

/*
* Called when a process reads dev file: cat /dev/chardev.
*/

static ssize_t hello_read(struct file *filp, char __user *buff, size_t len, loff_t * off)
{
	/* The acme_buf address corresponds to a device I/O memory area */
	/* of size acme_bufsize, obtained with ioremap() */
	int remaining_size, transfer_size;
	remaining_size = hello_bufsize - (int) (*off); // bytes left to transfer
	if (remaining_size == 0) { /* All read, returning 0 (End Of File) */
		return 0;
	}
	/* Size of this transfer */
	transfer_size = min(remaining_size, (int) len);
	if (copy_to_user(buff, hello_buf + *off, transfer_size)){
		return -EFAULT;
	} else { /* Increase the position in the open file */
		*off += transfer_size;
		return transfer_size;
	}

}

/*
* Called when a process writes to dev file: echo "msg to" > /dev/chardev.
*/
static ssize_t hello_write(struct file *filp, const char __user *buff, size_t len, loff_t * off)
{
	int remaining_bytes;
	/* Number of bytes not written yet in the device */
	remaining_bytes = hello_bufsize - (int) (*off);
	if (len > remaining_bytes) {
		/* Can't write beyond the end of the device */
		return -EIO;
	}
	if (copy_from_user(hello_buf + *off /* to */, buff /* from */, len)) {
		return -EFAULT;
	} else {
		/* Increase the position in the open file */
		*off += len;
		return len;
	}
}

/* File ioctl function. */
static long hello_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	static unsigned int index = 0;
	char* tmp_buf = (char*)hello_buf;
	switch(cmd) {
		case 0:
			for(index = 0; index < hello_bufsize; index++){
				tmp_buf[index] = tolower(tmp_buf[index]);			
			}
			break;	
		case 1:
			for(index = 0; index < hello_bufsize; index++){
				tmp_buf[index] = toupper(tmp_buf[index]);			
			}
			break;
		default:
			pr_info("Error, command %u not supported.\n", cmd);
    	}

    	/* Success. */
	return 0;
}

static const struct file_operations hello_fops =
{
	.read = hello_read,
	.write = hello_write,
	.unlocked_ioctl = hello_ioctl	
};

static int __init hello_init(void)
{
	int err;
	
	do_gettimeofday(&tv);
	pr_alert("Hello %s, you are currently using Linux %s\nseconds:%lu\n", whom, utsname()->release, tv.tv_sec);
	
	
	hello_buf = kmalloc(hello_bufsize, GFP_KERNEL);
	if(!hello_buf)
	{
		err = -ENOMEM;
		goto err_exit;		
	}
	if (alloc_chrdev_region(&hello_dev, 0, hello_count, "hello")) 
	{
		err=-ENODEV;
		goto err_free_buf;
	}
	cdev_init(&hello_cdev, &hello_fops);
	if (cdev_add(&hello_cdev, hello_dev, hello_count)) 
	{
		err=-ENODEV;
		goto err_dev_unregister;
	}
	
	pr_alert("Major-Minor: %d - %d\n", MAJOR(hello_dev), MINOR(hello_dev));
	return 0;

	err_dev_unregister:
	unregister_chrdev_region(hello_dev, hello_count);
	
	err_free_buf:
	kfree(hello_buf);
	
	err_exit:
	return err;
}

static void __exit hello_exit(void)
{
	do_gettimeofday(&tv);
	pr_alert("Goodbye %s!\nseconds:%lu\n", whom, tv.tv_sec);
	
	cdev_del(&hello_cdev);

	unregister_chrdev_region(hello_dev, hello_count);
	kfree(hello_buf);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");

