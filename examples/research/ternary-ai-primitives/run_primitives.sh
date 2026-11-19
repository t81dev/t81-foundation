#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
CXX="${CXX:-c++}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

BIN="${TMPDIR}/ternary_ai_primitives"
"${CXX}" -std=c++23 -O3 -DNDEBUG -I"${ROOT_DIR}/include" \
  "${ROOT_DIR}/examples/research/ternary-ai-primitives/ternary_ai_primitives.cpp" \
  "${ROOT_DIR}/crypto/sha3.cpp" \
  -o "${BIN}"

OUT1="$("${BIN}")"
OUT2="$("${BIN}")"
R1="$(printf '%s\n' "${OUT1}" | grep '"retrieval_receipt"')"
R2="$(printf '%s\n' "${OUT2}" | grep '"retrieval_receipt"')"
if [[ "${R1}" != "${R2}" ]]; then
  echo "ERROR: non-deterministic retrieval receipt across consecutive runs" >&2
  exit 1
fi
printf '%s\n' "${OUT1}"
