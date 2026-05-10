#!/bin/bash

MATRIX=./matrix
BENCH=./benchmark
PENALTIES="1 5 10 20 30 60"
RESULTS=penalty_results.csv

if [ ! -x "$MATRIX" ] || [ ! -x "$BENCH" ]; then
    echo "error: compile matrix and benchmark first"
    exit 1
fi

echo "penalty_secs,matrix_time_s,matrix_gflops,bench_iters" > $RESULTS

# matrix alone as baseline
$MATRIX > out_penalty_matrix_alone.txt 2>&1
t_alone=$(grep "time_seconds" out_penalty_matrix_alone.txt | awk -F= '{print $2}' | awk '{print $1}')
g_alone=$(grep "gflops"       out_penalty_matrix_alone.txt | awk -F'gflops=' '{print $2}' | awk '{print $1}')
echo "0(alone),$t_alone,$g_alone,0" >> $RESULTS

for p in $PENALTIES; do
    echo "running penalty=${p}s..."

    $BENCH --cooperative --penalty $p > out_penalty_bench_${p}.txt 2>&1 &
    BPID=$!
    $MATRIX > out_penalty_matrix_${p}.txt 2>&1
    kill -SIGTERM $BPID; wait $BPID 2>/dev/null

    t=$(grep "time_seconds"     out_penalty_matrix_${p}.txt | awk -F= '{print $2}' | awk '{print $1}')
    g=$(grep "gflops"           out_penalty_matrix_${p}.txt | awk -F'gflops=' '{print $2}' | awk '{print $1}')
    i=$(grep "total_iterations" out_penalty_bench_${p}.txt  | awk -F= '{print $2}' | awk '{print $1}')

    echo "$p,$t,$g,$i" >> $RESULTS
done

echo ""
echo "results written to $RESULTS"
echo ""

printf "%-14s %-16s %-12s %-14s\n" "penalty(s)" "matrix_time(s)" "gflops" "bench_iters"
while IFS=, read -r p t g i; do
    [ "$p" = "penalty_secs" ] && continue
    printf "%-14s %-16s %-12s %-14s\n" "$p" "$t" "$g" "$i"
done < $RESULTS
