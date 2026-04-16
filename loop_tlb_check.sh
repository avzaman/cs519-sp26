#!/bin/bash

gcc -o ./test_program ./test_program.c

ITERS=1000
SMALL_PAGES=1000
LARGE_PAGES=100000

OUTPUT_SMALL="results_${SMALL_PAGES}pages.tsv"
OUTPUT_LARGE="results_${LARGE_PAGES}pages.tsv"

# write headers
echo -e "Iteration\tPhysical Pages\tExtents\tTLB Savings\tReduction %" > $OUTPUT_SMALL
echo -e "Iteration\tPhysical Pages\tExtents\tTLB Savings\tReduction %" > $OUTPUT_LARGE

run_experiment() {
    local pages=$1
    local iters=$2
    local outfile=$3

    echo "Running $iters iterations with $pages pages..."

    for i in $(seq 1 $iters); do
        ./test_program $pages > /dev/null 2>&1

        pgcount=$(sudo dmesg | grep -o "total pages = [0-9]*" | tail -1 | grep -o "[0-9]*")
        extent_c=$(sudo dmesg | grep -o "total extents = [0-9]*" | tail -1 | grep -o "[0-9]*")

        if [[ -z "$pgcount" || -z "$extent_c" ]]; then
            echo "Warning: failed to read dmesg on iteration $i, skipping"
            continue
        fi

        savings=$((pgcount - extent_c))
        pct=$(echo "scale=2; $savings * 100 / $pgcount" | bc)

        echo -e "$i\t$pgcount\t$extent_c\t$savings\t$pct" >> $outfile

        # progress every 100 iterations
        if (( i % 100 == 0 )); then
            echo "  $i / $iters done"
        fi
    done

    echo "Done. Results written to $outfile"
}

run_experiment $SMALL_PAGES $ITERS $OUTPUT_SMALL
run_experiment $LARGE_PAGES $ITERS $OUTPUT_LARGE
