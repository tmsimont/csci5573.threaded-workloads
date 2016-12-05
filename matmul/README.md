# Matrix multiply

This directory contains tests performed on various versions of
a matrix-multiplication script.

The original script was written by Gita Alaghband and Lan Vu.

The sequential script performs n * n dot products, each involving
n summations. The result of the procedure is an n * n matrix that
is the product of two n * n matrices.

The user inputs the size, n, of the matrix. The program creates
two n * n matrix. Each n * n matrix is just n * n 9's.

For example, input 3 and get this matrix 2 times in memory:

```
9 9 9
9 9 9
9 9 9
```

That matrix is then muliplied by itself, to form a new 3 * 3
size result matrix.


## Pthreads

We use a completely unreasonable amount of threads to perform 
the matrix multiplication.

We launch n^3 threads. One for each summation operation during
the dot products for each n * n cells in the result matrix.

This makes no sense for a practical application, but it gives
us a situation where many threads are launched by one process,
and then compete for resources. This is exactly what we need
to expiriment with the Linux scheduler

### Varied scheduling policy, lock options

See `./matmul.out -h`
```
Usage: ./matmul.out [options]
Options are:
	-n<size> Set matrix size
	-p<prio> Set scheduling priority in
	         thread attributes object
	-i<nice> Set nice value
	-m<prio> Set scheduling priority on
	         main thread before pthread_create() call
	-o<idx> Set scheduling policy:
	           0 : SCHED_RR
	           1 : SCHED_FIFO
	           2 : SCHED_OTHER
	-S       Don't use threads, perform sequentially 
	-D       Print debug info 
	-l<lock> Set locking mechanism used during matrix
	         updates. Options are:
	           0 : No lock
	           1 : Mutex
	           2 : Semaphore
	           3 : Spinlock
	-t<tmr>  Set the thread lock acquisition timer type
	           0 : CLOCK_REALTIME 
	           1 : CLOCK_MONOTONIC 
	           2 : CLOCK_MONOTONIC_RAW 
	           3 : (DEFAULT) CLOCK_THREAD_CPUTIME_ID 
	-T       Disable timing output

```


### multirun.sh

```
usage: ./multirun [times to run each permutation] > [outfile.tsv]
```

This will run the given program X amount of times for each permutation
of program settings. This is how results are collected for our expirment.
