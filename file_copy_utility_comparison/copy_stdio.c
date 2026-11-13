#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * main - Copies a file using standard I/O functions.
 * @argc: The number of command-line arguments.
 * @argv: The array of command-line arguments.
 *
 * Return: 0 on success, 1 on error.
 */
int main(int argc, char *argv[])
{
	FILE *fp_in, *fp_out;
	size_t bytes_read, bytes_written, buf_size;
	char *buffer;
	struct timespec start, end;
	double elapsed;

	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s <src> <dst> <buf_size>\n", argv[0]);
		return (1);
	}

	buf_size = strtoul(argv[3], NULL, 10);
	buffer = malloc(buf_size);
	if (!buffer)
	{
		perror("malloc");
		return (1);
	}

	fp_in = fopen(argv[1], "rb");
	if (!fp_in)
	{
		perror("fopen source");
		free(buffer);
		return (1);
	}

	fp_out = fopen(argv[2], "wb");
	if (!fp_out)
	{
		perror("fopen destination");
		fclose(fp_in);
		free(buffer);
		return (1);
	}

	clock_gettime(CLOCK_MONOTONIC, &start);

	while ((bytes_read = fread(buffer, 1, buf_size, fp_in)) > 0)
	{
		bytes_written = fwrite(buffer, 1, bytes_read, fp_out);
		if (bytes_written != bytes_read)
		{
			perror("fwrite");
			fclose(fp_in);
			fclose(fp_out);
			free(buffer);
			return (1);
		}
	}

	if (ferror(fp_in))
		perror("fread");

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("copy_stdio elapsed time: %.6f seconds\n", elapsed);

	fclose(fp_in);
	fclose(fp_out);
	free(buffer);
	return (0);
}
