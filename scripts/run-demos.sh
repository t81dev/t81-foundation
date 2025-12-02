#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
CLI="${BUILD_DIR}/t81"
WEIGHTS_MODEL="${WEIGHTS_MODEL:-}"

if [[ ! -x "${CLI}" ]]; then
  echo "error: ${CLI} not found; build the project first."
  exit 1
fi

run_demo() {
  local src="$1"
  shift
  local extra_args=("$@")
  local out
  out="$(mktemp --dry-run /tmp/$(basename "$src" .t81)-XXXXXX.tisc)" 2>/dev/null || out="/tmp/$(basename "$src" .t81).tisc"
  echo "=== Compiling ${src} ==="
  "${CLI}" compile "${src}" -o "${out}" "${extra_args[@]}"
  echo "=== Running ${out} ==="
  "${CLI}" run "${out}"
  echo
  rm -f "${out}"
}

DEMO_SOURCES=(
  examples/match_demo.t81
  examples/data_types.t81
  examples/fraction_demo.t81
  examples/tensor_demo.t81
  examples/bigint_demo.t81
  examples/float_demo.t81
  examples/string_demo.t81
  examples/vector_demo.t81
  examples/matrix_demo.t81
  examples/cell_demo.t81
  examples/quaternion_demo.t81
  examples/high_rank_tensor_demo.t81
  examples/graph_demo.t81
)

for src in "${DEMO_SOURCES[@]}"; do
  if [[ -f "${src}" ]]; then
    run_demo "${src}"
  else
    echo "warning: ${src} not found; skipping"
  fi
done

if [[ -n "${WEIGHTS_MODEL}" ]]; then
  if [[ -f "${WEIGHTS_MODEL}" ]]; then
    run_demo "examples/weights_load_demo.t81" "--weights-model" "${WEIGHTS_MODEL}"
  else
    echo "warning: WEIGHTS_MODEL=${WEIGHTS_MODEL} not found; skipping weights_demo"
  fi
else
  echo "info: set WEIGHTS_MODEL=path/to/model.t81w to include the weights_load_demo"
fi
