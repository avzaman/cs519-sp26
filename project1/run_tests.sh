#!/bin/bash
# run_tests.sh
#
# This script compiles the test_app_helper program (if not already compiled)
# and then runs it with various buffer sizes.
#
# Usage:
#   ./run_tests.sh [buffer_size1 buffer_size2 ...]
#
# If no buffer sizes are provided as arguments, the script will use
# the default sizes: 256, 512, 1024, and 2048 bytes.

# Default buffer sizes to test if none are provided
if [ "$#" -eq 0 ]; then
    buffer_sizes=(256 512 1024 2048)
else
    buffer_sizes=("$@")
fi

# Compile the test_app_helper program if the binary doesn't exist.
if [ ! -f test_app_helper ]; then
    echo "Compiling test_app_helper.c..."
    gcc -o test_app_helper test_app_helper.c
    if [ $? -ne 0 ]; then
        echo "Compilation failed. Exiting."
        exit 1
    fi
fi

# Run the test_app_helper program for each buffer size.
for size in "${buffer_sizes[@]}"; do
    echo "--------------------------------------------------"
    echo "Running test with buffer size: ${size} bytes"
    ./test_app_helper ${size}
    echo ""
done

echo "All tests completed."

