#!/bin/bash
set -e

# T81 'Go Broad' Killer Demo Reproduction Script
# This script runs the Llama-3.2-1B block demo and verifies the Axion trace.

BUILD_DIR="build"
DEMO_BIN="./${BUILD_DIR}/llama32_demo"

if [ ! -f "$DEMO_BIN" ]; then
    echo "Error: Demo binary not found. Please build the project first."
    exit 1
fi

RUN1_OUT="$(mktemp "${TMPDIR:-/tmp}/t81-llama-demo-run1.XXXXXX.txt")"
RUN2_OUT="$(mktemp "${TMPDIR:-/tmp}/t81-llama-demo-run2.XXXXXX.txt")"
TRACE1_OUT="$(mktemp "${TMPDIR:-/tmp}/t81-llama-trace1.XXXXXX.txt")"
TRACE2_OUT="$(mktemp "${TMPDIR:-/tmp}/t81-llama-trace2.XXXXXX.txt")"
trap 'rm -f "$RUN1_OUT" "$RUN2_OUT" "$TRACE1_OUT" "$TRACE2_OUT"' EXIT

echo "--- Step 1: Running Llama-3.2-1B Demo and capturing Axion Trace ---"
$DEMO_BIN > "$RUN1_OUT"

echo "--- Step 2: Extracting Axion trace for comparison ---"
grep "\[Axion\]" "$RUN1_OUT" > "$TRACE1_OUT"

echo "--- Step 3: Running second pass to verify determinism ---"
$DEMO_BIN > "$RUN2_OUT"
grep "\[Axion\]" "$RUN2_OUT" > "$TRACE2_OUT"

echo "--- Step 4: Comparing traces ---"
if diff "$TRACE1_OUT" "$TRACE2_OUT"; then
    echo "SUCCESS: Axion traces are bit-identical and reproducible!"
else
    echo "FAILURE: Axion traces differ between runs!"
    exit 1
fi

echo "--- Step 5: Verifying policy enforcement ---"
if grep -q "SUCCESS: Llama-3.2-1B block inference complete" "$RUN1_OUT"; then
    echo "SUCCESS: Policy enforced and inference succeeded."
else
    echo "FAILURE: Inference did not complete successfully."
    exit 1
fi

echo "--- Llama-3.2-1B Deterministic Story is REPRODUCIBLE and SHAREABLE ---"
