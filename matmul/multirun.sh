#!/bin/bash
# run the given problem x amount of times, waiting until program completion
# before running the next process
# usage: ./multirun.sh [num times]

echo "N	Thread Priority	Main Priority	Nice	Policy	Sequential	Lock Type	total	avg	max"

# ---------------------------------------------------------------
# SCHED_RR
# ---------------------------------------------------------------

# low priority  -------------------------------------------------
# ----- mutex lock     ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l1 -n6 -o0 -m1 -p1
done
# ----- semaphore lock ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l2 -n6 -o0 -m1 -p1
done
# ----- spin lock      ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l3 -n6 -o0 -m1 -p1
done

# high priority --------------------------------------------------
# ----- mutex lock     ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l1 -n6 -o0 -m99 -p99
done
# ----- semaphore lock ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l2 -n6 -o0 -m99 -p99
done
# ----- spin lock      ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l3 -n6 -o0 -m99 -p99
done


# ---------------------------------------------------------------
# SCHED_FIFO
# ---------------------------------------------------------------

# low priority  -------------------------------------------------
# ----- mutex lock     ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l1 -n6 -o1 -m1 -p1
done
# ----- semaphore lock ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l2 -n6 -o1 -m1 -p1
done
# ----- spin lock      ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l3 -n6 -o1 -m1 -p1
done

# high priority --------------------------------------------------
# ----- mutex lock     ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l1 -n6 -o1 -m99 -p99
done
# ----- semaphore lock ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l2 -n6 -o1 -m99 -p99
done
# ----- spin lock      ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l3 -n6 -o1 -m99 -p99
done



# ---------------------------------------------------------------
# SCHED_OTHER
# ---------------------------------------------------------------

# low priority  -------------------------------------------------
# ----- mutex lock     ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l1 -n6 -o2 -i-20
done
# ----- semaphore lock ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l2 -n6 -o2 -i-20
done
# ----- spin lock      ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l3 -n6 -o2 -i-20
done

# high priority --------------------------------------------------
# ----- mutex lock     ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l1 -n6 -o2 -i19
done
# ----- semaphore lock ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l2 -n6 -o2 -i19
done
# ----- spin lock      ------------------------------------------
for i in `seq 1 $1`
do
	./matmul.out -l3 -n6 -o2 -i19
done

