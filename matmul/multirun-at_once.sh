#!/bin/bash
# run the given problem x amount of times, launching all instances at once
# usage: ./multirun-at_once [num of processes to run] [outfile]
for i in `seq 1 $1`
do
	./matmul.out $1 >> $2 &
done
