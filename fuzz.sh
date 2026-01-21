#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Check if the 'tgt' argument was provided
if [ -z "$1" ]; then
    echo "Usage: $0 <target_name>"
    exit 1
fi

TGT=$1

CORPUS_DIR="./fuzz_input/$TGT"
REPORT_DIR="./fuzz_output/$TGT"
FUZZ_BINARY="./build/fuzz/src/$TGT/${TGT}_fuzz"

ninja -C build/fuzz

echo "Setting up directories for $TGT..."
mkdir -p "$CORPUS_DIR" "$REPORT_DIR"

echo "Starting honggfuzz for $TGT..."
../honggfuzz/honggfuzz -Q \
    -W "$REPORT_DIR" \
    --input "$CORPUS_DIR" \
    -- "$FUZZ_BINARY"
