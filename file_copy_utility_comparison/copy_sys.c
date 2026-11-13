#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

/**
 * main - Copies a file using raw POSIX system calls.
 * @argc: The number of command-line arguments.
 * @argv: The array of command-line arguments.
 *
 * Return: 0 on success, 1 on error.
 */
int main(int argc, char *argv[])
{
	int fd_in, fd_out;
	ssize_t bytes_read, bytes_written;
	size_t buf_size;
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

	fd_in = open(argv[1], O_RDONLY);
	if (fd_in == -1)
	{
		perror("open source");
		free(buffer);
		return (1);
	}

	fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd_out == -1)
	{
		perror("open destination");
		close(fd_in);
		free(buffer);
		return (1);
	}

	clock_gettime(CLOCK_MONOTONIC, &start);

	while ((bytes_read = read(fd_in, buffer, buf_size)) > 0)
	{
		bytes_written = write(fd_out, buffer, bytes_read);
		if (bytes_written != bytes_read)
		{
			perror("write");
			close(fd_in);
			close(fd_out);
			free(buffer);
			return (1);
		}
	}

	if (bytes_read == -1)
		perror("read");

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("copy_sys elapsed time: %.6f seconds\n", elapsed);

	close(fd_in);
	close(fd_out);
	free(buffer);
	return (0);
}
