#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
CXX="${CXX:-clang++}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

BIN="${TMPDIR}/bitnet_verifiable_token"
SRC="${ROOT_DIR}/examples/research/bitnet-verifiable-token/bitnet_verifiable_token.cpp"

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
if [[ "${OUT1}" != "${OUT2}" ]]; then
  echo "ERROR: non-deterministic output across consecutive runs" >&2
  exit 1
fi
printf '%s\n' "${OUT1}"
