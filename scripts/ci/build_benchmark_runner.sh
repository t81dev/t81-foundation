#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${T81_BENCH_BUILD_DIR:-build}"
CMAKE_GENERATOR="${T81_BENCH_CMAKE_GENERATOR:-Ninja}"
CMAKE_BUILD_TYPE="${T81_BENCH_CMAKE_BUILD_TYPE:-Release}"
CMAKE_C_COMPILER="${T81_BENCH_C_COMPILER:-}"
CMAKE_CXX_COMPILER="${T81_BENCH_CXX_COMPILER:-}"
CMAKE_CXX_FLAGS="${T81_BENCH_CXX_FLAGS:-}"
EXTRA_ARGS="${T81_BENCH_EXTRA_CMAKE_ARGS:-}"

cmake_args=(
  -S .
  -B "${BUILD_DIR}"
  -G "${CMAKE_GENERATOR}"
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
  -DT81_BUILD_BENCHMARKS=ON
  -DT81_BUILD_TESTS=OFF
  -DT81_BUILD_EXAMPLES=OFF
  -DT81_BUILD_FUZZ_TESTS=OFF
)

if [[ -n "${CMAKE_C_COMPILER}" ]]; then
  cmake_args+=("-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
fi

if [[ -n "${CMAKE_CXX_COMPILER}" ]]; then
  cmake_args+=("-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
fi

if [[ -n "${CMAKE_CXX_FLAGS}" ]]; then
  cmake_args+=("-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
fi

if [[ -n "${EXTRA_ARGS}" ]]; then
  # Intentionally split on shell words so workflows can pass multiple flags.
  read -r -a extra_args <<< "${EXTRA_ARGS}"
  cmake_args+=("${extra_args[@]}")
fi

echo "[bench-build] configuring benchmark runner in ${BUILD_DIR}"
cmake "${cmake_args[@]}"

echo "[bench-build] building benchmark_runner"
cmake --build "${BUILD_DIR}" --target benchmark_runner --parallel
