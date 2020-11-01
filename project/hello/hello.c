#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sched.h>


#define	NWORKERS	5
#define	QUEUE_DEPTH	10
#define	NTASKS		10

void
print_array(int *array, int len)
{
	int i;

	assert(array != NULL);

	for (i = 0; i < len; i++)
		printf("%d ", array[i]);

	printf("\n");
}

void
hello_func(void *arg, int thread_num)
{
	int *elem;
	int val;
	int result;

	assert(arg != NULL);

	/*
	 * We know that `arg' is really the address of one of the integers in
	 * the `args' array which was defined in main().  We are going to
	 * square the value stored at that address.  The address contained by
	 * `elem' is the same as the address in `arg', however `elem' being
	 * an `int *' instead of a `void *' tells the compiler that the data
	 * located at this address can be treated like an integer (because it is
	 * one).
	 */
	elem = (int *)arg;

	/* Dereference `elem' to get the integer stored at that address. */
	val = *elem;

	/* Square the value. */
	result = val * val;

	printf("Hello, I am thread %d.  I am going to square %d to get %d.\n",
	    thread_num, *elem, result);

	/* Store the result at the address specified by `elem'. */
	*elem = result;
}

void main(int argc, char **argv)
{
	int i;
	bool ret;
	sched_t sched;
	task_t tasks[NTASKS];
	int args[NTASKS];

	/* Populate our array with values increasing from 0 to NTASKS. */
	for (i = 0; i < NTASKS; i++)
		args[i] = i;

	/* Print the array before. */
	printf("Before: ");
	print_array(args, NTASKS);

	/* Initialize our scheduler. */
	ret = sched_init(&sched, NWORKERS, QUEUE_DEPTH);

	/* If we failed to initialize the scheduler, then immediately abort. */
	assert(ret == true);

	for (i = 0; i < NTASKS; i++) {
		/*
		 * Initialize a task.  Give it a priority of 1, specify that we
		 * want the task to run `hello_func' and the address of the
		 * argument that should be supplied to `hello_func' is the
		 * address of element `i' in the `args' array.
		 */
		task_init(&tasks[i], 1, hello_func, (void *)&args[i]);

		/* Insert the task in to the schedulers task queue. */
		(void) sched_post(&sched, &tasks[i], false);
	}

	/* Begin execution of all tasks and block until they are finished. */
	sched_execute(&sched);

	/* Print the array after. */
	printf("After: ");
	print_array(args, NTASKS);

	/* Destroy our scheduler -- we are done. */
	sched_fini(&sched);
}
