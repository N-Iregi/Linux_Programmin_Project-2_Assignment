#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

/**
 * struct shared_data - Holds all shared state for the thread pool
 * @files: Array of filenames to process
 * @num_files: Total number of files
 * @current_index: Index of the next file to process
 * @pattern: The string pattern to search for
 * @report_fp: File pointer for the shared report file
 * @total_matches: Grand total of matching lines across all files
 * @queue_mutex: Protects current_index
 * @report_mutex: Protects report_fp
 * @total_mutex: Protects total_matches
 */
typedef struct {
	char **files;
	int num_files;
	int current_index;
	char *pattern;
	FILE *report_fp;
	int total_matches;
	pthread_mutex_t queue_mutex;
	pthread_mutex_t report_mutex;
	pthread_mutex_t total_mutex;
} shared_data_t;

/**
 * struct worker_args - Arguments passed to each worker thread
 * @id: The thread ID (0 to N-1)
 * @data: Pointer to the shared state structure
 */
typedef struct {
	int id;
	shared_data_t *data;
} worker_args_t;

/**
 * process_file - Scans a single file for the pattern and updates shared state
 * @filename: The file to scan
 * @data: Pointer to shared data
 * @tid: Thread ID processing this file
 */
void process_file(const char *filename, shared_data_t *data, int tid)
{
	FILE *fp;
	char buf[4096];
	int lines = 0, matches = 0;

	fp = fopen(filename, "r");
	if (!fp)
	{
		perror(filename);
		return;
	}
	while (fgets(buf, sizeof(buf), fp))
	{
		lines++;
		if (strstr(buf, data->pattern))
			matches++;
	}
	fclose(fp);

	pthread_mutex_lock(&data->report_mutex);
	fprintf(data->report_fp, "%s: %d matching lines out of %d (thread %d)\n",
		filename, matches, lines, tid);
	pthread_mutex_unlock(&data->report_mutex);

	pthread_mutex_lock(&data->total_mutex);
	data->total_matches += matches;
	pthread_mutex_unlock(&data->total_mutex);
}

/**
 * worker_loop - The main execution loop for each thread in the pool
 * @arg: Pointer to worker_args_t
 *
 * Return: NULL upon completion
 */
void *worker_loop(void *arg)
{
	worker_args_t *args = (worker_args_t *)arg;
	shared_data_t *data = args->data;
	int idx;

	while (1)
	{
		pthread_mutex_lock(&data->queue_mutex);
		if (data->current_index >= data->num_files)
		{
			pthread_mutex_unlock(&data->queue_mutex);
			break;
		}
		idx = data->current_index++;
		pthread_mutex_unlock(&data->queue_mutex);

		process_file(data->files[idx], data, args->id);
	}
	return (NULL);
}

/**
 * run_threads - Initializes and joins the thread pool
 * @shared: Pointer to the initialized shared data
 * @t_cnt: Number of threads to create
 */
void run_threads(shared_data_t *shared, int t_cnt)
{
	pthread_t *threads;
	worker_args_t *args;
	int i;

	threads = malloc(sizeof(pthread_t) * t_cnt);
	args = malloc(sizeof(worker_args_t) * t_cnt);

	for (i = 0; i < t_cnt; i++)
	{
		args[i].id = i;
		args[i].data = shared;
		pthread_create(&threads[i], NULL, worker_loop, &args[i]);
	}

	for (i = 0; i < t_cnt; i++)
		pthread_join(threads[i], NULL);

	free(threads);
	free(args);
}

/**
 * main - Entry point for the logscan utility
 * @argc: Argument count
 * @argv: Argument vector
 *
 * Return: 0 on success, 1 on failure
 */
int main(int argc, char *argv[])
{
	shared_data_t shared;
	int t_cnt;
	struct timespec t_start, t_end;
	double elapsed;

	if (argc < 5)
	{
		fprintf(stderr, "Usage: %s <pat> <rep> <threads> <f1>...\n", argv[0]);
		return (1);
	}

	t_cnt = atoi(argv[3]);
	shared.pattern = argv[1];
	shared.report_fp = fopen(argv[2], "w");
	shared.files = &argv[4];
	shared.num_files = argc - 4;
	shared.current_index = 0;
	shared.total_matches = 0;

	pthread_mutex_init(&shared.queue_mutex, NULL);
	pthread_mutex_init(&shared.report_mutex, NULL);
	pthread_mutex_init(&shared.total_mutex, NULL);

	clock_gettime(CLOCK_MONOTONIC, &t_start);
	run_threads(&shared, t_cnt);
	clock_gettime(CLOCK_MONOTONIC, &t_end);

	fprintf(shared.report_fp, "Total matching lines across %d files: %d\n",
		shared.num_files, shared.total_matches);
	printf("Total matching lines across %d files: %d\n",
		shared.num_files, shared.total_matches);

	elapsed = (t_end.tv_sec - t_start.tv_sec) +
		  (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
	printf("Elapsed time: %.6f seconds\n", elapsed);

	fclose(shared.report_fp);
	pthread_mutex_destroy(&shared.queue_mutex);
	pthread_mutex_destroy(&shared.report_mutex);
	pthread_mutex_destroy(&shared.total_mutex);
	return (0);
}
