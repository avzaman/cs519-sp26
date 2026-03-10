!#/bin/bash

$m = [1000 2000 3000 4000 5000 6000 7000 8000 9000 10000]

for n in m
do
	./IPC-pipe | tee pipe-bench.out
done
