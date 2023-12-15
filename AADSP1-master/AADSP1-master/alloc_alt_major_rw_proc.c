#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");

static int jzr_major;
static char *memory;
static size_t memory_size;

static struct proc_dir_entry *proc;

static int buffer_size = 522;
module_param(buffer_size, int, 0);

static ssize_t jzr_read(struct file *filp, char *buff, size_t len, loff_t *off) {
	/* The acme_buf address corresponds to a device I/O memory area */
	/* of size acme_bufsize, obtained with ioremap() */
	int remaining_size, transfer_size;
	
	remaining_size = memory_size - (int) (*off); // bytes left to transfer
	if (remaining_size == 0) { /* All read, returning 0 (End Of File) */
		return 0;
	}
	
	/* Size of this transfer */
	transfer_size = min(remaining_size, (int) len);
	if (copy_to_user(buff, memory + *off, transfer_size)){
		return -EFAULT;
	}
	
	*off += transfer_size;
	return transfer_size;
	
}

static ssize_t jzr_write(struct file *filp, const char *buff, size_t len, loff_t *off) {
	int remaining_bytes;
	remaining_bytes = buffer_size - (*off);
	
	if (len > remaining_bytes) {
		return -EIO;
	}
	
	if (copy_from_user(memory + *off, buff, len) != 0) {
		return -EFAULT;
	}
	
	*off += len;
	memory_size = len;
	
	return len;
}

static ssize_t proc_read(struct file *filp, char *buff, size_t len, loff_t *off) {
	if (*off == 0) {
		int length = snprintf(buff, len, "total: %d\nused: %zu\n", buffer_size, memory_size);
		*off += length;
		return length;
	}
	else {
		return 0;
	}
}

static ssize_t proc_write(struct file *filp, const char *buff, size_t len, loff_t *off) {
	// Po P.S.-u:
	// 1. prvo kopirati buff u neki interni buffer sa copy_from_user
	// 2. proveriti da li je sigurno validan int, ako ne treba da se vrati greska
	// 3. krealloc moze da pukne, treba proveriti da li je vratio NULL i ako jeste treba da se vrati greska
	int length = sscanf(buff, "%d", &buffer_size);
	
	memory = krealloc(memory, buffer_size, GFP_KERNEL);
	
	if (buffer_size < memory_size) {
		memory_size = buffer_size;
	}
	
	*off = len;
	return len;
}


static const struct file_operations jzr_fops =
{
	.read = jzr_read,
	.write = jzr_write
};

static const struct file_operations proc_fops =
{
	.read = proc_read,
	.write = proc_write
};

static int __init jzr_init(void)
{
    int result = -1;

    printk(KERN_INFO "Inserting jzr module\n");

    result = register_chrdev(0, "jzr", &jzr_fops);
    if (result < 0)
    {
        printk(KERN_INFO "jzr: cannot obtain major number, ERROR %d\n", result);
        return result;
    }

    jzr_major = result;
    printk(KERN_INFO "jzr major number is %d\n", jzr_major);

	memory = kmalloc(buffer_size, GFP_KERNEL);
	if (memory == NULL)
	{	
		printk(KERN_INFO "jzr: cannot allocate memory\n");
		return -ENOMEM;
	}

	printk("Allocated buffer of size %d\n", buffer_size);

	proc = proc_create("name", 0, NULL, &proc_fops);

    return 0;
}

static void __exit jzr_exit(void)
{
    printk(KERN_INFO "Removing jzr module\n");

	proc_remove(proc);

    kfree(memory);

    unregister_chrdev(jzr_major, "jzr");
}

module_init(jzr_init);
module_exit(jzr_exit);

