#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build}"

# Fast deterministic regression gate covering JIT/VM/repro/BigInt/tritwise surfaces.
regex='(jit|vm_|determinism|bigint|trace|canonical|repro|tritwise_backend_equivalence)'

ctest --test-dir "${build_dir}" --output-on-failure -R "${regex}"
