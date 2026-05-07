#!/bin/bash

BENCHMARK=./benchmark

if [ ! -x "$BENCHMARK" ]; then
    echo "benchmark binary not found or not executable"
    exit 1
fi

echo "=== Default mode ==="
$BENCHMARK > out_default.txt        # run without cooperative scheduling
cat out_default.txt

echo ""
echo "=== Cooperative mode ==="
$BENCHMARK --cooperative > out_coop.txt   # run with cooperative scheduling
cat out_coop.txt

echo ""
echo "=== Comparison ==="

# extract total iteration count from each run
default_iters=$(grep "total_iterations=" out_default.txt | awk -F= '{print $2}' | awk '{print $1}')
coop_iters=$(grep "total_iterations=" out_coop.txt    | awk -F= '{print $2}' | awk '{print $1}')

# extract average vruntime across threads: higher in cooperative mode confirms inflation
default_vrt=$(grep "^thread=" out_default.txt | awk -F'vruntime_ms=' '{sum+=$2; n++} END {printf "%.2f", sum/n}')
coop_vrt=$(grep    "^thread=" out_coop.txt    | awk -F'vruntime_ms=' '{sum+=$2; n++} END {printf "%.2f", sum/n}')

# extract average involuntary switches: lower in cooperative mode confirms less preemption
default_sw=$(grep "^thread=" out_default.txt | awk -F'involuntary_switches=' '{sum+=$2; n++} END {printf "%d", sum/n}')
coop_sw=$(grep    "^thread=" out_coop.txt    | awk -F'involuntary_switches=' '{sum+=$2; n++} END {printf "%d", sum/n}')

printf "%-35s %12s %12s\n" "Metric" "Default" "Cooperative"
printf "%-35s %12s %12s\n" "Total iterations"          "$default_iters" "$coop_iters"
printf "%-35s %12s %12s\n" "Avg vruntime at stop (ms)" "$default_vrt"   "$coop_vrt"
printf "%-35s %12s %12s\n" "Avg involuntary switches"  "$default_sw"    "$coop_sw"

echo ""
echo "Note: iteration and switch differences are most meaningful when"
echo "running alongside the matrix application (Step 5). Standalone runs"
echo "show vruntime inflation but minimal throughput difference because"
echo "there is no competing workload to benefit from the freed CPU time."
