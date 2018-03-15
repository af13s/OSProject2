#include <linux/module.h>
#include <syscall.h>

extern long (*STUB_test_call)(int);

long my_test_call(int test) {
	printk(KERN_NOTICE "%s: Your int is %d\n", __FUNCTION__, test);
	return test;
}

int syscall_init(void) {
	STUB_test_call = my_test_call;
	return 0;
}

int syscall_exit(void) {
	STUB_test_call = NULL;
	return 0;
}
