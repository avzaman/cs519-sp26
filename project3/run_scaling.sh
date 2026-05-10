#!/bin/bash

MATRIX=./matrix
BENCH=./benchmark
THREAD_COUNTS="1 2 4 8 12 16"
RESULTS=scaling_results.csv

if [ ! -x "$MATRIX" ] || [ ! -x "$BENCH" ]; then
    echo "error: compile matrix and benchmark first"
    exit 1
fi

# csv header
echo "spinners,mode,matrix_time_s,matrix_gflops,bench_iters,bench_elapsed_s,bench_iters_per_s,bench_cpu_ms" \
    > $RESULTS

run_case() {
    local n="$1"
    local mode="$2"
    local out_m="out_matrix_${mode}_${n}.txt"
    local out_b="out_bench_${mode}_${n}.txt"
    local coop_flag=""
    [ "$mode" = "coop" ] && coop_flag="--cooperative"

    $BENCH $coop_flag --threads $n > "$out_b" 2>&1 &
    BPID=$!
    $MATRIX > "$out_m" 2>&1
    kill -SIGTERM $BPID; wait $BPID 2>/dev/null

    local t_mat=$(grep "time_seconds" "$out_m" | awk -F= '{print $2}' | awk '{print $1}')
    local g_mat=$(grep "gflops"       "$out_m" | awk -F'gflops=' '{print $2}' | awk '{print $1}')
    local iters=$(grep "total_iterations" "$out_b" | awk -F= '{print $2}' | awk '{print $1}')
    local etime=$(grep "elapsed_s"        "$out_b" | awk -F= '{print $2}' | awk '{print $1}')
    local cpums=$(grep "total_cpu_ms"     "$out_b" | awk -F= '{print $2}' | awk '{print $1}')
    local ips=$(awk "BEGIN {if ($etime > 0) printf \"%.0f\", $iters / $etime; else print 0}")

    echo "$n,$mode,$t_mat,$g_mat,$iters,$etime,$ips,$cpums" >> $RESULTS
}

# baseline: matrix alone, no spinner
$MATRIX > out_matrix_alone.txt 2>&1
t_alone=$(grep "time_seconds" out_matrix_alone.txt | awk -F= '{print $2}' | awk '{print $1}')
g_alone=$(grep "gflops"       out_matrix_alone.txt | awk -F'gflops=' '{print $2}' | awk '{print $1}')
echo "0,alone,$t_alone,$g_alone,0,0,0,0" >> $RESULTS

for n in $THREAD_COUNTS; do
    echo "running: $n spinner threads..."
    run_case $n "coop"
    run_case $n "nocoop"
done

echo ""
echo "results written to $RESULTS"
echo ""

# print summary table to stdout
printf "%-10s %-10s %-16s %-12s %-14s %-16s\n" \
    "spinners" "mode" "matrix_time(s)" "gflops" "bench_iters" "bench_iter/s"
while IFS=, read -r n mode t_mat g_mat iters etime ips cpums; do
    [ "$n" = "spinners" ] && continue   /* skip header row */
    printf "%-10s %-10s %-16s %-12s %-14s %-16s\n" \
        "$n" "$mode" "$t_mat" "$g_mat" "$iters" "$ips"
done < $RESULTS
