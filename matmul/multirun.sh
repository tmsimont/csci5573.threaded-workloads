#!/bin/bash
# run the given problem x amount of times, waiting until program completion
# before running the next process
# usage: ./multirun.sh [num times]

for i in `seq 1 $1`
do
	./matmul.out -l0 -n6 -T
done
