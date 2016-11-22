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

### Mutex locks and Semaphores

Both **mm-pthreads.semaphores.cpp** and **mm-pthreads.mutex.cpp** 
contain the multi-threaded process described above.

The only difference is that one uses mutex locks, where the other
uses semaphores.


### multirun.sh

```
usage: ./multirun [program index] [matrix size] [0 or 1 use lock] [num times]
```

This will run the given program X amount of times sequentially.

The script will wait for one matrix multiply to complete before
starting another process

Valid indices for program selection:

	0 : Sequential
	1 : Mutex lock
	2 : Semaphore


### multirun-at_once.sh

```
 usage: ./multirun-at_once [program index] [matrix size] [0 or 1 use lock] [num times] [outfile]
```

This will launch the given program X amount of times all at once.

That means that X number of processes will be sent to the 
scheduler to fight for CPU resources.

See valid indices above for program selection.


### matmul.out

```
 usage: ./matmul.out -h
```

This will give you the usage of the program
