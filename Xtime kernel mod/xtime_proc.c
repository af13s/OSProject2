#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple module featuring proc read");

#define ENTRY_NAME "timed"
#define ENTRY_SIZE 300
#define PERMS 0644
#define PARENT NULL
static struct file_operations fops;
 
static int read_p;
static char * message;
static struct timespec prevTime;
static struct timespec currTime;
static struct timespec result;
int time_proc_open(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_INFO "proc called open\n");
	
	read_p = 1;
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "hello_proc_open");
		return -ENOMEM;
	}
	return 0;
}




void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

ssize_t time_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = 0;
	read_p = !read_p;
	if (read_p)
		return 0;
		
	//printk(KERN_INFO "proc called read\n");
	currTime = current_kernel_time();
	if(!prevTime.tv_sec)
	{
		sprintf(message,"current time: %ld.%ld\n",currTime.tv_sec,currTime.tv_nsec);
	}
	if(prevTime.tv_sec)
	{
		timespec_diff(&prevTime,&currTime,&result);
		sprintf(message,"Current time: %ld.%ld\nTime elapsed: %ld.%ld\n",currTime.tv_sec,currTime.tv_nsec,result.tv_sec,result.tv_nsec);
	}
	len = strlen(message);
	copy_to_user(buf,message, len);
	prevTime.tv_sec = currTime.tv_sec;
	prevTime.tv_nsec = currTime.tv_nsec;
	return len;
}

int time_proc_release(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_NOTICE "proc called release\n");
	kfree(message);
	return 0;
}





static int time_init(void) {
	printk(KERN_NOTICE "/proc/%s create\n",ENTRY_NAME);
	fops.open = time_proc_open;
	fops.read = time_proc_read;
	fops.release = time_proc_release;
	
	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "proc create\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}
	
	prevTime.tv_sec = 0;

	return 0;
}
module_init(time_init);


static void time_exit(void) {
	remove_proc_entry(ENTRY_NAME, NULL);
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
}
module_exit(time_exit);
