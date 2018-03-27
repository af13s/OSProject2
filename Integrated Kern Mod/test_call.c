#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* System call stub */
long (*STUB_start_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);

long (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_stop_elevator);

long (*STUB_issue_request)(int,int,int) = NULL;
EXPORT_SYMBOL(STUB_issue_request);

/* System call wrapper */
asmlinkage long start_elevator(int test_int) {
	if (STUB_start_elevator != NULL)
		return STUB_start_elevator();
	else
		return -ENOSYS;
}

/* System call wrapper */
asmlinkage long stop_elevator(int test_int) {
	if (STUB_stop_elevator != NULL)
		return STUB_stop_elevator();
	else
		return -ENOSYS;
}

asmlinkage long issue_request(int ptype, int sfloor, int dfloor) {
	if (STUB_issue_request != NULL)
		return STUB_issue_request(ptype, sfloor, dfloor);
	else
		return -ENOSYS;
}


