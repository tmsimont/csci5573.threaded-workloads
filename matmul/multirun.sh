#!/bin/bash
# run the given problem x amount of times, waiting until program completion
# before running the next process
# usage: ./multirun [program index] [matrix size] [0 or 1 use lock] [num times]

for i in `seq 1 $4`
do
	if [ $1 -eq 0 ]
	then
		/usr/bin/time ./mmseq.out $2 $3
	fi
	if [ $1 -eq 1 ]
	then
		/usr/bin/time ./mmmtx.out $2 $3
	fi
	if [ $1 -eq 2 ]
	then
		/usr/bin/time ./mmsem.out $2 $3
	fi
	if [ $1 -eq 3 ]
	then
		/usr/bin/time ./mmts.out $2 $3
	fi
done
