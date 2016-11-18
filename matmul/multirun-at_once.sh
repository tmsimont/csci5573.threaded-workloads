#!/bin/bash
# run the given problem x amount of times, launching all instances at once
if [ "$#" -ne 5 ]; then
  echo "usage: ./multirun-at_once [program index] [matrix size] [0 or 1 use lock] [num times] [outfile]"
	exit
fi
for i in `seq 1 $4`
do
	if [ $1 -eq 0 ]
	then
		./mmseq.out $2 $3 >> $5 &
	fi
	if [ $1 -eq 1 ]
	then
		./mmmtx.out $2 $3 >> $5 &
	fi
	if [ $1 -eq 2 ]
	then
		./mmsem.out $2 $3 >> $5 &
	fi
  if [ $1 -eq 3 ]
	then
	  ./mmts.out $2 $3 >> $5 &
	fi

done
