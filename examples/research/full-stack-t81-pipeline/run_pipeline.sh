#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
CXX="${CXX:-c++}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

if [[ ! -f "${ROOT_DIR}/build/libt81_vm.a" ]]; then
  cmake --build "${ROOT_DIR}/build" --target t81_vm t81_core t81_axion t81_isa -j8
fi

BIN="${TMPDIR}/full_stack_pipeline"
"${CXX}" -std=c++23 -O3 -DNDEBUG -DT81_DETERMINISTIC -DT81_HAS_NATIVE_INT128=1 \
  -I"${ROOT_DIR}/include" \
  "${ROOT_DIR}/examples/research/full-stack-t81-pipeline/full_stack_pipeline.cpp" \
  "${ROOT_DIR}/build/libt81_vm.a" \
  "${ROOT_DIR}/build/libt81_experimental.a" \
  "${ROOT_DIR}/build/libt81_axion.a" \
  "${ROOT_DIR}/build/libt81_isa.a" \
  "${ROOT_DIR}/build/libt81_core.a" \
  -o "${BIN}"

OUT1="$("${BIN}")"
OUT2="$("${BIN}")"
REC1="$(printf '%s\n' "${OUT1}" | grep '"execution_receipt"')"
REC2="$(printf '%s\n' "${OUT2}" | grep '"execution_receipt"')"
if [[ "${REC1}" != "${REC2}" ]]; then
  echo "ERROR: non-deterministic execution receipt across consecutive runs" >&2
  exit 1
fi
printf '%s\n' "${OUT1}"
