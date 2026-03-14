#!/bin/bash

declare -a c=(1 3 5 7 9 11 13 15)

mkdir -p pipe-perf-data

for n in "${c[@]}"
do
        echo "Running Perf on cores 0-$n"
        perf record -a -g taskset --cpu-list 0-"$n" ./IPC-pipe 10000
	mkdir -p pipe-perf-data/pipe-perf-"$n"
	mv perf.data pipe-perf-data/pipe-perf-"$n"/perf.data
done
