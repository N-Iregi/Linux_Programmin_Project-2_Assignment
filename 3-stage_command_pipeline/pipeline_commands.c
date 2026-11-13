#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NSTAGES 3
#define OUT_FILE "result.txt"
#define BUF_SIZE 4096
#define HEAD_LINES 5

static char *cmd_cat[] = {"cat", "/etc/passwd", NULL};
static char *cmd_cut[] = {"cut", "-d:", "-f7", NULL};
static char *cmd_sort[] = {"sort", "-u", NULL};
static char **stages[NSTAGES] = {cmd_cat, cmd_cut, cmd_sort};

/**
 * run_stage - wire a child's stdin/stdout to the pipes, then exec its command
 * @i: index of the stage (0 = cat, 1 = cut, 2 = sort)
 * @fds: the pipes; fds[k] connects stage k to stage k + 1 (or the parent)
 *
 * Never returns: it either execs or exits with 126/127.
 */
void run_stage(int i, int fds[][2])
{
	int j;

	if (i > 0 && dup2(fds[i - 1][0], STDIN_FILENO) == -1)
	{
		perror("dup2 stdin");
		_exit(126);
	}
	if (dup2(fds[i][1], STDOUT_FILENO) == -1)
	{
		perror("dup2 stdout");
		_exit(126);
	}
	for (j = 0; j < NSTAGES; j++)
	{
		close(fds[j][0]);
		close(fds[j][1]);
	}
	execvp(stages[i][0], stages[i]);
	perror(stages[i][0]);
	_exit(127);
}

/**
 * save_output - close the parent's unused pipe ends, then copy the last
 * pipe into OUT_FILE
 * @fds: the pipes; only the read end of the last one is kept open
 *
 * Return: 0 on success, -1 on error
 */
int save_output(int fds[][2])
{
	char buf[BUF_SIZE];
	ssize_t n, w, off;
	int i, ofd, ret = 0, rfd = fds[NSTAGES - 1][0];

	for (i = 0; i < NSTAGES; i++)
	{
		close(fds[i][1]);
		if (i != NSTAGES - 1)
			close(fds[i][0]);
	}
	ofd = open(OUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (ofd == -1)
	{
		perror("open " OUT_FILE);
		close(rfd);
		return (-1);
	}
	while (ret == 0 && (n = read(rfd, buf, sizeof(buf))) != 0)
	{
		if (n == -1 && errno != EINTR)
			ret = -1;
		for (off = 0; n > 0 && off < n && ret == 0; off += w)
		{
			w = write(ofd, buf + off, n - off);
			if (w == -1)
				ret = -1;
		}
	}
	if (ret == -1)
		perror("save_output");
	close(rfd);
	close(ofd);
	return (ret);
}

/**
 * show_summary - re-open OUT_FILE, print its first lines and the line count
 *
 * Return: 0 on success, -1 on error
 */
int show_summary(void)
{
	char buf[BUF_SIZE];
	ssize_t n, i;
	int fd, lines = 0;

	fd = open(OUT_FILE, O_RDONLY);
	if (fd == -1)
	{
		perror("open " OUT_FILE);
		return (-1);
	}
	printf("--- first %d lines of %s ---\n", HEAD_LINES, OUT_FILE);
	while ((n = read(fd, buf, sizeof(buf))) > 0)
	{
		for (i = 0; i < n; i++)
		{
			if (lines < HEAD_LINES)
				putchar(buf[i]);
			if (buf[i] == '\n')
				lines++;
		}
	}
	close(fd);
	printf("--- total lines: %d ---\n", lines);
	return (0);
}

/**
 * reap_children - waitpid() each child and print how it ended
 * @pids: pids of the NSTAGES children
 *
 * Return: 0 if every child exited with status 0, 1 otherwise
 */
int reap_children(pid_t pids[])
{
	int i, status, bad = 0;

	for (i = 0; i < NSTAGES; i++)
	{
		if (waitpid(pids[i], &status, 0) == -1)
		{
			perror("waitpid");
			bad = 1;
		}
		else if (WIFEXITED(status))
		{
			printf("child %d (%s, pid %d): exit status %d\n", i,
			       stages[i][0], (int)pids[i], WEXITSTATUS(status));
			bad |= WEXITSTATUS(status) != 0;
		}
		else if (WIFSIGNALED(status))
		{
			printf("child %d (%s, pid %d): killed by signal %d\n", i,
			       stages[i][0], (int)pids[i], WTERMSIG(status));
			bad = 1;
		}
	}
	return (bad);
}

/**
 * main - cat /etc/passwd | cut -d: -f7 | sort -u, captured by the parent
 *
 * Return: 0 on success, 1 on failure
 */
int main(void)
{
	int fds[NSTAGES][2], i;
	pid_t pids[NSTAGES];

	for (i = 0; i < NSTAGES; i++)
	{
		if (pipe(fds[i]) == -1)
		{
			perror("pipe");
			return (1);
		}
	}
	for (i = 0; i < NSTAGES; i++)
	{
		pids[i] = fork();
		if (pids[i] == -1)
		{
			perror("fork");
			return (1);
		}
		if (pids[i] == 0)
			run_stage(i, fds);
	}
	if (save_output(fds) == -1)
		return (1);
	if (show_summary() == -1)
		return (1);
	return (reap_children(pids));
}
