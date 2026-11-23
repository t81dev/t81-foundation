#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
CXX="${CXX:-clang++}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

BIN="${TMPDIR}/tisc_subgraph_dispatch"
SRC="${ROOT_DIR}/examples/research/tisc-subgraph-dispatch/tisc_subgraph_dispatch.cpp"

if [[ "$(uname -s)" == "Darwin" ]]; then
  "${CXX}" -std=c++23 -O3 -DNDEBUG -mcpu=native -x objective-c++ -fobjc-arc \
    -I"${ROOT_DIR}/include" \
    "${SRC}" "${ROOT_DIR}/isa/encoding.cpp" "${ROOT_DIR}/crypto/sha3.cpp" \
    -framework Metal -framework Foundation \
    -o "${BIN}"
else
  "${CXX}" -std=c++23 -O3 -DNDEBUG \
    -I"${ROOT_DIR}/include" \
    "${SRC}" "${ROOT_DIR}/isa/encoding.cpp" "${ROOT_DIR}/crypto/sha3.cpp" \
    -o "${BIN}"
fi

OUT1="$("${BIN}")"
OUT2="$("${BIN}")"
REC1="$(printf '%s\n' "${OUT1}" | grep '"execution_receipt"')"
REC2="$(printf '%s\n' "${OUT2}" | grep '"execution_receipt"')"
if [[ "${REC1}" != "${REC2}" ]]; then
  echo "ERROR: non-deterministic execution receipt across consecutive runs" >&2
  exit 1
fi
printf '%s\n' "${OUT1}"
