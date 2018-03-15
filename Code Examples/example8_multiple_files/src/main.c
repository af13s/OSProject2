#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/linkage.h>

#include <syscall.h>
#include <proc.h>
#include <thread.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Combination of example3 and example8 spread out into multiple files");

struct thread_parameter thread1;

static int hello_init(void) {
	int ret;
	
	syscall_init();
	
	ret = thread_init(&thread1);
	if (ret)
		return ret;
	
	ret = proc_init(&thread1);
	if (ret) {
		thread_exit(&thread1);
		return ret;
	}
	
	return 0;
}
module_init(hello_init);

static void hello_exit(void) {
	syscall_exit();
	proc_exit();
	thread_exit(&thread1);
}
module_exit(hello_exit);
