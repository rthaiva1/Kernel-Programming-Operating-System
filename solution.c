#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
static int lines;
static int r_status;
static char **pipe;
static int read_line;
static int write_line;
struct semaphore available;
struct semaphore filled;
struct semaphore flag;
static int i = 0;
module_param(lines,int,0);
MODULE_PARM_DESC(lines,"Number of Lines");
static ssize_t my_read(struct file *file,char __user* out, size_t size,loff_t * off)//Read function
{
	ssize_t bytes_notcopied = 0,bytes_copied = 0;
	if(down_interruptible(&filled) == 0)
	{
		if(down_interruptible(&flag) == 0)
		{
		if(read_line>lines)
		{
			read_line = 1;
		}
		bytes_notcopied = copy_to_user(out,pipe[read_line-1],strlen(pipe[read_line-1])+1);
		bytes_copied = strlen(pipe[read_line-1]);
		read_line++;
		up(&flag);
		up(&available);
		}
		else
		{
			printk(KERN_ALERT "Lock not acquired");
		}
	}
	else
	{
		printk(KERN_ALERT "Waiting for write\n");
	}
	return bytes_copied;

}
static ssize_t my_write(struct file *filep,const char *buff, size_t size,loff_t * off) //Write Function
{		
	ssize_t bytes_notcopied = 0,bytes_copied = 0;
	if(down_interruptible(&available) == 0)
	{
		if(down_interruptible(&flag) == 0)
		{
			if(write_line>lines)
			{
			write_line = 1;
			}
	bytes_notcopied = copy_from_user(pipe[write_line-1],buff,size);
	bytes_copied = strlen(pipe[write_line-1]);
	write_line++;
	up(&filled);
	up(&flag);
		}
		else
		{
			printk(KERN_ALERT "Lock not acquired");
		}
	}
	else
	{
		printk(KERN_ALERT "Waiting for Read");
	}
	return bytes_copied;
}
static int my_open(struct inode *inode,struct file *file) //Open Device
{
	printk(KERN_ALERT "Open mydevice\n");
	return 0;
}
static int my_close(struct inode *inode,struct file *file) //Close Device
{
	printk(KERN_ALERT "Close mydevice\n");
	return 0;
}
static struct file_operations my_fops ={
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write,
	.llseek = noop_llseek
};
static struct miscdevice my_misc_device={
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mydevice",
	.fops = &my_fops
};
// called when module is installed
int __init init_module()
{	
	sema_init(&available, lines);
	sema_init(&filled, 0);
	sema_init(&flag, 1);
	read_line = 1;
	write_line = 1;
	r_status = misc_register(&my_misc_device);
	if(r_status != 0)
	{
		printk(KERN_ALERT "Device not registered");
		return 0;
	}
	pipe = (char**)kmalloc(sizeof(char*) * lines, GFP_KERNEL);
	while(i<lines)
	{
		pipe[i] = (char*)kmalloc(sizeof(char) * 100,GFP_KERNEL);
		i++;
	
	printk(KERN_ALERT "Device Registered %d\n",i);
	}
	return 0;
}


// called when module is removed
void __exit cleanup_module()
{
	misc_deregister(&my_misc_device);
	printk(KERN_ALERT "mymodule: Goodbyemachiucruel world!!\n");
}

