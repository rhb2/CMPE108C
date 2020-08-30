#ifndef	_HEAP_H
#define	_HEAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

typedef struct heap_elem {
	int val;	/* Used when performing heap operations. */
	void *meta;	/* Used if we want to keep structures in the heap. */
} heap_elem_t;

typedef struct heap {
	int capacity;
	int total;
	heap_elem_t *data;
} heap_t;

heap_t *heap_create(int);
void heap_destroy(heap_t *);
bool heap_insert(heap_t *, heap_elem_t);
bool heap_remove(heap_t *, heap_elem_t *);
bool heap_empty(heap_t *);
bool heap_full(heap_t *);
bool heap_double(heap_t *);
void heap_print(heap_t *);

static void sift_down(heap_elem_t *, int, int);
static void sift_up(heap_elem_t *, int);
static bool expand(heap_elem_t **, int);


#endif	/* _HEAP_H */
