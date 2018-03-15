#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* Syscall wrapper file */

long (*STUB_test_call)(int) = NULL;
EXPORT_SYMBOL(STUB_test_call);

asmlinkage long sys_test_call(int test_int) {
	if (STUB_test_call != NULL)
		return STUB_test_call(test_int);
	else
		return -ENOSYS;
}
