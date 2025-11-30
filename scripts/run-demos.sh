#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
CLI="${BUILD_DIR}/t81"

if [[ ! -x "${CLI}" ]]; then
  echo "error: ${CLI} not found; build the project first."
  exit 1
fi

run_demo() {
  local src="$1"
  local out
  out="$(mktemp --dry-run /tmp/$(basename "$src" .t81)-XXXXXX.tisc)" 2>/dev/null || out="/tmp/$(basename "$src" .t81).tisc"
  echo "=== Compiling ${src} ==="
  "${CLI}" compile "${src}" -o "${out}"
  echo "=== Running ${out} ==="
  "${CLI}" run "${out}"
  echo
  rm -f "${out}"
}

for src in examples/match_demo.t81 examples/data_types.t81 examples/fraction_demo.t81 examples/tensor_demo.t81 examples/bigint_demo.t81 examples/float_demo.t81 examples/string_demo.t81 examples/vector_demo.t81 examples/matrix_demo.t81 examples/cell_demo.t81 examples/quaternion_demo.t81; do
  if [[ -f "${src}" ]]; then
    run_demo "${src}"
  else
    echo "warning: ${src} not found; skipping"
  fi
done
