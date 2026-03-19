#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"

cmake --build "${BUILD_DIR}" --target \
  t81_vm_rfc0040_swar_test \
  jit_trace_equivalence_test \
  benchmark_runner \
  -j4

ctest --test-dir "${BUILD_DIR}" -R 't81_vm_rfc0040_swar_test' --output-on-failure
"./${BUILD_DIR}/jit_trace_equivalence_test"
"./${BUILD_DIR}/benchmarks/benchmark_runner" \
  --benchmark_filter='BM_(ComputeTAnd_Phase2A|ComputeTOr_Phase2A|ComputeTNot_Phase2A|ComputeTAnd_Phase2B_LUT|ComputeTOr_Phase2B_LUT|ComputeTNot_Phase2B_LUT|ComputeTAnd_Phase2C_SWAR|ComputeTOr_Phase2C_SWAR|ComputeTNot_Phase2C_SWAR)/(64|256)' \
  --benchmark_min_time=0.01s \
  --benchmark_format=json \
  --benchmark_out="${BUILD_DIR}/rfc0040_swar_bench.json"

echo "RFC-0040 SWAR evidence written to ${BUILD_DIR}/rfc0040_swar_bench.json"
