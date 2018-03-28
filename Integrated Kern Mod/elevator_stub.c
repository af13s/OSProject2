#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/export.h>

/* System call stub */
long (*STUB_start_elevator)(void) = NULL;
long (*STUB_stop_elevator)(void) = NULL;
long (*STUB_issue_request)(int,int,int) = NULL;

EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_stop_elevator);
EXPORT_SYMBOL(STUB_issue_request);

/* System call wrapper */
asmlinkage long start_elevator(void) {
	if (STUB_start_elevator != NULL)
		return STUB_start_elevator(void);
	else
		return -ENOSYS;
}

/* System call wrapper */
asmlinkage long stop_elevator(void) {
	if (STUB_stop_elevator != NULL)
		return STUB_stop_elevator(void);
	else
		return -ENOSYS;
}

asmlinkage long issue_request(int ptype, int sfloor, int dfloor) {
	if (STUB_issue_request != NULL)
		return STUB_issue_request(ptype, sfloor, dfloor);
	else
		return -ENOSYS;
}


