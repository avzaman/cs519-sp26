#!/bin/bash

declare -a c=(1 3 5 7 9 11 13 15)

mkdir -p mesg-perf-data

for n in "${c[@]}"
do
        echo "Running Perf on cores 0-$n"
        perf record -a -g taskset --cpu-list 0-"$n" ./IPC-shmem 10000
	mkdir -p mesg-perf-data/mesg-perf-"$n"
	mv ./perf.data ./mesg-perf-data/mesg-perf-"$n"/perf.data
done
