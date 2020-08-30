#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include "heap.h"

heap_t *
heap_create(int capacity)
{
	heap_t *hp;

	if ((hp = malloc(sizeof (heap_t))) == NULL)
		return (NULL);

	(void) bzero(hp, sizeof (heap_t));

	if ((hp->data = malloc(sizeof (heap_elem_t) * capacity)) == NULL) {
		free(hp);
		return (NULL);
	}

	hp->capacity = capacity;
	return (hp);
}

void
heap_destroy(heap_t *hp)
{
	assert(hp != NULL);

	free(hp->data);
	free(hp);
}

bool
heap_insert(heap_t *hp, heap_elem_t elem)
{

	assert(hp != NULL);

	/*
	 * If the heap is full and we fail to increase our capacity, then there
	 * is not much else we can do.  Flag failure and return.
	 */
	if (heap_full(hp) && !heap_double(hp))
		return (false);

	hp->data[hp->total] = elem;
	sift_up(hp->data, hp->total);
	hp->total++;
	return (true);
}

bool
heap_remove(heap_t *hp, heap_elem_t *elem)
{
	int i;

	assert(hp != NULL && elem != NULL);

	if (heap_empty(hp))
		return (false);

	*elem = hp->data[0];

	hp->data[0] = hp->data[hp->total - 1];
	sift_down(hp->data, 0, hp->total - 1);
	hp->total--;
	return (true);
}

bool
heap_empty(heap_t *hp)
{
	assert(hp != NULL);

	return (hp->total == 0);
}

bool
heap_full(heap_t *hp)
{
	assert(hp != NULL);

	return (hp->total == hp->capacity);
}

bool
heap_double(heap_t *hp)
{
	assert(hp != NULL);

	if (!expand(&hp->data, hp->capacity))
		return (false);

	hp->capacity *= 2;
	return (true);
}

void
heap_print(heap_t *hp)
{
	int i;

	assert(hp != NULL);

	for (i = 0; i < hp->total; i++) {
		printf("%d ", hp->data[i].val);

		int x = i + 2;
		if (!(x & (x - 1)))
			printf("\n");
	}

	printf("\n");
}

static void
sift_up(heap_elem_t *array, int cur)
{
	int parent;
	heap_elem_t tmp;

	assert(array != NULL);

	for (tmp = array[cur]; cur > 0; cur = parent) {
		parent = cur / 2;

		if (tmp.val >= array[parent].val)
			break;

		array[cur] = array[parent];
	}

	array[cur] = tmp;
}

static int
left_child(int i)
{
	return (i * 2 + 1);
}

static void
sift_down(heap_elem_t *array, int cur, int len)
{
	heap_elem_t tmp;
	int child;

	assert(array != NULL);

	for (tmp = array[cur]; left_child(cur) < len; cur = child) {
		child = left_child(cur);

		if (child != len - 1 && array[child].val > array[child + 1].val)
			child++;

		if (tmp.val <= array[child].val)
			break;

		array[cur] = array[child];
	}

	array[cur] = tmp;
}

static bool
expand(heap_elem_t **array, int len)
{
	int i;
	int newlen = len * 2;
	heap_elem_t *newarray;

	assert(array != NULL && *array != NULL);

	if ((newarray = malloc(sizeof (heap_elem_t) * newlen)) == NULL)
		return (false);

	(void) bzero(newarray, sizeof (heap_elem_t) * newlen);
	(void) memcpy(newarray, *array, sizeof (heap_elem_t) * len);
	free(*array);
	*array = newarray;
	return (true);
}
