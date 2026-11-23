#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
T81_BIN="${T81_BIN:-${ROOT_DIR}/build/t81}"
SIDECAR="${ROOT_DIR}/examples/research/agent-receipt-sidecar/t81_receipt_sidecar.py"
GAUNTLET="${ROOT_DIR}/examples/research/agent-receipt-sidecar/run_agent_gauntlet.py"
MODEL_EXPORTED="${ROOT_DIR}/models/tiny-random-llama-exported.safetensors"
MODEL_SOURCE="${ROOT_DIR}/models/tiny-random-llama/model.safetensors"

if [[ ! -x "${T81_BIN}" ]]; then
  echo "error: t81 binary not found at ${T81_BIN}" >&2
  exit 1
fi
if [[ ! -f "${MODEL_EXPORTED}" || ! -f "${MODEL_SOURCE}" ]]; then
  echo "error: expected repo demo models not found under models/" >&2
  exit 1
fi

echo "=== [1/2] Dogfooding T81 MCP Receipt Sidecar on Repo Model Artifacts ==="
python3 - "${T81_BIN}" "${SIDECAR}" "${MODEL_EXPORTED}" "${MODEL_SOURCE}" <<'PY'
import hashlib, json, subprocess, sys, tempfile
from pathlib import Path

t81_bin, sidecar, model_exported, model_source = sys.argv[1:5]

proc = subprocess.Popen(
    [sys.executable, sidecar],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    bufsize=1,
)

req_id = 0
def call_tool(name, arguments):
    global req_id
    req_id += 1
    req = {"jsonrpc": "2.0", "id": req_id, "method": "tools/call", "params": {"name": name, "arguments": arguments}}
    proc.stdin.write(json.dumps(req) + "\n")
    proc.stdin.flush()
    resp = json.loads(proc.stdout.readline())
    assert "error" not in resp, resp
    return resp["result"]["structured"]

try:
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        priv, pub = tmp / "ci.priv.pem", tmp / "ci.pub.pem"
        croot = tmp / "canonfs"
        croot.mkdir()
        subprocess.run(["openssl", "genpkey", "-algorithm", "ED25519", "-out", str(priv)], check=True, capture_output=True)
        subprocess.run(["openssl", "pkey", "-in", str(priv), "-pubout", "-out", str(pub)], check=True, capture_output=True)

        # 1. AI-to-AI model_diff via MCP sidecar
        diff_same = call_tool("t81.model_diff", {"t81_bin": t81_bin, "lhs": model_exported, "rhs": model_exported})
        assert diff_same.get("identical") is True, diff_same
        diff_other = call_tool("t81.model_diff", {"t81_bin": t81_bin, "lhs": model_exported, "rhs": model_source})
        assert diff_other.get("identical") is False, diff_other

        # 2. Gate & seal repo model artifact under pinned .apl policy
        model_sha = "sha3-512:" + hashlib.sha3_512(Path(model_exported).read_bytes()).hexdigest()
        policy = tmp / "repo_model_policy.apl"
        policy.write_text(
            f'(policy\n  (tier 1)\n  (allowed-tensor-hashes ["{model_sha}"])\n  (require-axion-event (reason "ci:model_review")))\n',
            encoding="utf-8",
        )
        sealed = call_tool("t81.gate_and_seal", {
            "t81_bin": t81_bin,
            "artifact_path": model_exported,
            "policy_path": str(policy),
            "canonfs_root": str(croot),
            "signing_key": str(priv),
            "public_key_uri": f"file://{pub}",
            "principal_id": "t81-ci-agent@local",
            "action_type": "ci.model_artifact.review.v1",
        })
        assert sealed["ok"] and sealed["verdict"] == "ALLOW", sealed

        # 3. Independent verifier agent verifies receipt + CanonFS fsck
        verified = call_tool("t81.verify_receipt", {
            "t81_bin": t81_bin,
            "bundle": sealed["bundle"],
            "public_key": str(pub),
            "trusted_policy": str(policy),
            "canonfs_root": str(croot),
        })
        assert verified["ok"] and verified["signature_verified"] and verified["policy_verified"] and verified["canonfs_verified"], verified
        print(json.dumps({
            "dogfood_model_diff_same": diff_same["identical"],
            "dogfood_model_diff_other": diff_other["identical"],
            "receipt_verified": verified,
        }, indent=2))
finally:
    proc.stdin.close()
    proc.wait(timeout=5)
PY

echo "=== [2/3] Running Compromised-Agent Gauntlet & Warm Latency Benchmark ==="
python3 "${GAUNTLET}"

echo "=== [3/3] Running Native C++ t81 mcp serve Test & Sub-Millisecond Benchmark ==="
python3 "${ROOT_DIR}/examples/research/agent-receipt-sidecar/test_native_mcp.py"
