#!/bin/bash
#
# usage: ./launch-loops.sh [num loops] [priority]
for i in `seq 1 $1`
do
	nice -n $2 ./loopy.out &
done
