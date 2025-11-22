#!/usr/bin/env bash
set -e

HARNESS="tests/harness/harness.py"

echo "Running T81 Test Harness..."
python3 "$HARNESS" run
echo "All tests passed."
