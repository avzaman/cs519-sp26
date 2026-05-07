#!/bin/bash

MATRIX=./matrix
BENCH=./benchmark

if [ ! -x "$MATRIX" ] || [ ! -x "$BENCH" ]; then
    echo "error: compile matrix and benchmark first"
    exit 1
fi

run_case() {
    local label="$1"
    local bench_mode="$2"   # "none", "cooperative", or "default"
    local out_matrix="out_matrix_${label}.txt"
    local out_bench="out_bench_${label}.txt"

    echo "--- $label ---"

    if [ "$bench_mode" != "none" ]; then
        # launch benchmark in background before matrix so they run concurrently
        $BENCH $( [ "$bench_mode" = "cooperative" ] && echo "--cooperative" ) \
            > "$out_bench" 2>&1 &
        BENCH_PID=$!
    fi

    # launch matrix and wait for it to finish; its completion time is the key metric
    $MATRIX > "$out_matrix" 2>&1
    MATRIX_EXIT=$?

    if [ "$bench_mode" != "none" ]; then
        # matrix is done; stop the benchmark cleanly so it can write its metrics
        kill -SIGTERM $BENCH_PID 2>/dev/null
        wait $BENCH_PID 2>/dev/null
    fi

    if [ $MATRIX_EXIT -ne 0 ]; then
        echo "matrix failed; check $out_matrix"
        return
    fi

    cat "$out_matrix"
    [ "$bench_mode" != "none" ] && cat "$out_bench"
}

# case 1: matrix alone, establishes the uncontended baseline
run_case "alone"       "none"

# case 2: matrix alongside cooperative benchmark (spinner yields CPU to matrix)
run_case "coop"        "cooperative"

# case 3: matrix alongside non-cooperative benchmark (spinner competes for CPU)
run_case "nocoop"      "default"

echo ""
echo "====== Summary ======"
printf "%-30s %12s %12s %12s\n" "Metric" "Alone" "Coop" "No-Coop"

# matrix time
t_alone=$(  grep "time_seconds" out_matrix_alone.txt   | awk -F= '{print $2}' | awk '{print $1}')
t_coop=$(   grep "time_seconds" out_matrix_coop.txt    | awk -F= '{print $2}' | awk '{print $1}')
t_nocoop=$( grep "time_seconds" out_matrix_nocoop.txt  | awk -F= '{print $2}' | awk '{print $1}')
printf "%-30s %12s %12s %12s\n" "Matrix time (s)"  "$t_alone" "$t_coop" "$t_nocoop"

# matrix GFLOPS
g_alone=$(  grep "gflops" out_matrix_alone.txt   | awk -F'gflops=' '{print $2}' | awk '{print $1}')
g_coop=$(   grep "gflops" out_matrix_coop.txt    | awk -F'gflops=' '{print $2}' | awk '{print $1}')
g_nocoop=$( grep "gflops" out_matrix_nocoop.txt  | awk -F'gflops=' '{print $2}' | awk '{print $1}')
printf "%-30s %12s %12s %12s\n" "Matrix GFLOPS" "$g_alone" "$g_coop" "$g_nocoop"

# benchmark total iterations (n/a for alone case)
i_coop=$(   grep "total_iterations" out_bench_coop.txt   | awk -F= '{print $2}' | awk '{print $1}')
i_nocoop=$( grep "total_iterations" out_bench_nocoop.txt | awk -F= '{print $2}' | awk '{print $1}')
printf "%-30s %12s %12s %12s\n" "Benchmark iterations" "n/a" "$i_coop" "$i_nocoop"

# benchmark CPU time consumed (lower in coop = more CPU given to matrix)
c_coop=$(   grep "total_cpu_ms" out_bench_coop.txt   | awk -F= '{print $2}' | awk '{print $1}')
c_nocoop=$( grep "total_cpu_ms" out_bench_nocoop.txt | awk -F= '{print $2}' | awk '{print $1}')
printf "%-30s %12s %12s %12s\n" "Benchmark CPU time (ms)" "n/a" "$c_coop" "$c_nocoop"
