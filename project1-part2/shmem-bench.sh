#!/bin/bash

declare -a m=(1 3 5 7 9 11 13 15)

declare -a p=(1000 2000 3000 4000 5000 6000 7000 8000 9000)

for q in "${p[@]}"
do
	for n in "${m[@]}"
	do
		echo "Matrix $q Running on cores 0-$n" | tee -a shmem-bench.log
		taskset --cpu-list 0-"$n" ./IPC-shmem "$q" | tee -a shmem-bench.log
	done
done
