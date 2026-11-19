#!/usr/bin/env python3
"""End-to-end test & latency benchmark for native C++ `t81 mcp serve`."""

from __future__ import annotations

import hashlib
import json
import os
import statistics
import subprocess
import tempfile
import time
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[3]
T81_BIN = os.environ.get("T81_BIN", str(ROOT_DIR / "build" / "t81"))


def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        croot = tmp / "canonfs"
        croot.mkdir()
        good = tmp / "approved.bin"
        good.write_bytes(b"approved-native-mcp-weights")
        bad = tmp / "poisoned.bin"
        bad.write_bytes(b"POISONED_WEIGHTS")

        good_sha = "sha3-512:" + hashlib.sha3_512(good.read_bytes()).hexdigest()
        pol = tmp / "policy.apl"
        pol.write_text(
            f'(policy\n'
            f'  (tier 1)\n'
            f'  (allowed-tensor-hashes ["{good_sha}"])\n'
            f'  (require-axion-event (reason "enforcement-mode:warn"))\n'
            f'  (require-axion-event (reason "action-policy:deploy_config=fail-closed"))\n'
            f'  (require-axion-event (reason "action-policy:read_metrics=observe-only")))\n',
            encoding="utf-8",
        )
        # Seed CanonFS with one object
        subprocess.run([T81_BIN, "canonfs", "import", str(good), "--canonfs-root", str(croot), "--json"], check=True, capture_output=True)

        proc = subprocess.Popen(
            [T81_BIN, "mcp", "serve"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        rid = 0

        def rpc(method: str, params: dict | None = None) -> dict:
            nonlocal rid
            rid += 1
            proc.stdin.write(json.dumps({"jsonrpc": "2.0", "id": rid, "method": method, "params": params or {}}) + "\n")
            proc.stdin.flush()
            return json.loads(proc.stdout.readline())

        try:
            init = rpc("initialize")
            assert init["result"]["serverInfo"]["name"] == "t81-native-mcp"
            tools = rpc("tools/list")["result"]["tools"]
            assert len(tools) == 11
            assert all("inputSchema" in t for t in tools)

            # 0. session_bootstrap (1-call verified wake-up packet)
            boot = rpc("tools/call", {"name": "t81.session_bootstrap", "arguments": {"repo_root": str(ROOT_DIR)}})["result"]["structured"]
            assert boot["ok"] and boot["public_api_lock_present"] and boot["determinism_registry_present"]
            assert len(boot["git_head_sha"]) == 40

            # 0b. Actionable error hint on missing required arg
            err_hint = rpc("tools/call", {"name": "t81.policy_validate", "arguments": {}})["result"]["structured"]
            assert not err_hint["ok"] and "expected" in err_hint

            # 0c. Atomic transaction: begin -> mutate -> denied commit -> abort restores exact bytes
            tx_file = tmp / "tx_target.txt"
            tx_file.write_bytes(b"original-safe-state")
            tx_bak_dir = tmp / "tx_bak"
            tx_b = rpc("tools/call", {"name": "t81.tx_begin", "arguments": {"target_path": str(tx_file), "backup_dir": str(tx_bak_dir)}})["result"]["structured"]
            assert tx_b["ok"]
            tx_file.write_bytes(b"corrupted-mutation")
            tx_bad_commit = rpc("tools/call", {"name": "t81.tx_commit", "arguments": {"tx_id": tx_b["tx_id"], "policy_path": str(pol)}})["result"]["structured"]
            assert not tx_bad_commit["ok"] and not tx_bad_commit["committed"]
            tx_ab = rpc("tools/call", {"name": "t81.tx_abort", "arguments": {"tx_id": tx_b["tx_id"]}})["result"]["structured"]
            assert tx_ab["ok"] and tx_ab["aborted"]
            assert tx_file.read_bytes() == b"original-safe-state"

            # 1. get_effective_policy
            eff_deploy = rpc("tools/call", {"name": "t81.get_effective_policy", "arguments": {"policy_path": str(pol), "action_type": "deploy_config"}})["result"]["structured"]
            eff_metrics = rpc("tools/call", {"name": "t81.get_effective_policy", "arguments": {"policy_path": str(pol), "action_type": "read_metrics"}})["result"]["structured"]
            assert eff_deploy["ok"] and eff_deploy["policy_floor"] == "fail-closed"
            assert eff_metrics["ok"] and eff_metrics["policy_floor"] == "observe-only"

            # 2. gate_check: legit ALLOW
            g_ok = rpc("tools/call", {"name": "t81.gate_check", "arguments": {"artifact_path": str(good), "policy_path": str(pol), "action_type": "deploy_config"}})["result"]["structured"]
            assert g_ok["ok"] and g_ok["decision"] == "ALLOW" and not g_ok["blocked"]

            # 3. gate_check: anti-downgrade clamping on poisoned artifact
            g_deny = rpc("tools/call", {"name": "t81.gate_check", "arguments": {"artifact_path": str(bad), "policy_path": str(pol), "action_type": "deploy_config", "mode": "observe-only"}})["result"]["structured"]
            assert not g_deny["ok"] and g_deny["blocked"] and g_deny["downgrade_attempt_blocked"] and g_deny["mode_applied"] == "fail-closed"

            # 4. gate_check: observe-only truthful shadow verdict
            g_obs = rpc("tools/call", {"name": "t81.gate_check", "arguments": {"artifact_path": str(bad), "policy_path": str(pol), "action_type": "read_metrics"}})["result"]["structured"]
            assert g_obs["ok"] and not g_obs["blocked"] and g_obs["decision"] == "DENY"

            # 5. canonfs_fsck clean + tampered
            fsck_ok = rpc("tools/call", {"name": "t81.canonfs_fsck", "arguments": {"canonfs_root": str(croot)}})["result"]["structured"]
            assert fsck_ok["ok"] and fsck_ok["checked_objects"] >= 1

            # 6. 100-call warm gate_check benchmark
            lats = []
            for _ in range(100):
                t0 = time.perf_counter()
                r = rpc("tools/call", {"name": "t81.gate_check", "arguments": {"artifact_path": str(good), "policy_path": str(pol), "action_type": "deploy_config"}})["result"]["structured"]
                lats.append((time.perf_counter() - t0) * 1000.0)
                assert r["ok"]

            blk = next((croot / "objects").rglob("*.blk"))
            blk.write_bytes(blk.read_bytes() + b"X")
            fsck_bad = rpc("tools/call", {"name": "t81.canonfs_fsck", "arguments": {"canonfs_root": str(croot)}})["result"]["structured"]
            assert not fsck_bad["ok"]

            # 7. Narrowed TISC subgraph static verification (valid + fail-closed adversarial)
            import struct
            def enc_insn(op, a, b, c):
                return struct.pack("<Biii", op, a, b, c)
            # Opcodes: Halt=1, AxVerify=24, TMatMul=34, ChkShape=45, WeightsLoad=72
            valid_bc = (
                enc_insn(72, 1, 0, 0) +
                enc_insn(45, 1, 4096, 4096) +
                enc_insn(24, 1, 1, 0) +
                enc_insn(34, 10, 0, 1) +
                enc_insn(1, 0, 0, 0)
            ).hex()
            vr_ok = rpc("tools/call", {"name": "t81.verify_subgraph", "arguments": {"bytecode_hex": valid_bc}})["result"]["structured"]
            assert vr_ok["ok"] and vr_ok["verdict"] == "VERIFIED_OK" and vr_ok["matmul_count"] == 1
            assert vr_ok["program_sha3_512"].startswith("sha3-512:")

            # Adversarial subgraph missing AxVerify -> fail-closed REJECT_UNVERIFIED_POLICY
            bad_bc = (
                enc_insn(72, 1, 0, 0) +
                enc_insn(45, 1, 4096, 4096) +
                enc_insn(34, 10, 0, 1) +
                enc_insn(1, 0, 0, 0)
            ).hex()
            vr_bad = rpc("tools/call", {"name": "t81.verify_subgraph", "arguments": {"bytecode_hex": bad_bc}})["result"]["structured"]
            assert not vr_bad["ok"] and vr_bad["verdict"] == "REJECT_UNVERIFIED_POLICY"

            print(json.dumps({
                "native_cpp_mcp_tools": len(tools),
                "all_checks_passed": True,
                "gate_check_100_calls": {
                    "mean_ms": round(statistics.mean(lats), 3),
                    "p50_ms": round(statistics.median(lats), 3),
                    "p95_ms": round(sorted(lats)[94], 3),
                    "max_ms": round(max(lats), 3),
                },
            }, indent=2))
            return 0
        finally:
            proc.stdin.close()
            proc.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
