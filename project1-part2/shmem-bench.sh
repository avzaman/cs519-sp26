#/bin/bash

declare -a m=(1 3 5 7 9 11 13 15)

for n in "${m[@]}"
do
	echo "Running on cores 0-$n" | tee -a pipe-bench.log
	taskset --cpu-list 0-"$n" ./IPC-shmem 10000 | tee -a pipe-bench.log
done
