#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "heap.h"
#include "sched.h"


static task_t *
get_next_task(heap_t *hp)
{
	heap_elem_t elem;

	assert(hp != NULL);

	if (heap_remove(hp, &elem))
		return (elem.meta);

	return (NULL);
}

void
task_init(task_t *tp, uint64_t pri, void (*fptr)(void *, int), void *args)
{
	assert(tp != NULL);

	bzero(tp, sizeof (task_t));
	tp->pri = pri;
	tp->fptr = fptr;
	tp->args = args;
}

static int
get_thread_index(sched_t *sp)
{
	int i;
	int index = -1;
	pthread_t tid = pthread_self();

	assert(sp != NULL);

	(void) pthread_mutex_lock(&sp->pq.lock);

	for (i = 0; i < sp->num_workers; i++) {
		if (pthread_equal(sp->workers[i], tid) != 0) {
			index = i;
			break;
		}
	}

	(void) pthread_mutex_unlock(&sp->pq.lock);

	return (index);
}

static void *
worker_func(void *arg)
{
	sched_t *sp = arg;
	priority_queue_t *pq;
	heap_t *hp;
	heap_elem_t elem;
	task_t *task;
	int thread_num;

	assert(sp != NULL);

	pq = &sp->pq;
	hp = pq->heap;

	/*
	 * Under normal circumstances, it does not matter what thread does the
	 * work as long as it gets done, but there may be circumstances where
	 * we want certain workers to always process the same part of a given
	 * problem.  Such examples include, but are not limited to parallel
	 * compression over a network.  Knowing the index of this thread in
	 * our table of workers will allow applications to do things like direct
	 * the nth worker at the nth chunk of a block that all threads have
	 * access to.
	 */
	thread_num = get_thread_index(sp);

	assert(thread_num >= 0);

	for (;;) {
		(void) pthread_mutex_lock(&pq->lock);

		/*
		 * If the heap is empty, and the `done' latch has been set,
		 * then no more work is coming.  This worker is free to exit.
		 */
		if (sp->state == SCHED_DONE && heap_empty(hp)) {
			(void) pthread_mutex_unlock(&pq->lock);
			break;
		}

		/*
		 * If the heap is empty and the scheduler is still running, then
		 * more work will eventualy come.  Wait on the cv until we
		 * receive a signal.
		 */
		if (heap_empty(hp) || sp->state == SCHED_STOPPED)
			(void) pthread_cond_wait(&pq->cv, &pq->lock);

		task = get_next_task(hp);
		(void) pthread_mutex_unlock(&pq->lock);

		/*
		 * There is no gaurantee that we will have work to do.
		 * Another thread may have beat us to get whatever the next
		 * task is, leaving the queue empty.  If that's the case, we
		 * will just basically go back to sleep when we resume the
		 * loop (above).
		 */
		if (task != NULL) {
			task->fptr(task->args, thread_num);

			(void) pthread_mutex_lock(&pq->lock);

			if (--pq->remaining_tasks == 0)
				(void) pthread_cond_broadcast(&pq->cv);

			(void) pthread_mutex_unlock(&pq->lock);
		}

	}

	return (NULL);
}

static bool
priority_queue_init(priority_queue_t *pq, size_t capacity)
{
	assert(pq != NULL);

	bzero(pq, sizeof (priority_queue_t));

	if (pthread_cond_init(&pq->cv, NULL) != 0)
		return (false);

	if (pthread_mutex_init(&pq->lock, NULL) != 0) {
		(void) pthread_cond_destroy(&pq->cv);
		return (false);
	}

	if ((pq->heap = heap_create(capacity)) == NULL) {
		(void) pthread_cond_destroy(&pq->cv);
		(void) pthread_mutex_destroy(&pq->lock);
		return (false);
	}

	return (true);
}

static void
priority_queue_destroy(priority_queue_t *pq)
{
	assert(pq != NULL);

	(void) pthread_cond_destroy(&pq->cv);
	(void) pthread_mutex_destroy(&pq->lock);
	heap_destroy(pq->heap);
}

bool
sched_init(sched_t *sp, int num_workers, int queue_depth)
{
	int i;

	assert(sp != NULL);

	bzero(sp, sizeof (sched_t));

	if (!priority_queue_init(&sp->pq, queue_depth))
		return (false);

	if ((sp->workers = malloc(sizeof (pthread_t) * num_workers)) == NULL) {
		priority_queue_destroy(&sp->pq);
		return (false);
	}

	sp->state = SCHED_STOPPED;

	(void) pthread_mutex_lock(&sp->pq.lock);

	for (i = 0; i < num_workers; i++) {
		(void) pthread_create(&sp->workers[i], NULL, worker_func,
		    (void *)sp);
	}

	sp->num_workers = num_workers;
	(void) pthread_mutex_unlock(&sp->pq.lock);

	return (true);
}

bool
sched_post(sched_t *sp, task_t *tp, bool run_now)
{
	priority_queue_t *pq;
	heap_elem_t elem;

	assert(sp != NULL && tp != NULL);

	pq = &sp->pq;

	(void) pthread_mutex_lock(&pq->lock);

	if (heap_full(pq->heap)) {
		(void) pthread_mutex_unlock(&pq->lock);
		return (false);
	}

	elem.val = tp->pri;
	elem.meta = tp;
	(void) heap_insert(pq->heap, elem);

	pq->remaining_tasks++;

	if (run_now) {
		sp->state = SCHED_RUNNING;
		(void) pthread_cond_signal(&pq->cv);
	}

	(void) pthread_mutex_unlock(&pq->lock);
	return (true);
}

/*
 * When this function is invoked, it will block the caller until all tasks in
 * the queue have been execute.  At the conclusion of the last task, it will
 * set the scheduler to a state of SCHED_STOPPED, ultimately putting all workers
 * to sleep until more work is posted or until this function is invoked again.
 * If the task queue gets full, it may be necessary to either call this function
 * to clear it out in order to make room, _or_ call sched_post() where the
 * third argument is set to true indicating that the scheduler should get to
 * work if it isn't already.
 */
void
sched_execute(sched_t *sp)
{
	priority_queue_t *pq;

	assert(sp != NULL);

	pq = &sp->pq;
	(void) pthread_mutex_lock(&pq->lock);
	sp->state = SCHED_RUNNING;
	(void) pthread_cond_broadcast(&pq->cv);

	while (pq->remaining_tasks > 0)
		(void) pthread_cond_wait(&pq->cv, &pq->lock);

	sp->state = SCHED_STOPPED;
	(void) pthread_mutex_unlock(&pq->lock);
}

void
sched_fini(sched_t *sp)
{
	int i;
	priority_queue_t *pq;

	assert(sp != NULL);

	pq = &sp->pq;
	assert(pq != NULL);

	(void) pthread_mutex_lock(&pq->lock);
	sp->state = SCHED_DONE;
	(void) pthread_cond_broadcast(&pq->cv);
	(void) pthread_mutex_unlock(&pq->lock);

	for (i = 0; i < sp->num_workers; i++)
		(void) pthread_join(sp->workers[i], NULL);

	priority_queue_destroy(&sp->pq);
	free(sp->workers);
}
