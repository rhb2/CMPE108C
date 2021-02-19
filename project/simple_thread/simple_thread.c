#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sched.h>

static pthread_mutex_t lock;

void
simple_thread(void *arg, int thread_num)
{
	int num;
	int val;
	int *shared_variable;

	assert(arg != NULL);

	shared_variable = (int *)arg;

	for (num = 0; num < 20; num++) {
#ifdef PTHREAD_SYNC
		(void) pthread_mutex_lock(&lock);
#endif	/* PTHREAD_SYNC */

		val = *shared_variable;
		printf("*** thread %d sees value %d\n", thread_num, val);
		*shared_variable = val + 1;

#ifdef PTHREAD_SYNC
		(void) pthread_mutex_unlock(&lock);
#endif
	}
}

bool
validate_arg(int len, char **args)
{
	int i;

	if (len != 2)
		return (false);

	for (i = 0; i < strlen(args[1]); i++)
		if (!isdigit(args[1][i]))
			return (false);

	return (true);	
}

int main(int argc, char **argv)
{
	int i;
	bool ret;
	sched_t sched;
	task_t *tasks;
	int nthreads;
	int queue_depth;
	int val = 0;

	if (!validate_arg(argc, argv)) {
		printf("Invalid arguments supplied.\n");
		exit(-1);
	}

	queue_depth = nthreads = atoi(argv[1]);

	if ((tasks = malloc(sizeof (task_t) * nthreads)) == NULL) {
		printf("Memory allocation failure.\n");
		exit(-1);
	}

#ifdef PTHREAD_SYNC
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("Mutex initialization failed.\n");
		exit (-1);
	}
#endif	/* PTHREAD_SYNC */

	/* Initialize our scheduler. */
	ret = sched_init(&sched, nthreads, queue_depth);

	/* If we failed to initialize the scheduler, then immediately abort. */
	assert(ret == true);

	for (i = 0; i < nthreads; i++) {
		/*
		 * Initialize a task.  Give it a priority of 1, specify that we
		 * want the task to run `simple_thread' and the address of the
		 * argument that should be supplied to `simple_thread' is the
		 * address of element to process is that of `val` which was
		 * allocated on the stack of main().
		 */
		task_init(&tasks[i], 1, simple_thread, (void *)&val);

		/* Insert the task in to the schedulers task queue. */
		(void) sched_post(&sched, &tasks[i], false);
	}

	/* Begin execution of all tasks and block until they are finished. */
	sched_execute(&sched);

	/* Destroy our scheduler -- we are done. */
	sched_fini(&sched);

	return (0);
}
