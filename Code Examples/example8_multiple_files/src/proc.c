#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <thread.h>
#include <proc.h>

#define ENTRY_NAME "mutlifile_example"
#define ENTRY_SIZE 10000 
#define PERMS 0644
#define PARENT NULL

static char *message;
static int read_p;

struct file_operations fops;
struct thread_parameter *thread;

int proc_open(struct inode *sp_inode, struct file *sp_file) {
	int i;
	char *buf;
	read_p = 1;

	buf = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "hello_proc_open");
		return -ENOMEM;
	}
	
	sprintf(message, "Thread %d has cycled this many times:\n", thread->id);
	if (mutex_lock_interruptible(&thread->mutex) == 0) {
		for (i = 0; i < CNT_SIZE; i++) {
			sprintf(buf, "%d\n", thread->cnt[i]);
			strcat(message, buf);
		}
	}
	else {
		for (i = 0; i < CNT_SIZE; i++) {
			sprintf(buf, "%d\n", -1);
			strcat(message, buf);
		}
	}
	mutex_unlock(&thread->mutex);
	strcat(message, "\n");

	return 0;
}

ssize_t proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	copy_to_user(buf, message, len);
	return len;
}

int proc_release(struct inode *sp_inode, struct file *sp_file) {
	kfree(message);
	return 0;
}


int proc_init(struct thread_parameter *parm) {
	thread = parm;
	
	fops.open = proc_open;
	fops.read = proc_read;
	fops.release = proc_release;
	
	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "thread_init");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}
	else
		return 0;
}

int proc_exit(void) {
	remove_proc_entry(ENTRY_NAME, NULL);
	return 0;
}
