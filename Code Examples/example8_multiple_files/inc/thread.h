#include <linux/mutex.h>

#ifndef __THREAD_H
#define __THREAD_H

#define CNT_SIZE 20

struct thread_parameter {
	int id;
	int cnt[CNT_SIZE];
	struct task_struct *kthread;
	struct mutex mutex;
};

int thread_init(struct thread_parameter *parm);
int thread_exit(struct thread_parameter *parm);

#endif
