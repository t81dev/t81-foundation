#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
T81_BIN="${T81_BIN:-${ROOT_DIR}/build/t81}"
if [ ! -x "${T81_BIN}" ]; then
  echo "error: t81 binary not found at ${T81_BIN} (set T81_BIN)" >&2
  exit 1
fi

SRC_T81="${ROOT_DIR}/examples/research/exact-rational-lab/exact_rational_lab.t81"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

NODE_A="${TMPDIR}/node_a"
NODE_B="${TMPDIR}/node_b"
TRANSPORT="${TMPDIR}/transport"
mkdir -p "${NODE_A}" "${NODE_B}" "${TRANSPORT}"

echo "=== Node A: Compile, seal into CanonFS_A, verify, export, certify ==="
"${T81_BIN}" code build "${SRC_T81}" -o "${NODE_A}/program.tisc"
IMP_A=$("${T81_BIN}" canonfs import "${NODE_A}/program.tisc" --canonfs-root "${NODE_A}/canonfs" --json)
REF_A=$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read())["imported_objects"][0])' <<< "${IMP_A}")
"${T81_BIN}" canonfs fsck --verify-hashes --canonfs-root "${NODE_A}/canonfs" --json >/dev/null
"${T81_BIN}" canonfs export "${REF_A}" -o "${TRANSPORT}/program.tisc" --canonfs-root "${NODE_A}/canonfs" --json >/dev/null
"${T81_BIN}" determinism certify "${NODE_A}/program.tisc" --json > "${NODE_A}/cert.json"

echo "=== Node B: Import from transport into CanonFS_B, verify, materialize, certify ==="
IMP_B=$("${T81_BIN}" canonfs import "${TRANSPORT}/program.tisc" --canonfs-root "${NODE_B}/canonfs" --json)
REF_B=$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read())["imported_objects"][0])' <<< "${IMP_B}")
"${T81_BIN}" canonfs fsck --verify-hashes --canonfs-root "${NODE_B}/canonfs" --json >/dev/null
"${T81_BIN}" canonfs get "${REF_B}" -o "${NODE_B}/program.tisc" --canonfs-root "${NODE_B}/canonfs" --json >/dev/null
"${T81_BIN}" determinism certify "${NODE_B}/program.tisc" --json > "${NODE_B}/cert.json"

echo "=== Consensus Verification ==="
python3 - "${REF_A}" "${REF_B}" "${NODE_A}/cert.json" "${NODE_B}/cert.json" <<'PY'
import json, sys

ref_a, ref_b, cert_a_path, cert_b_path = sys.argv[1:5]
cert_a = json.load(open(cert_a_path, encoding="utf-8"))
cert_b = json.load(open(cert_b_path, encoding="utf-8"))

assert ref_a == ref_b, f"CanonRef mismatch: {ref_a} != {ref_b}"
assert cert_a["ok"] and cert_b["ok"], "Determinism certification failed"
assert cert_a["deterministic"] and cert_b["deterministic"], "Non-deterministic run"
for key in ("program_sha3_512", "run1_output_sha3_512", "run1_trace_sha3_512"):
    assert cert_a[key] == cert_b[key], f"{key} mismatch: {cert_a[key]} != {cert_b[key]}"

print("Consensus verified:")
print(f"  canon_ref:           {ref_a}")
print(f"  program_sha3_512:    {cert_a['program_sha3_512']}")
print(f"  run1_output_sha3_512:{cert_a['run1_output_sha3_512']}")
print(f"  run1_trace_sha3_512: {cert_a['run1_trace_sha3_512']}")
PY
