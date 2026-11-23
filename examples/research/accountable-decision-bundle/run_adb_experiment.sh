#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
T81_BIN="${T81_BIN:-${ROOT_DIR}/build/t81}"
DEMO_MAKER="${ROOT_DIR}/build/t81_make_assess_fixed_demo"
ADB_TOOL="${ROOT_DIR}/examples/research/accountable-decision-bundle/adb_tool.py"

if [[ ! -x "${T81_BIN}" || ! -x "${DEMO_MAKER}" ]]; then
  echo "error: build t81 and t81_make_assess_fixed_demo first" >&2
  exit 1
fi

TMPDIR="$(mktemp -d)"
trap 'rm -rf "${TMPDIR}"' EXIT

NODE_A="${TMPDIR}/node_a"
NODE_B="${TMPDIR}/node_b"
KEYS="${TMPDIR}/keys"
mkdir -p "${NODE_A}/canonfs" "${NODE_A}/model" "${NODE_B}/canonfs" "${NODE_B}/model" "${KEYS}"

# Generate ephemeral Ed25519 keypair for Node A producer attestation
openssl genpkey -algorithm ED25519 -out "${KEYS}/node_a.priv.pem" 2>/dev/null
openssl pkey -in "${KEYS}/node_a.priv.pem" -pubout -out "${KEYS}/node_a.pub.pem" 2>/dev/null

# Prepare identical demo model & policy on Node A and Node B
"${DEMO_MAKER}" "${NODE_A}/model" >/dev/null
cp "${NODE_A}/model/assess-fixed-demo.t81w" "${NODE_B}/model/assess-fixed-demo.t81w"
MODEL_HASH="$("${T81_BIN}" determinism hash "${NODE_A}/model/assess-fixed-demo.t81w" | awk 'NR==1{print $1}')"

cat > "${NODE_A}/policy.apl" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:${MODEL_HASH}"])
  (require-axion-event (reason "task:assess_fixed.v1")))
EOF
cp "${NODE_A}/policy.apl" "${NODE_B}/policy.apl"

echo "=== [1/5] Node A: Run assess-fixed, emit signed RFC-00D4 bundle, seal CanonFS ==="
"${T81_BIN}" ai task assess-fixed \
  --model assess-fixed-demo \
  --model-file "${NODE_A}/model/assess-fixed-demo.t81w" \
  --policy "${NODE_A}/policy.apl" \
  --canonfs-root "${NODE_A}/canonfs" \
  --mode strict_deterministic \
  --input "greet hello" > "${NODE_A}/task.json"

python3 "${ADB_TOOL}" emit \
  --task-json "${NODE_A}/task.json" \
  --policy "${NODE_A}/policy.apl" \
  --signing-key "${KEYS}/node_a.priv.pem" \
  --public-key-uri "file://${KEYS}/node_a.pub.pem" \
  --principal-id "node-a-operator@t81.local" \
  --out "${TMPDIR}/bundle.json"

"${T81_BIN}" canonfs fsck --verify-hashes --canonfs-root "${NODE_A}/canonfs" --json >/dev/null

echo "=== [2/5] Node B: Offline signature, policy, and bit-identical replay verification ==="
python3 "${ADB_TOOL}" verify \
  --bundle "${TMPDIR}/bundle.json" \
  --public-key "${KEYS}/node_a.pub.pem" \
  --trusted-policy "${NODE_B}/policy.apl" \
  --replay \
  --t81-bin "${T81_BIN}" \
  --model-file "${NODE_B}/model/assess-fixed-demo.t81w" \
  --input-text "greet hello" \
  --canonfs-root "${NODE_B}/canonfs"
"${T81_BIN}" canonfs fsck --verify-hashes --canonfs-root "${NODE_B}/canonfs" --json >/dev/null

echo "=== [3/5] Adversarial: Mid-flight policy revocation ==="
cat > "${NODE_B}/revoked_policy.apl" <<'EOF'
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"])
  (require-axion-event (reason "task:assess_fixed.v1")))
EOF
if python3 "${ADB_TOOL}" verify \
  --bundle "${TMPDIR}/bundle.json" \
  --public-key "${KEYS}/node_a.pub.pem" \
  --trusted-policy "${NODE_B}/revoked_policy.apl"; then
  echo "ERROR: revoked policy unexpectedly verified" >&2
  exit 1
fi
if "${T81_BIN}" ai task assess-fixed \
  --model assess-fixed-demo \
  --model-file "${NODE_B}/model/assess-fixed-demo.t81w" \
  --policy "${NODE_B}/revoked_policy.apl" \
  --canonfs-root "${NODE_B}/canonfs" \
  --mode strict_deterministic \
  --input "greet hello" >/dev/null 2>&1; then
  echo "ERROR: revoked policy unexpectedly executed compute" >&2
  exit 1
fi
echo "PASS: Mid-flight policy revocation blocked both offline verification and VM execution."

echo "=== [4/5] Adversarial: 1-byte bundle tampering ==="
python3 - "${TMPDIR}/bundle.json" "${TMPDIR}/tampered_bundle.json" <<'PY'
import json, sys
b = json.load(open(sys.argv[1]))
b["meaning"]["parameters"]["decision"] = "DENY"
open(sys.argv[2], "w").write(json.dumps(b, indent=2))
PY
if python3 "${ADB_TOOL}" verify \
  --bundle "${TMPDIR}/tampered_bundle.json" \
  --public-key "${KEYS}/node_a.pub.pem"; then
  echo "ERROR: tampered bundle unexpectedly verified" >&2
  exit 1
fi
echo "PASS: Tampered bundle rejected by Ed25519 signature check."

echo "=== [5/5] Adversarial: 1-byte CanonFS object tampering ==="
BLK_FILE="$(find "${NODE_B}/canonfs/objects" -type f | head -n 1)"
printf "X" >> "${BLK_FILE}"
if "${T81_BIN}" canonfs fsck --verify-hashes --canonfs-root "${NODE_B}/canonfs" --json >/dev/null; then
  echo "ERROR: tampered CanonFS object passed fsck" >&2
  exit 1
fi
echo "PASS: CanonFS fsck --verify-hashes caught 1-byte object tampering."
echo "All RFC-00D4 Accountable Decision Bundle adversarial checks PASSED."
