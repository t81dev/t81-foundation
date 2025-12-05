#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
LOG_DIR="$BUILD_DIR/artifacts"
POLICY_LOG="$LOG_DIR/axion_policy_runner.log"
GC_LOG="$LOG_DIR/axion_heap_compaction_trace.log"
BOUNDS_LOG="$LOG_DIR/vm_bounds_trace.log"

mkdir -p "$LOG_DIR"

if [[ ! -x "$BUILD_DIR/axion_policy_runner" ]]; then
  echo "Building axion_policy_runner in $BUILD_DIR"
  (cd "$BUILD_DIR" && cmake --build . --parallel --target axion_policy_runner)
fi

echo "Running axion_policy_runner and capturing Axion trace to $POLICY_LOG"
"$BUILD_DIR/axion_policy_runner" > "$POLICY_LOG"
echo "Saved policy runner output ($POLICY_LOG):"
cat "$POLICY_LOG"

for target in axion_heap_compaction_trace_test vm_bounds_trace_test canonfs_axion_trace_test; do
  log_var="$LOG_DIR/${target}.log"
  echo "Building $target"
  (cd "$BUILD_DIR" && cmake --build . --parallel --target "$target")
  echo "Running $target and capturing trace to $log_var"
  "$BUILD_DIR/$target" > "$log_var"
  echo "Saved trace output for $target ($log_var):"
  cat "$log_var"
done
