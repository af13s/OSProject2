#include <thread.h>

#ifndef __PROC_H
#define __PROC_H

int proc_init(struct thread_parameter *parm);
int proc_exit(void);

#endif
