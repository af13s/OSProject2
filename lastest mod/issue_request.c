#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* System call stub */
long (*STUB_issue_request)(int,int,int) = NULL;
EXPORT_SYMBOL(STUB_issue_request);


asmlinkage long sys_issue_request(int ptype, int sfloor, int dfloor) {
	if (STUB_issue_request != NULL)
		return STUB_issue_request(ptype, sfloor, dfloor);
	else
		return -ENOSYS;
}


