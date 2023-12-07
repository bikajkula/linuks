#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
/* A couple of parameters that can be passed in: how many times we say
hello, and to whom */

static void *local_buf;
static int local_buf_size=10;
static int hello_count=1;
static dev_t hello_dev;
static struct cdev hello_cdev;
static ssize_t hello_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static ssize_t hello_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static char *whom = "world";
module_param(whom, charp, S_IRUGO | S_IWUSR);

static struct file_operations hello_fops =
{
    .owner = THIS_MODULE,
    .read = hello_read,
    .write = hello_write
};

static ssize_t hello_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
	int to_transfer = min(local_buf_size-(int)(*ppos), count);
	if(to_transfer == 0){
		return 0;
	}
	if (copy_from_user(local_buf+(int)(*ppos), buf, to_transfer)){
		return -EFAULT;
	} else {
    	*ppos += to_transfer;
		return to_transfer;
	}
}

static ssize_t hello_read(struct file *file, char __user *buf, size_t count, loff_t *ppos){
	int len = strlen(local_buf+(int)(*ppos));
	int to_transfer = min(len, count);
	if(to_transfer == 0){
		return 0;
	}
	if (copy_to_user(buf, local_buf+(int)(*ppos), to_transfer)){
		return -EFAULT;
	} else {
		*ppos += to_transfer;
		return to_transfer;
	}
}

static int __init hello_init(void)
{
    int err;

    pr_info("Hello, %s\n", whom);
    
    local_buf = kzalloc(local_buf_size, GFP_KERNEL);
    if(!local_buf)
    {
    	err=-ENOMEM;
    	goto err_exit;
    }
    
    if (alloc_chrdev_region(&hello_dev, 0, hello_count, "hello")) {
        err=-ENODEV;
        goto err_free_buff;
    }
    pr_info("Major: %d, Minor:%d\n", MAJOR(hello_dev), MINOR(hello_dev));
    cdev_init(&hello_cdev, &hello_fops);
    if (cdev_add(&hello_cdev, hello_dev, hello_count)) {
        err=-ENODEV;
        goto err_dev_unregister;
    }

    return 0;
    
err_dev_unregister:
    unregister_chrdev_region(hello_dev, hello_count);

err_free_buff:
    kfree(local_buf);
    
err_exit:
    return err;

}
static void __exit hello_exit(void)
{
    pr_alert("Goodbye, cruel %s \n", whom);
    cdev_del(&hello_cdev);
    unregister_chrdev_region(hello_dev, hello_count);
    kfree(local_buf);
}
module_init(hello_init);
module_exit(hello_exit);

