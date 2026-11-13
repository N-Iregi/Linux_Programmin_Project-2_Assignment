# Systems Programming & Concurrency Laboratory

This repository contains a collection of C programs and performance analyses exploring low-level operating system mechanics, POSIX system calls, I/O buffering, and multi-threaded synchronization.

## Overview of Projects

### 1. Three-Stage Process Pipeline Analysis

A C program that constructs a three-stage command pipeline (`cat /etc/passwd | cut -d: -f7 | sort -u`) using primitive system calls.

* **Core Concepts:** Process creation (`fork`, `clone`), Inter-Process Communication (IPC) via `pipe`, file descriptor duplication (`dup2`), and process replacement (`execvp`).
* **Key Findings:** Demonstrated how end-of-file (EOF) propagates through a pipeline only when all unneeded pipe write ends are strictly closed. Analyzed the exact system call sequence and data reduction (3181 bytes down to 49 bytes) using `strace`.

### 2. POSIX I/O vs. Standard I/O Performance

A comparative study of two file-copy utilities transferring a 100 MB payload to measure the overhead of kernel context switches.

* **Core Concepts:** Raw system calls (`open`, `read`, `write`) vs. Standard C Library buffered I/O (`fopen`, `fread`, `fwrite`).
* **Key Findings:** `copy_stdio` drastically outperforms `copy_sys` at smaller buffer sizes (e.g., 512 B) by minimizing transitions from user mode to kernel mode. At larger buffer sizes (64 KB), the performance converges as standard I/O bypasses its internal cache, shifting the bottleneck to physical hardware bandwidth.

### 3. Multi-Threaded Log Scanner

A concurrent search utility that counts specific pattern matches across multiple log files using a fixed-size thread pool and a shared work queue.

* **Core Concepts:** POSIX threads (`pthread_create`, `pthread_join`), mutex synchronization (`pthread_mutex_t`), and dynamic load balancing.
* **Key Findings:** Achieved a ~2x speedup with 8 threads over a single-threaded baseline. Highlighted the necessity of mutexes to prevent race conditions on the work queue, interleaved outputs in the report file, and lost updates to the grand total. Identified disk I/O, page cache states, and mutex contention as the primary limiters of linear concurrency scaling.

### 4. Concurrent Perfect Number Finder

A multi-threaded mathematical utility that finds all perfect numbers between 1 and 100,000 by dividing the search range across 8 threads.

* **Core Concepts:** Concurrency, lock contention mitigation, and workload distribution.
* **Key Findings:** Achieved a near-perfect ~8.02x speedup by utilizing private local arrays and counters for each thread, acquiring the global mutex only once at the end of execution to virtually eliminate lock contention. Also identified an inherent workload imbalance, as larger numbers at the end of the range require exponentially more CPU cycles to process than smaller numbers.

## Prerequisites

* Linux environment (tested on Ubuntu)
* GCC compiler
* `strace` (for system call profiling)

## Compilation & Execution

All projects adhere to strict Betty coding standards and should be compiled with the following flags to ensure maximum warning coverage and POSIX compliance:

```bash
# General compilation flags
gcc -Wall -Wextra -Werror -pedantic -std=gnu89 [source_file.c] -o [output_name]

# For multi-threaded projects (Log Scanner & Perfect Number Finder)
gcc -Wall -Wextra -Werror -pedantic -std=gnu89 -pthread [source_file.c] -o [output_name]

```
