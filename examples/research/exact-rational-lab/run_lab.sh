#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
T81_BIN="${T81_BIN:-${ROOT_DIR}/build-audit/t81}"
if [ ! -x "${T81_BIN}" ]; then
  T81_BIN="${ROOT_DIR}/build/t81"
fi
if [ ! -x "${T81_BIN}" ]; then
  echo "error: t81 binary not found (set T81_BIN or build ./build/t81)" >&2
  exit 1
fi

LAB_DIR="${ROOT_DIR}/examples/research/exact-rational-lab"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

CROOT="${TMPDIR}/canonfs_root"
TISC_OUT="${TMPDIR}/exact_rational_lab.tisc"
CERT_OUT="${TMPDIR}/cert.json"

echo "1. Compiling exact_rational_lab.t81 -> TISC..."
"${T81_BIN}" code build "${LAB_DIR}/exact_rational_lab.t81" -o "${TISC_OUT}"

echo "2. Running 3-way determinism check..."
"${T81_BIN}" determinism multi-run "${TISC_OUT}" --count 3 --json

echo "3. Emitting reproducibility certificate..."
"${T81_BIN}" determinism certify "${TISC_OUT}" --json > "${CERT_OUT}"

echo "4. Sealing source, TISC binary, and certificate into isolated CanonFS..."
"${T81_BIN}" canonfs import "${LAB_DIR}/exact_rational_lab.t81" --canonfs-root "${CROOT}" --json >/dev/null
"${T81_BIN}" canonfs import "${TISC_OUT}" --canonfs-root "${CROOT}" --json >/dev/null
"${T81_BIN}" canonfs import "${CERT_OUT}" --canonfs-root "${CROOT}" --json >/dev/null

echo "5. Verifying CanonFS cryptographic integrity (fsck --verify-hashes)..."
"${T81_BIN}" canonfs fsck --verify-hashes --canonfs-root "${CROOT}" --json

echo "Zero-Drift Exact Rational Lab completed cleanly."
