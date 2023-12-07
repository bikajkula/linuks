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

#define MESSAGE_MAX 100
#define DEVICE_NAME "misra_mixer" 
#define CLASS_NAME  "ebb"

/* ---------------- Module properties ---------------- */
MODULE_LICENSE("GPL");

/* Declaration of led driver functions */
static int device_init(void);
static void device_exit(void);
static ssize_t device_read(struct file *filp, char *buff, size_t len, loff_t * off);
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off);
static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static char* ime;

struct timeval t_start;
struct timeval t_end;
struct tm time;
static char* message;
static short size_of_message = 0;
static struct class* ebbcharClass = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

static int majorNumber;
static int minorNumber = 0;

module_param(ime,charp,0000);
MODULE_PARM_DESC(ime,"naziv korisnika");

/*
* Structure holds pointers to functions defined by the

* driver that perform various operations on the device.
*/
static const struct file_operations fops = 
{
	.read       =   device_read,
	.write      =   device_write,
	.unlocked_ioctl = device_ioctl
};

static int device_init(void)
{
	pr_info("Hello %s. You are currently using Linux %s.\n", ime,utsname()->release);
	do_gettimeofday(&t_start);
	message = kmalloc(MESSAGE_MAX, GFP_KERNEL);

 	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	
	// Register the device class
   	ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   
	printk(KERN_INFO "EBBChar: device class registered correctly\n");
 
   	// Register the device driver
   	ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, minorNumber), NULL, DEVICE_NAME);
   	
   	printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized

	return 0;
}

static void device_exit(void)
{
	pr_info("Hello %s. You are currently using Linux %s.\n", ime,utsname()->release);
	do_gettimeofday(&t_end);
	time_to_tm(t_end.tv_sec - t_start.tv_sec, 0, &time);
	printk("Kernel is runnin for: %dH:%dmin:%dsec\n", time.tm_hour, time.tm_min, time.tm_sec);
	kfree(message);

	device_destroy(ebbcharClass, MKDEV(majorNumber, minorNumber));     // remove the device
	class_unregister(ebbcharClass);                          // unregister the device class
	class_destroy(ebbcharClass);                             // remove the device class
	unregister_chrdev(majorNumber, DEVICE_NAME);  
}

/*
* Called when a process writes to dev file: cat /dev/chardev.
*/
static ssize_t device_read(struct file *filp, char *buff, size_t len, loff_t * off)
{
	int size;	
	size = copy_to_user(buff, message, size_of_message);
	pr_info("READ FUNC : %d\n", size_of_message);
	pr_info("msg: %s", message);

	return 0;
}


/*
* Called when a process writes to dev file: echo "msg to" > /dev/chardev.
*/
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	int size;	
	size_of_message = len; 
	size = copy_from_user(message, buff, len);
	pr_info("WRITE FUNC : %d\n", size_of_message);
 
	return len;
}

/* File ioctl function. */
static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int i;
		
	switch(cmd) {
		case 0:
			for(i=0; i<size_of_message; i++)
			{
				message[i] = tolower(message[i]);
			}
			break;
		case 1:
			for(i=0; i<size_of_message; i++)
			{
				message[i] = toupper(message[i]);
			}
			break;
	default:
		pr_info("Error, command not supported.\n");
    }

    /* Success. */
	return 0;
}

/* Declaration of the init and exit functions. */
module_init(device_init);
module_exit(device_exit);




