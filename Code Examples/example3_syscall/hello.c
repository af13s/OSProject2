#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/linkage.h>
MODULE_LICENSE("GPL");

extern long (*STUB_test_call)(int);
long my_test_call(int test) {
	printk(KERN_NOTICE "%s: Your int is %d\n", __FUNCTION__, test);
	return test;
}

static int hello_init(void) {
	STUB_test_call = my_test_call;
	return 0;
}
module_init(hello_init);

static void hello_exit(void) {
	STUB_test_call = NULL;
}
module_exit(hello_exit);
