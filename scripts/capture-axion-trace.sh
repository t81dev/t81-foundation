#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
LOG_DIR="$BUILD_DIR/artifacts"
LOG_FILE="$LOG_DIR/axion_policy_runner.log"

mkdir -p "$LOG_DIR"
if [[ ! -x "$BUILD_DIR/axion_policy_runner" ]]; then
  echo "Building axion_policy_runner in $BUILD_DIR"
  (cd "$BUILD_DIR" && cmake --build . --parallel --target axion_policy_runner)
fi

echo "Running axion_policy_runner and capturing Axion trace to $LOG_FILE"
"$BUILD_DIR/axion_policy_runner" > "$LOG_FILE"
echo "Saved trace output:"
cat "$LOG_FILE"
