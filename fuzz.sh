#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# 1. Check if the 'tgt' argument was provided
if [ -z "$1" ]; then
    echo "Usage: $0 <target_name>"
    exit 1
fi

# 2. Assign the argument to the TGT variable
TGT=$1

# 3. Define directory paths
CORPUS_DIR="./fuzz_input/$TGT"
REPORT_DIR="./fuzz_output/$TGT"
FUZZ_BINARY="./build/fuzzinger/src/$TGT/${TGT}_fuzz"

# 4. Prepare directories (similar to prepare_dirs)
echo "Setting up directories for $TGT..."
mkdir -p "$CORPUS_DIR" "$REPORT_DIR"

# 5. Execute honggfuzz
echo "Starting honggfuzz for $TGT..."
../honggfuzz/honggfuzz -Q \
    -W "$REPORT_DIR" \
    --input "$CORPUS_DIR" \
    -- "$FUZZ_BINARY"
