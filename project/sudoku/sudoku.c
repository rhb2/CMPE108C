#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sched.h>


typedef struct pair {
	int row;
	int col;
} pair_t;

typedef struct map {
	int **matrix;
	pair_t coords;
	bool valid;
} map_t;

static bool validate_row(int **, int);
static bool validate_rows(int **);
static bool validate_col(int **, int);
static bool validate_cols(int **);
static bool validate_3_by_3(int **, int, int);

void
validate_rows_func(void *args, int thread_num)
{
	map_t *map = args;

	assert(args != NULL);

	printf("Thread num: %d.\n", thread_num);
	map->valid = validate_rows(map->matrix);
}

void
validate_cols_func(void *args, int thread_num)
{
	map_t *map = args;

	assert(args != NULL);

	printf("Thread num: %d.\n", thread_num);
	map->valid = validate_cols(map->matrix);
}

void
validate_3_by_3_func(void *args, int thread_num)
{
	map_t *map = args;

	assert(args != NULL);

	printf("Thread num: %d.\n", thread_num);
	map->valid = validate_3_by_3(map->matrix, map->coords.col,
	    map->coords.row);
}

static bool
validate_row(int **matrix, int row)
{
	int i;
	int tmp[10];
	int index;

	assert(matrix != NULL);

	bzero(tmp, sizeof (tmp));

	for (i = 1; i < 10; i++) {
		index = matrix[i - 1][row];
		if (index < 1 || index > 9 || tmp[index] > 0)
			return (false);

		tmp[index] = tmp[index] + 1;
	}
	return (true);
}

static bool
validate_rows(int **matrix)
{
	int i;

	assert(matrix != NULL);

	for (i = 0; i < 9; i++)
		if (!validate_row(matrix, i))
			return (false);

	return (true);
}

static bool
validate_col(int **matrix, int col)
{
	int i;
	int tmp[10];
	int index;

	assert(matrix != NULL);

	bzero(tmp, sizeof (tmp));

	for (i = 1; i < 10; i++) {
		index = matrix[col][i - 1];
		if (index < 1 || index > 9 || tmp[index] > 0)
			return (false);

		tmp[index] = tmp[index] + 1;
	}
	return (true);
}

static bool
validate_cols(int **matrix)
{
	int i;

	assert(matrix != NULL);

	for (i = 0; i < 9; i++)
		if (!validate_col(matrix, i))
			return (false);

	return (true);
}

static bool
validate_3_by_3(int **matrix, int col, int row)
{
	int tmp[10];
	int index;
	int r, c;

	assert(matrix != NULL);

	bzero(tmp, sizeof (tmp));

	/* Validate the sub-table one row at a time. */
	for (r = row; r < row + 3; r++) {
		for (c = col; c < col + 3; c++) {
			index = matrix[c][r];
			if (index < 1 || index > 9 || tmp[index] > 0)
				return (false);
			tmp[index] = tmp[index] + 1;
		}
	}

	return (true);
}

int **
matrix_clone(int (*m)[9], int rows, int cols)
{
	int i, j;
	int **matrix;
	int r, c;

	assert(m != NULL);

	if ((matrix = malloc(sizeof (int *) * cols)) == NULL)
		return (NULL);

	for (i = 0; i < rows; i++) {
		if ((matrix[i] = malloc(sizeof (int) * cols)) == NULL) {
			for (j = 0; i < i; i++)
				free(matrix[j]);

			free(matrix);
			return (NULL);
		}
	}

	for (r = 0; r < rows; r++) {
		for (c = 0; c < cols; c++)
			matrix[c][r] = m[c][r];
	}

	return (matrix);
}

void
matrix_destroy(int **matrix, int cols)
{
	int c;

	assert(matrix != NULL);

	for (c = 0; c < cols; c++)
		free(matrix[c]);

	free(matrix);
}

/*
 * The total number of tasks is determined by how many operations we intend to
 * perform.  The heuristic used was one task for each 3x3 sub-table in the
 * matrix which comes out to nine, plus another two tasks, where one validates
 * all columns in the table and the other validates all rows.  In short, the
 * forumula was:
 *
 *  9 subtable tasks + 1 rows task + 1 columns task = 11 total tasks
 */
#define	TOTAL_TASKS	11
#define	TOTAL_WORKERS	11
#define	SUDOKU_ROWS	9
#define	SUDOKU_COLS	9

bool
validate_sudoku_map(sched_t *sp, int **matrix)
{
	int i = 0;
	int row, col;
	sched_t sched_sudoku;
	task_t tasks[TOTAL_TASKS];
	map_t args[TOTAL_TASKS];

	for (col = 0; col < 9; col += 3) {
		for (row = 0; row < 9; row += 3) {
			args[i].matrix = matrix;
			args[i].coords.row = row;
			args[i].coords.col = col;
			task_init(&tasks[i], 1, validate_3_by_3_func,
			    (void *)&args[i]);
			(void) sched_post(sp, &tasks[i], false);
			i++;
		}
	}

	for (; i < TOTAL_TASKS; i++) {
		args[i].matrix = matrix;
		task_init(&tasks[i], 1,
		    (i % 2) ? validate_rows_func : validate_cols_func,
		    (void *)&args[i]);
		(void) sched_post(sp, &tasks[i], false);
	}

	sched_execute(sp);
	for (i = 0; i < TOTAL_TASKS; i++) {
		if (!args[i].valid)
			return (false);
	}

	return (true);
}

void main(int argc, char **argv)
{
	sched_t sched;
	int **copy;
	int valid_sudoku[SUDOKU_COLS][SUDOKU_ROWS] = {
		{ 6, 5, 8, 1, 9, 7, 3, 4, 2 },
		{ 2, 1, 3, 4, 5, 6, 7, 9, 8 },
		{ 4, 9, 7, 3, 8, 2, 1, 6, 5 },
		{ 5, 7, 6, 8, 2, 3, 9, 1, 4 },
		{ 3, 2, 1, 6, 4, 9, 5, 8, 7 },
		{ 9, 8, 4, 5, 7, 1, 6, 2, 3 },
		{ 1, 6, 2, 7, 3, 4, 8, 5, 9 },
		{ 8, 3, 9, 2, 6, 5, 4, 7, 1 },
		{ 7, 4, 5, 9, 1, 8, 2, 3, 6 },
	};

	/*
	 * Create a copy of the valid sudoku matrix because we plan to modify it
	 * after the test and re-test it.
	 */
	if ((copy = matrix_clone(valid_sudoku, 9, 9)) == NULL) {
		printf("Memory allocation failure\n.");
		exit(1);
	}

	sched_init(&sched, 20, TOTAL_TASKS);

	if (validate_sudoku_map(&sched, copy))
		printf("Valid sudoku map.\n");
	else
		printf("Invalid sudoku map.\n");

	matrix_destroy(copy, SUDOKU_COLS);
	sched_fini(&sched);
}
