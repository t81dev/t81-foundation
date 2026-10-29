#!/usr/bin/env bash
set -euo pipefail

echo "========================================================================"
echo "Running T81 Exact Rational Conversion Prototype Suite"
echo "========================================================================"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

cd "${REPO_ROOT}"

COMPILER="g++"
CXXFLAGS="-std=c++23 -O3 -Wall -Wextra"

echo "[1/3] Compiling C++23 Prototype Test Runner and Benchmark..."
${COMPILER} ${CXXFLAGS} experiments/exact_rational_prototype/test_prototype.cpp -o experiments/exact_rational_prototype/test_prototype
${COMPILER} ${CXXFLAGS} experiments/exact_rational_prototype/benchmark_prototype.cpp -o experiments/exact_rational_prototype/benchmark_prototype

echo "[2/3] Executing Corpus & Boundary Conformance Tests..."
./experiments/exact_rational_prototype/test_prototype

echo "[3/3] Running Performance Benchmarks..."
./experiments/exact_rational_prototype/benchmark_prototype

echo "Experiment execution complete."
