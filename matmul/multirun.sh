#!/bin/bash
# run the given problem x amount of times, waiting until program completion
# before running the next process
# usage: ./multirun.sh [num times]

for i in `seq 1 $1`
do
	./matmul.out -l3 -n6 -o1 -m1 -p1
done
