#!/bin/bash

declare -a m=(1 3 5 7 9 11 13 15)
declare -a p=(1000 2000 3000 4000 5000 6000 7000 8000 9000)

for q in "${p[@]}"
do
        for n in "${m[@]}"
        do
                echo "Size $q Running on cores 0-$n" | tee -a pipe-bench.log
                taskset --cpu-list 0-"$n" ./IPC-pipe "$q" | tee -a pipe-bench.log
        done
done
