#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
CXX="${CXX:-c++}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

BIN="${TMPDIR}/risk_triage_gate"
"${CXX}" -std=c++23 -O2 \
  -I"${ROOT_DIR}/include" \
  "${ROOT_DIR}/examples/research/deterministic-risk-triage/risk_triage_gate.cpp" \
  "${ROOT_DIR}/crypto/sha3.cpp" \
  -o "${BIN}"

OUT1="$("${BIN}")"
OUT2="$("${BIN}")"
if [[ "${OUT1}" != "${OUT2}" ]]; then
  echo "ERROR: non-deterministic output across consecutive runs" >&2
  exit 1
fi
printf '%s\n' "${OUT1}"
