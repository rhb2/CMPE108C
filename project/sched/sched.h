#ifndef	SCHED_H_
#define	SCHED_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "heap.h"


typedef struct task {
	uint64_t pri;
	void (*fptr)(void *, int);
	void *args;
} task_t;

void task_init(task_t *, uint64_t, void (*fptr)(void *, int), void *);

typedef enum sched_state {
	SCHED_STOPPED,	/* Tasks can be posted, but will not be processed. */
	SCHED_RUNNING,	/* All tasks posted will be immediately processed. */
	SCHED_DONE	/* Used to tell all workers to exit. */
} sched_state_t;

typedef struct priority_queue {
	pthread_cond_t cv;
	pthread_mutex_t lock;
	heap_t *heap;
	int remaining_tasks;
} priority_queue_t;

typedef struct sched {
	priority_queue_t pq;
	pthread_t *workers;
	int num_workers;
	sched_state_t state;
} sched_t;

bool sched_init(sched_t *, int, int);
bool sched_post(sched_t *, task_t *, bool);
void sched_execute(sched_t *);
void sched_fini(sched_t *);

#endif	/* SCHED_H_ */
