#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
CXX="${CXX:-clang++}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

BIN="${TMPDIR}/heterogeneous_dualrail_demo"
SRC="${ROOT_DIR}/examples/research/heterogeneous-dualrail/heterogeneous_dualrail_demo.cpp"

if [[ "$(uname -s)" == "Darwin" ]]; then
  "${CXX}" -std=c++23 -O3 -DNDEBUG -mcpu=native -x objective-c++ -fobjc-arc \
    -I"${ROOT_DIR}/include" \
    "${SRC}" "${ROOT_DIR}/crypto/sha3.cpp" \
    -framework Metal -framework Foundation \
    -o "${BIN}"
else
  "${CXX}" -std=c++23 -O3 -DNDEBUG \
    -I"${ROOT_DIR}/include" \
    "${SRC}" "${ROOT_DIR}/crypto/sha3.cpp" \
    -o "${BIN}"
fi

OUT1="$("${BIN}" "$@")"
OUT2="$("${BIN}" "$@")"

DIG1="$(printf '%s\n' "${OUT1}" | grep '"proj_digest"')"
DIG2="$(printf '%s\n' "${OUT2}" | grep '"proj_digest"')"
if [[ "${DIG1}" != "${DIG2}" ]]; then
  echo "ERROR: non-deterministic digest across consecutive runs" >&2
  exit 1
fi
printf '%s\n' "${OUT1}"
