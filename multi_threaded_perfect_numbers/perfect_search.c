#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

/**
 * struct t_ctx - Context passed to each thread
 * @start: The starting number for this thread's segment
 * @end: The ending number for this thread's segment
 * @arr: Pointer to the shared array storing perfect numbers
 * @cnt: Pointer to the shared global counter
 * @mut: Pointer to the shared mutex
 */
typedef struct {
	int start;
	int end;
	int *arr;
	int *cnt;
	pthread_mutex_t *mut;
} t_ctx;

/**
 * is_perfect - Checks if a number is a perfect number
 * @n: The number to check
 *
 * Return: 1 if perfect, 0 otherwise
 */
int is_perfect(int n)
{
	int sum = 1, i;

	if (n <= 1)
		return (0);

	for (i = 2; i * i <= n; i++)
	{
		if (n % i == 0)
		{
			sum += i;
			if (i * i != n)
				sum += n / i;
		}
	}
	return (sum == n);
}

/**
 * sort_array - Sorts an array of integers in ascending order
 * @arr: The array to sort
 * @n: The number of elements
 */
void sort_array(int *arr, int n)
{
	int i, j, tmp;

	for (i = 0; i < n - 1; i++)
	{
		for (j = 0; j < n - i - 1; j++)
		{
			if (arr[j] > arr[j + 1])
			{
				tmp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = tmp;
			}
		}
	}
}

/**
 * worker - Thread entry point to find perfect numbers
 * @arg: Pointer to the thread context (t_ctx)
 *
 * Return: NULL
 */
void *worker(void *arg)
{
	t_ctx *c = (t_ctx *)arg;
	int i, l_cnt = 0, l_arr[10];

	for (i = c->start; i <= c->end; i++)
	{
		if (is_perfect(i))
			l_arr[l_cnt++] = i;
	}

	pthread_mutex_lock(c->mut);
	for (i = 0; i < l_cnt; i++)
		c->arr[*(c->cnt) + i] = l_arr[i];
	*(c->cnt) += l_cnt;
	pthread_mutex_unlock(c->mut);

	return (NULL);
}

/**
 * run_single - Executes the search on a single thread
 *
 * Return: Elapsed time in seconds
 */
double run_single(void)
{
	int i, dummy = 0;
	struct timespec st, en;

	clock_gettime(CLOCK_MONOTONIC, &st);
	for (i = 1; i <= 100000; i++)
	{
		if (is_perfect(i))
			dummy++;
	}
	clock_gettime(CLOCK_MONOTONIC, &en);

	return ((en.tv_sec - st.tv_sec) + (en.tv_nsec - st.tv_nsec) / 1e9);
}

/**
 * run_multi - Executes the search across 8 threads
 * @arr: Pointer to store results
 * @cnt: Pointer to the results counter
 *
 * Return: Elapsed time in seconds
 */
double run_multi(int *arr, int *cnt)
{
	int i;
	pthread_t t[8];
	t_ctx c[8];
	struct timespec ts[2]; /* 0: start, 1: end */
	pthread_mutex_t m;

	pthread_mutex_init(&m, NULL);
	clock_gettime(CLOCK_MONOTONIC, &ts[0]);

	for (i = 0; i < 8; i++)
	{
		c[i].start = (i * 12500) + 1;
		c[i].end = (i + 1) * 12500;
		c[i].arr = arr;
		c[i].cnt = cnt;
		c[i].mut = &m;
		pthread_create(&t[i], NULL, worker, &c[i]);
	}
	for (i = 0; i < 8; i++)
		pthread_join(t[i], NULL);

	clock_gettime(CLOCK_MONOTONIC, &ts[1]);
	pthread_mutex_destroy(&m);

	return ((ts[1].tv_sec - ts[0].tv_sec) + (ts[1].tv_nsec - ts[0].tv_nsec) / 1e9);
}

/**
 * main - Program entry point
 *
 * Return: 0 on success
 */
int main(void)
{
	int arr[20], cnt = 0, i;
	double t_st, t_mt;

	t_st = run_single();
	t_mt = run_multi(arr, &cnt);

	sort_array(arr, cnt);

	printf("Perfect numbers found:");
	for (i = 0; i < cnt; i++)
		printf(" %d", arr[i]);
	printf("\n");

	printf("The synchronized total number of perfect numbers between 1 and 100000 is %d\n", cnt);
	printf("Single-threaded time: %f seconds\n", t_st);
	printf("Multi-threaded time:  %f seconds\n", t_mt);

	return (0);
}
