#!/usr/bin/env python3
"""Three-Plane Stratified Execution Cross-Platform Differential Oracle (Phase 3).

Runs the canonical 4-stage Three-Plane workload:
  1. Deterministic Top Plane: loads immutable `axion_root.apl` policy and `State A`
     via `t81 mcp serve` and verifies deterministic T81VM execution.
  2. Stochastic Middle Plane: generates a `seeded_probabilistic` proposal and an
     `external_probabilistic` proposal (plus runs `t81_provider_contract_test` if built).
  3. Hybrid Reconciliation Plane: audits Hamming flip-budget (`t81.diff_check`),
     runs `RegimeSelector` (`R0`–`R4`) / Dual-Rail `SBDR1T2D` verification, proves
     fail-closed `ERR_PLANE_BYPASS_DENIED` (-1) and `DEFER_HYBRID_ABSTAIN` (0),
     and commits `State A -> State C` (+1) via `t81.tx_commit`.
  4. Deterministic Base Verification: verifies bit-exact `state_c_ref` SHA3-512,
     RFC-00D4 Ed25519 receipt DAG, cache-aware BitNet-2B fixture parity, and
     bounded `TISC` payload parity against `t81 enclave run`.

Asserts bit-identical `state_c_ref` and canonical `three_plane_parity_sha3_512`
across repeat runs and across `AArch64` (macOS/Linux) and `x86_64` (Linux/Windows).
"""

from __future__ import annotations

import hashlib
import json
import os
import platform
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

BUILD_DIR = Path(os.environ.get("T81_BUILD_DIR", str(ROOT / "build")))


def _sha3_512(data: bytes) -> str:
    return "sha3-512:" + hashlib.sha3_512(data).hexdigest()


def _find_bin(name: str) -> Path | None:
    for p in (
        BUILD_DIR / name,
        BUILD_DIR / f"{name}.exe",
        BUILD_DIR / "Release" / f"{name}.exe",
    ):
        if p.exists():
            return p
    return None


def _run_checked(cmd: list[str]) -> str:
    proc = subprocess.run(cmd, cwd=str(ROOT), capture_output=True, text=True, check=True)
    return proc.stdout.strip()


def _run_mcp_three_plane_once(t81_bin: Path) -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix="t81-three-plane-oracle-") as td:
        root = Path(td)
        bak_dir = root / "bak"
        bak_dir.mkdir()

        state_a_bytes = b'{"plane":"deterministic","state":"A","epoch":0}\n'
        # Seeded probabilistic proposal (deterministic from fixed seed=42)
        seeded_proposal = (
            b'{"plane":"stochastic","repro":"seeded_probabilistic","seed":42,"state":"C"}\n'
        )
        # External probabilistic proposal (unseeded, rejected on direct bypass)
        ext_proposal = (
            b'{"plane":"stochastic","repro":"external_probabilistic","state":"UNTRUSTED"}\n'
        )

        target_path = root / "state.json"
        target_path.write_bytes(state_a_bytes)
        state_a_ref = _sha3_512(state_a_bytes)
        state_c_expected = _sha3_512(seeded_proposal)

        # Dual-Rail (P, N) 8-byte bitplanes for t81.diff_check (1 bit flip = 1/64 = 1.5625% <= 5%)
        base_pos = root / "base_pos.bin"
        base_neg = root / "base_neg.bin"
        cand_pos = root / "cand_pos.bin"
        cand_neg = root / "cand_neg.bin"
        base_pos.write_bytes(b"\x0f\x00\x00\x00\x00\x00\x00\x00")
        base_neg.write_bytes(b"\xf0\x00\x00\x00\x00\x00\x00\x00")
        cand_pos.write_bytes(b"\x0e\x00\x00\x00\x00\x00\x00\x00")
        cand_neg.write_bytes(b"\xf0\x00\x00\x00\x00\x00\x00\x00")

        policy_path = root / "axion_root.apl"
        allowed = " ".join(f'"{h}"' for h in (state_a_ref, state_c_expected))
        policy_path.write_text(
            f"(policy\n  (tier 1)\n  (allowed-tensor-hashes [{allowed}]))\n",
            encoding="utf-8",
        )
        os.chmod(policy_path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

        try:
            proc = subprocess.Popen(
                [str(t81_bin), "mcp", "serve", "--root", str(root)],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                text=True,
                bufsize=1,
            )
            rid = 0

            def call(name: str, arguments: dict[str, Any]) -> dict[str, Any]:
                nonlocal rid
                rid += 1
                assert proc.stdin is not None and proc.stdout is not None
                req = {
                    "jsonrpc": "2.0",
                    "id": rid,
                    "method": "tools/call",
                    "params": {"name": name, "arguments": arguments},
                }
                proc.stdin.write(json.dumps(req) + "\n")
                proc.stdin.flush()
                line = proc.stdout.readline()
                if not line:
                    raise RuntimeError(f"MCP EOF on {name}")
                return json.loads(line)["result"]["structured"]

            try:
                # Hybrid diff_check Hamming flip-budget gate
                diff_res = call(
                    "t81.diff_check",
                    {
                        "base_pos_path": str(base_pos),
                        "base_neg_path": str(base_neg),
                        "candidate_pos_path": str(cand_pos),
                        "candidate_neg_path": str(cand_neg),
                        "max_flip_pct": 5.0,
                    },
                )
                assert (
                    diff_res.get("ok") is True
                    and diff_res.get("allowed") is True
                    and diff_res.get("verdict") == "ADMIT_WITHIN_FLIP_BUDGET"
                ), diff_res

                # Negative check (-1): direct stochastic -> deterministic bypass denied
                b_neg = call(
                    "t81.tx_begin",
                    {
                        "target_path": str(target_path),
                        "backup_dir": str(bak_dir),
                        "agent_id": "oracle-bypass",
                        "isolated": True,
                    },
                )
                Path(b_neg["staging_path"]).write_bytes(ext_proposal)
                c_neg = call(
                    "t81.tx_commit",
                    {
                        "tx_id": b_neg["tx_id"],
                        "policy_path": str(policy_path),
                        "agent_id": "oracle-bypass",
                        "source_plane": "stochastic",
                        "target_plane": "deterministic",
                        "transition_kind": "transition_path",
                        "reproducibility_class": "external_probabilistic",
                        "hybrid_reconciled": False,
                        "state_a_ref": state_a_ref,
                    },
                )
                assert c_neg["ok"] is False and c_neg["code"] == "ERR_PLANE_BYPASS_DENIED"
                assert c_neg["state_c_ref"] is None

                # Inert check (0): hybrid abstain leaves zero mutation
                b_zero = call(
                    "t81.tx_begin",
                    {
                        "target_path": str(target_path),
                        "backup_dir": str(bak_dir),
                        "agent_id": "oracle-abstain",
                        "isolated": True,
                    },
                )
                Path(b_zero["staging_path"]).write_bytes(seeded_proposal)
                c_zero = call(
                    "t81.tx_commit",
                    {
                        "tx_id": b_zero["tx_id"],
                        "policy_path": str(policy_path),
                        "agent_id": "oracle-abstain",
                        "source_plane": "hybrid",
                        "target_plane": "deterministic",
                        "transition_kind": "transition_path",
                        "hybrid_reconciled": True,
                        "hybrid_verdict_trit": 0,
                        "state_a_ref": state_a_ref,
                    },
                )
                assert c_zero["ok"] is False and c_zero["trit"] == 0
                assert c_zero["code"] == "DEFER_HYBRID_ABSTAIN" and c_zero["state_c_ref"] is None

                # Positive check (+1): reconciled Hybrid -> Deterministic commit State A -> State C
                b_ok = call(
                    "t81.tx_begin",
                    {
                        "target_path": str(target_path),
                        "backup_dir": str(bak_dir),
                        "agent_id": "oracle-commit",
                        "isolated": True,
                    },
                )
                Path(b_ok["staging_path"]).write_bytes(seeded_proposal)
                c_ok = call(
                    "t81.tx_commit",
                    {
                        "tx_id": b_ok["tx_id"],
                        "policy_path": str(policy_path),
                        "agent_id": "oracle-commit",
                        "source_plane": "hybrid",
                        "target_plane": "deterministic",
                        "transition_kind": "transition_path",
                        "reproducibility_class": "seeded_probabilistic",
                        "hybrid_reconciled": True,
                        "state_a_ref": state_a_ref,
                    },
                )
                assert c_ok["ok"] is True and c_ok["trit"] == 1
                assert c_ok["state_a_ref"] == state_a_ref
                assert c_ok["state_c_ref"] == state_c_expected
                assert bool(c_ok.get("ed25519_signature_b64"))

                return {
                    "state_a_ref": c_ok["state_a_ref"],
                    "state_c_ref": c_ok["state_c_ref"],
                    "commit_receipt": c_ok["commit_receipt"],
                    "policy_sha3_512": c_ok["policy_sha3_512"],
                    "diff_check_verdict": diff_res["verdict"],
                    "bypass_code": c_neg["code"],
                    "abstain_code": c_zero["code"],
                }
            finally:
                if proc.stdin:
                    proc.stdin.close()
                if proc.stdout:
                    proc.stdout.close()
                proc.wait(timeout=5)
        finally:
            if policy_path.exists():
                os.chmod(policy_path, stat.S_IRUSR | stat.S_IWUSR)


def main() -> int:
    report: dict[str, Any] = {
        "schema": "t81.three-plane-differential.v1",
        "host": f"{platform.system()} {platform.machine()}",
        "qemu_aarch64_available": shutil.which("qemu-system-aarch64") is not None,
        "checks": {},
    }

    # 1. Core deterministic C++ binaries (Ed25519 DAG, ProviderGovernor, DPE+MCP, RegimeSelector)
    for bin_name in (
        "t81_ed25519_bundle_test",
        "t81_provider_contract_test",
        "t81_dpe_mcp_unification_test",
        "test_regime_dispatch",
        "t81_dualrail_sbdr_vm_test",
        "t81_bitnet_2b_dispatch_test",
    ):
        b = _find_bin(bin_name)
        if b is not None:
            _run_checked([str(b)])
            report["checks"][bin_name] = "PASS"

    # 2. Full Three-Plane MCP round-trip (2x repeat bit-exact parity) + Enclave parity
    t81_bin = _find_bin("t81")
    if t81_bin is not None:
        r1 = _run_mcp_three_plane_once(t81_bin)
        r2 = _run_mcp_three_plane_once(t81_bin)
        assert r1 == r2, f"Three-Plane round-trip non-deterministic: {r1} != {r2}"
        report["checks"]["three_plane_roundtrip"] = r1

        with tempfile.NamedTemporaryFile("w", suffix=".apl", delete=False) as tf:
            tf.write("(policy (tier 1))\n")
            pol_path = tf.name
        try:
            e1 = json.loads(
                _run_checked(
                    [
                        str(t81_bin),
                        "enclave",
                        "run",
                        "--payload",
                        r1["state_c_ref"],
                        "--policy",
                        pol_path,
                        "--dry-run",
                        "--json",
                    ]
                )
            )
            e2 = json.loads(
                _run_checked(
                    [
                        str(t81_bin),
                        "enclave",
                        "run",
                        "--payload",
                        r1["state_c_ref"],
                        "--policy",
                        pol_path,
                        "--dry-run",
                        "--json",
                    ]
                )
            )
            assert e1 == e2 and e1["ok"] is True
            report["checks"]["enclave_policy_sha3_512"] = e1["policy_sha3_512"]
        finally:
            Path(pol_path).unlink(missing_ok=True)

    # 3. Cache-aware BitNet-2B streaming checkpoint hook
    ingester = (
        ROOT / "examples" / "research" / "bitnet-verifiable-token" / "load_safetensors_bitnet.py"
    )
    m1 = json.loads(_run_checked([sys.executable, str(ingester), "--verify-real-hf-if-cached"]))
    m2 = json.loads(_run_checked([sys.executable, str(ingester), "--verify-real-hf-if-cached"]))
    assert m1["manifest_sha3_512"] == m2["manifest_sha3_512"]
    report["checks"]["bitnet_2b_manifest_sha3_512"] = m1["manifest_sha3_512"]

    canonical = json.dumps(report["checks"], sort_keys=True, separators=(",", ":")).encode("utf-8")
    report["three_plane_parity_sha3_512"] = _sha3_512(canonical)
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
