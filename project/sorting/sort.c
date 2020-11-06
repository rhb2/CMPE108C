#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sched.h>


typedef struct array_slice {
	int *array;
	int left;
	int right;
} array_slice_t;

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
slice_init(array_slice_t *sp, int *array, int left, int right)
{
	sp->array = array;
	sp->left = left;
	sp->right = right;
}

void
array_to_slice(int *array, int len, array_slice_t *slices)
{
	int i;
	int left = len / 2;
	int right = left;

	if (len % 2 == 0)
		right--;

	slice_init(&slices[0], array, 0, left - 1);
	slice_init(&slices[1], array, left, left + right);
}

void
swap(int *array, int x, int y)
{
	assert(array != NULL);

	array[x] = (array[x] + array[y]) - (array[y] = array[x]);
}

int
left_child(int i)
{
	return (i * 2 + 1);
}

void
heap_sift_down(int *array, int i, int len)
{
	int tmp, child;

	for (tmp = array[i]; left_child(i) < len; i = child) {
		child = left_child(i);

		if (child != len - 1 && array[child] < array[child + 1])
			child++;

		if (tmp >= array[child])
			break;

		array[i] = array[child];
	}

	array[i] = tmp;
}

void
heapify(int *array, int len)
{
	int i;

	for (i = len / 2; i >= 0; i--)
		heap_sift_down(array, i, len);
}

void
heap_sort(int *array, int len)
{
	int i;

	heapify(array, len + 1);

	for (i = len; i > 0; i--) {
		swap(array, i, 0);
		heap_sift_down(array, 0, i);
	}
}

void
quicksort(int *array, int left, int right)
{
	int last, i;

	if (left >= right)
		return;

	swap(array, left, (left + right) / 2);
	last = left;

	for (i = left + 1; i <= right; i++)
		if (array[i] < array[left])
			swap(array, ++last, i);

	swap(array, left, last);
	quicksort(array, left, last - 1);
	quicksort(array, last + 1, right);
}

void
sort_func(void *arg, int thread_num)
{
	array_slice_t *sp;

	assert(arg != NULL);

	sp = arg;
	heap_sort(sp->array + sp->left, sp->right - sp->left);
//	quicksort(sp->array, sp->left, sp->right);
}

void
merge(int *array, int l_pos, int r_pos, int r_end)
{
	int i;
	int elems = r_end - l_pos + 1;
	int l_end = r_pos - 1;
	int tmp_pos = 0;
	int *tmp;

	tmp = malloc(sizeof (int) * elems);

	assert(tmp != NULL);

	while (l_pos <= l_end && r_pos <= r_end) {
		if (array[l_pos] <= array[r_pos])
			tmp[tmp_pos++] = array[l_pos++];
		else
			tmp[tmp_pos++] = array[r_pos++];
	}

	while (l_pos <= l_end)
		tmp[tmp_pos++] = array[l_pos++];

	while (r_pos <= r_end)
		tmp[tmp_pos++] = array[r_pos++];

	for (i = 0; i < elems; i++)
		array[i] = tmp[i];

	free(tmp);
}

void
merge_func(void *arg, int thread_num)
{
	array_slice_t *slices;

	assert(arg != NULL);

	slices = arg;
	merge(slices[0].array, 0, slices[1].left, slices[1].right);
}

void
help_check(int *array, int len)
{
	int i;

	for (i = 0; i < len - 1; i++)
		assert(array[i] <= array[i + 1]);
}

void main(int argc, char **argv)
{
	int i;
	int  len;
	int *array;
	int threads;
	array_slice_t slices[2];
	sched_t sched_sort;
	task_t tasks[2];

	if (argc != 3) {
		printf("Mising length of array or number of threads.\n");
		exit(-1);
	}

	threads = atoi(argv[1]);
	len = atoi(argv[2]);
	if ((array = malloc(sizeof (int) * len)) == NULL) {
		printf("Memory allocation failed.\n");
		exit(-1);
	}

	for (i = 0; i < len; i++)
		array[i] = rand() % len * 3;

	print_array(array, len);

	sched_init(&sched_sort, threads, 2);
	array_to_slice(array, len, slices);

	for (i = 0; i < 2; i++) {
		task_init(&tasks[i], 1, sort_func, (void *)&slices[i]);
		(void) sched_post(&sched_sort, &tasks[i], false);
	}
	sched_execute(&sched_sort);

	task_init(&tasks[0], 1, merge_func, (void *)slices);
	(void) sched_post(&sched_sort, &tasks[0], false);
	sched_execute(&sched_sort);
	sched_fini(&sched_sort);

	help_check(array, len);
	print_array(array, len);
}
