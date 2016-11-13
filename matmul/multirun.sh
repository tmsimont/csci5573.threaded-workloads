#!/bin/bash
# run the given problem x amount of times, waiting until program completion
# before running the next process
# usage: ./multirun [program index] [matrix size] [0 or 1 use lock] [num times]

for i in `seq 1 $4`
do
	if [ $1 -eq 0 ]
	then
		./mmseq.out $2 $3
	fi
	if [ $1 -eq 1 ]
	then
		./mmmtx.out $2 $3
	fi
	if [ $1 -eq 2 ]
	then
		./mmsem.out $2 $3
	fi
done
