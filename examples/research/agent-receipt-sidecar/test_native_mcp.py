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
            assert init["result"]["serverInfo"]["version"] == "2.0.1"
            # JSON-RPC 2.0 notification must produce NO response; next ping must match its own id.
            proc.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n")
            proc.stdin.flush()
            pong = rpc("ping")
            assert pong["id"] == rid and pong["result"] == {}
            tools = rpc("tools/list")["result"]["tools"]
            assert len(tools) == 15
            assert all("inputSchema" in t for t in tools)

            # 0. session_bootstrap (1-call verified wake-up packet + standard MCP content envelope)
            boot_resp = rpc("tools/call", {"name": "t81.session_bootstrap", "arguments": {"repo_root": str(ROOT_DIR)}})["result"]
            assert boot_resp["isError"] is False
            assert boot_resp["content"][0]["type"] == "text"
            assert json.loads(boot_resp["content"][0]["text"]) == boot_resp["structured"]
            boot = boot_resp["structured"]
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

            # 8. session_bootstrap capabilities + t81.run_subgraph (fail-closed + verified VM execution)
            boot = rpc("tools/call", {"name": "t81.session_bootstrap", "arguments": {"repo_root": str(Path(__file__).resolve().parents[3])}})["result"]["structured"]
            assert boot["ok"] and boot["native_mcp_tools"] == 15
            assert boot["capabilities"]["bit_exact_cross_device_sha3_512"] is True
            assert boot["capabilities"]["bytes_per_trit"] == 0.25

            run_bad = rpc("tools/call", {"name": "t81.run_subgraph", "arguments": {"bytecode_hex": bad_bc}})["result"]["structured"]
            assert not run_bad["ok"] and not run_bad["executed"] and run_bad["verdict"] == "REJECT_UNVERIFIED_POLICY"

            run_ok = rpc("tools/call", {"name": "t81.run_subgraph", "arguments": {"bytecode_hex": valid_bc, "policy_path": str(pol)}})["result"]["structured"]
            assert run_ok["ok"] and run_ok["executed"] and run_ok["verdict"] == "VERIFIED_AND_EXECUTED"
            assert run_ok["execution_receipt"].startswith("sha3-512:")

            # 9. t81.diff_check (admit <= 1% flip, deny > 1% flip)
            # 16 words = 1024 weights: flipping 4 bits = 0.39% (ADMIT), flipping 32 bits = 3.125% (DENY)
            b_pos = tmp / "base.pos"
            b_neg = tmp / "base.neg"
            c_ok_pos = tmp / "cand_ok.pos"
            c_bad_pos = tmp / "cand_bad.pos"
            b_pos.write_bytes(b"\x00" * 128)
            b_neg.write_bytes(b"\x00" * 128)
            c_ok_pos.write_bytes(struct.pack("<Q", 0xF) + b"\x00" * 120)
            c_bad_pos.write_bytes(struct.pack("<Q", 0xFFFFFFFF) + b"\x00" * 120)

            dc_ok = rpc("tools/call", {"name": "t81.diff_check", "arguments": {
                "base_pos_path": str(b_pos), "base_neg_path": str(b_neg),
                "candidate_pos_path": str(c_ok_pos), "candidate_neg_path": str(b_neg),
                "max_flip_pct": "1.0"}})["result"]["structured"]
            assert dc_ok["ok"] and dc_ok["allowed"] and dc_ok["flipped_weights"] == 4

            dc_bad = rpc("tools/call", {"name": "t81.diff_check", "arguments": {
                "base_pos_path": str(b_pos), "base_neg_path": str(b_neg),
                "candidate_pos_path": str(c_bad_pos), "candidate_neg_path": str(b_neg),
                "max_flip_pct": "1.0"}})["result"]["structured"]
            assert not dc_bad["ok"] and not dc_bad["allowed"] and dc_bad["verdict"] == "DENY_EXCEEDS_FLIP_BUDGET"

            # 10. t81.memory_search (zero-float Dual-Rail top-K retrieval + SHA3-512 receipt)
            # 3 docs x 64 dims: doc 0 = all 0, doc 1 = all +1 (exact match), doc 2 = all -1
            idx_pos = tmp / "idx.pos"
            idx_neg = tmp / "idx.neg"
            q_pos = tmp / "q.pos"
            q_neg = tmp / "q.neg"
            idx_pos.write_bytes(struct.pack("<QQQ", 0, 0xFFFFFFFFFFFFFFFF, 0))
            idx_neg.write_bytes(struct.pack("<QQQ", 0, 0, 0xFFFFFFFFFFFFFFFF))
            q_pos.write_bytes(struct.pack("<Q", 0xFFFFFFFFFFFFFFFF))
            q_neg.write_bytes(struct.pack("<Q", 0))
            ms = rpc("tools/call", {"name": "t81.memory_search", "arguments": {
                "index_pos_path": str(idx_pos), "index_neg_path": str(idx_neg),
                "query_pos_path": str(q_pos), "query_neg_path": str(q_neg),
                "dim": 64, "top_k": 2}})["result"]["structured"]
            assert ms["ok"] and ms["docs_searched"] == 3
            assert ms["results"][0]["doc_id"] == 1 and ms["results"][0]["score"] == 64
            assert ms["results"][1]["doc_id"] == 0 and ms["results"][1]["score"] == 0
            assert ms["retrieval_receipt"].startswith("sha3-512:")

            # 11. t81.adapter_apply (zero-dequantization saturated ternary LoRA composition)
            # Base word: bit0=+1, bit1=-1, bit2=0, bit3=+1 -> pos=0b1001, neg=0b0010
            # Delta word: bit0=+1 (sat->+1, no flip), bit1=+1 (cancel->0, flip),
            #             bit2=-1 (0->-1, flip), bit3=0 (keep +1, no flip)
            # Expected merged: pos=0b1001, neg=0b0100, effective_flips=2
            ab_pos = tmp / "ad_base.pos"
            ab_neg = tmp / "ad_base.neg"
            ad_pos = tmp / "ad_delta.pos"
            ad_neg = tmp / "ad_delta.neg"
            ao_pos = tmp / "ad_out.pos"
            ao_neg = tmp / "ad_out.neg"
            ab_pos.write_bytes(struct.pack("<Q", 0b1001))
            ab_neg.write_bytes(struct.pack("<Q", 0b0010))
            ad_pos.write_bytes(struct.pack("<Q", 0b0011))
            ad_neg.write_bytes(struct.pack("<Q", 0b0100))

            # Fail-closed when max_flip_pct = 1.0% (2/64 = 3.125% > 1.0%)
            aa_deny = rpc("tools/call", {"name": "t81.adapter_apply", "arguments": {
                "base_pos_path": str(ab_pos), "base_neg_path": str(ab_neg),
                "delta_pos_path": str(ad_pos), "delta_neg_path": str(ad_neg),
                "out_pos_path": str(ao_pos), "out_neg_path": str(ao_neg),
                "max_flip_pct": "1.0"}})["result"]["structured"]
            assert not aa_deny["ok"] and not aa_deny["applied"]
            assert aa_deny["verdict"] == "DENY_EXCEEDS_FLIP_BUDGET"
            assert aa_deny["effective_flips"] == 2 and not ao_pos.exists()

            # Admitted when max_flip_pct = 5.0%
            aa_ok = rpc("tools/call", {"name": "t81.adapter_apply", "arguments": {
                "base_pos_path": str(ab_pos), "base_neg_path": str(ab_neg),
                "delta_pos_path": str(ad_pos), "delta_neg_path": str(ad_neg),
                "out_pos_path": str(ao_pos), "out_neg_path": str(ao_neg),
                "max_flip_pct": "5.0"}})["result"]["structured"]
            assert aa_ok["ok"] and aa_ok["applied"] and aa_ok["effective_flips"] == 2
            assert aa_ok["adapter_receipt"].startswith("sha3-512:")
            assert struct.unpack("<Q", ao_pos.read_bytes())[0] == 0b1001
            assert struct.unpack("<Q", ao_neg.read_bytes())[0] == 0b0100

            # 11b. t81.adapter_apply via compact SBDR1 wire format -> bit-exact merged_sha3_512!
            sbdr_file = tmp / "ad_delta.sbdr"
            sbdr_out_pos = tmp / "ad_sbdr_out.pos"
            sbdr_out_neg = tmp / "ad_sbdr_out.neg"
            # 1 word total, mask=0b1 (word 0 active), payload=(pos=0b0011, neg=0b0100)
            sbdr_file.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 1, 1, 0b0011, 0b0100))
            aa_sbdr = rpc("tools/call", {"name": "t81.adapter_apply", "arguments": {
                "base_pos_path": str(ab_pos), "base_neg_path": str(ab_neg),
                "delta_sbdr_path": str(sbdr_file),
                "out_pos_path": str(sbdr_out_pos), "out_neg_path": str(sbdr_out_neg),
                "max_flip_pct": 5.0}})["result"]["structured"]
            assert aa_sbdr["ok"] and aa_sbdr["applied"] and aa_sbdr["delta_format"] == "sbdr1"
            assert aa_sbdr["effective_flips"] == 2 and aa_sbdr["active_blocks"] == 1
            assert aa_sbdr["merged_sha3_512"] == aa_ok["merged_sha3_512"]
            assert sbdr_out_pos.read_bytes() == ao_pos.read_bytes()
            assert sbdr_out_neg.read_bytes() == ao_neg.read_bytes()

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
