#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <thread.h>

int thread_run(void *data) {
	int i;
	struct thread_parameter *parm = data;

	while (!kthread_should_stop()) {
		if (mutex_lock_interruptible(&parm->mutex) == 0) {
			for (i = 0; i < CNT_SIZE; i++)
				parm->cnt[i]++;
		}
		mutex_unlock(&parm->mutex);
	}

	return 0;
}

int thread_init(struct thread_parameter *parm) {
	static int id = 1;
	int i;

	parm->id = id++;
	for (i = 0; i < CNT_SIZE; i++)
		parm->cnt[i] = 0;
		
	mutex_init(&parm->mutex);
	parm->kthread = kthread_run(thread_run, parm, "thread example %d", parm->id);
	return 0;
}

int thread_exit(struct thread_parameter *parm) {
	kthread_stop(parm->kthread);
	mutex_destroy(&parm->mutex);
	return 0;
}
