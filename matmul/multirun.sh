#!/bin/bash
# run the given problem x amount of times.
# usage: ./multirun [program index] [matrix size] [0 or 1 use lock] [num times]

for i in `seq 1 $4`
do
	if [ $1 -eq 0 ]
	then
		./mmseq.out $2 $3
	fi
	if [ $1 -eq 1 ]
	then
		./mm.out $2 $3
	fi
	if [ $1 -eq 2 ]
	then
		./mms.out $2 $3
	fi
done
