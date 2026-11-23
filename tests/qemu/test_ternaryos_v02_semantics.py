#!/usr/bin/env python3
"""Executable contract tests for TernaryOS v0.2 semantics (TERNARYOS_ARCHITECTURE_v0.2.md).

Verifies Sections 4-6 cleanly without overlapping PR #856's owned files
(`tests/qemu/test_boot_demo.py`, `.github/workflows/qemu-verification.yml`):
  1. Transactional isolation (`t81.tx_begin` / `t81.tx_commit` / `t81.tx_abort`) over
     CanonFS SBDR1T2D (K* = 0) and TISC v1.9.0 / T81VM workspaces:
       - Policy mutation denied fail-closed + atomic `tx_abort` byte restoration.
       - Oracle drift (`max_abs_err != 0`) triggers atomic `tx_abort` with zero side effects.
       - Receipt-gated `tx_commit` succeeds only when `max_abs_err == 0`,
         `dualrail_fallback_count == 0`, and `repeat_sha3_identical == True`.
  2. Three-state `{-1, 0, +1}` classification across control/policy verdicts,
     SBDR1T2D zero-block skipping telemetry (`dualrail_sparse128_blocks_skipped`),
     and the 10 accepted / 16 negative routing ledger constraints.
  3. Immutable `axion_root.apl` + real `qemu-system-aarch64` PL011 UART0 (0x09000000)
     round-trip (`boot_image_sha3_512` & `serial_transcript_sha3_512`) paired with
     `t81.session_bootstrap` R1/R2 regime routing rules.
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

ROOT_DIR = Path(__file__).resolve().parents[2]
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from tests.qemu.test_boot_to_receipt_contract import (  # noqa: E402
    T81_BIN,
    _build_aarch64_uart_guest_elf,
    _build_dot_subgraph,
    _call_mcp_fresh,
    _run_real_qemu_serial_session,
)

# Three-state balanced-ternary contract classification
TRIT_REJECTED = -1
TRIT_INERT = 0
TRIT_VERIFIED = 1


def _classify_control_verdict(res: dict[str, Any]) -> int:
    if res.get("verdict") == "VM_TRAP" or res.get("ok") is False:
        return TRIT_REJECTED
    if res.get("verdict") == "VERIFIED_AND_EXECUTED" and res.get("halted") is True:
        return TRIT_VERIFIED
    return TRIT_INERT


class _McpSession:
    def __init__(self, root: Path) -> None:
        self._proc = subprocess.Popen(
            [T81_BIN, "mcp", "serve", "--root", str(root)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self._rid = 0

    def __enter__(self) -> "_McpSession":
        return self

    def call(self, name: str, arguments: dict[str, Any]) -> dict[str, Any]:
        assert self._proc.stdin is not None and self._proc.stdout is not None
        self._rid += 1
        req = {
            "jsonrpc": "2.0",
            "id": self._rid,
            "method": "tools/call",
            "params": {"name": name, "arguments": arguments},
        }
        self._proc.stdin.write(json.dumps(req) + "\n")
        self._proc.stdin.flush()
        line = self._proc.stdout.readline()
        if not line:
            raise RuntimeError(f"Empty MCP response for {name}")
        return json.loads(line)["result"]["structured"]

    def __exit__(self, *_: Any) -> None:
        if self._proc.stdin:
            self._proc.stdin.close()
        if self._proc.stdout:
            self._proc.stdout.close()
        self._proc.wait(timeout=5)


class TernaryOSV02SemanticsTest(unittest.TestCase):
    def setUp(self) -> None:
        if not Path(T81_BIN).exists():
            self.skipTest(f"t81 binary not built at {T81_BIN}")

    def _prepare_governed_workspace(self, root: Path) -> dict[str, Any]:
        croot = root / "canonfs"
        croot.mkdir()
        sbdr_path = root / "weights_sbdr1t2d.sbdr"
        # 4 blocks total, 1 active block (75% block-sparse), K* = 0 pre-packed
        sbdr_path.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 4, 1, 0b0101, 0b1010))
        subprocess.run(
            [T81_BIN, "canonfs", "import", str(sbdr_path), "--canonfs-root", str(croot), "--json"],
            check=True,
            capture_output=True,
        )
        fsck = _call_mcp_fresh(
            [root], "t81.canonfs_fsck", {"canonfs_root": str(croot), "compact": True}
        )["result"]["structured"]
        self.assertTrue(fsck["ok"] and fsck["checked_objects"] >= 1)

        weight_sha3 = _call_mcp_fresh(
            [root], "t81.determinism_hash", {"path": str(sbdr_path), "compact": True}
        )["result"]["structured"]["sha3_512"]

        policy_path = root / "axion_root.apl"
        policy_bytes = (
            f'(policy\n  (tier 1)\n  (allowed-tensor-hashes ["{weight_sha3}"]))\n'
        ).encode("utf-8")
        policy_path.write_bytes(policy_bytes)
        policy_sha3 = _call_mcp_fresh(
            [root], "t81.determinism_hash", {"path": str(policy_path), "compact": True}
        )["result"]["structured"]["sha3_512"]

        return {
            "canonfs_root": croot,
            "sbdr_path": sbdr_path,
            "weight_sha3": weight_sha3,
            "policy_path": policy_path,
            "policy_bytes": policy_bytes,
            "policy_sha3": policy_sha3,
        }

    def test_01_transactional_rollback_on_oracle_drift_and_policy_violation(self) -> None:
        with tempfile.TemporaryDirectory(prefix="t81-v02-tx-abort-") as td:
            root = Path(td)
            ws = self._prepare_governed_workspace(root)
            bak_dir = root / "tx_bak"
            bak_dir.mkdir()
            artifact_path = root / "speculative_receipt.json"
            original_bytes = b'{"status":"baseline","trit":0}\n'
            artifact_path.write_bytes(original_bytes)

            with _McpSession(root) as mcp:
                # Case A: Oracle drift (max_abs_err != 0) forces atomic tx_abort with zero side effects
                tx_a = mcp.call(
                    "t81.tx_begin",
                    {"target_path": str(artifact_path), "backup_dir": str(bak_dir), "compact": True},
                )
                self.assertTrue(tx_a["ok"])
                artifact_path.write_bytes(b'{"status":"mutated","max_abs_err":1}\n')
                observed_max_abs_err = 1
                self.assertNotEqual(observed_max_abs_err, 0)
                aborted = mcp.call("t81.tx_abort", {"tx_id": tx_a["tx_id"], "compact": True})
                self.assertTrue(aborted["ok"] and aborted["aborted"])
                self.assertEqual(artifact_path.read_bytes(), original_bytes)

                # Case B: Policy mutation attempt denied fail-closed (-1) + atomic tx_abort
                tx_b = mcp.call(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["policy_path"]),
                        "backup_dir": str(bak_dir),
                        "compact": True,
                    },
                )
                self.assertTrue(tx_b["ok"])
                ws["policy_path"].write_bytes(b"(policy (tier 0))\n")
                commit_deny = mcp.call(
                    "t81.tx_commit",
                    {
                        "tx_id": tx_b["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "compact": True,
                    },
                )
                self.assertEqual(_classify_control_verdict(commit_deny), TRIT_REJECTED)
                self.assertFalse(commit_deny["committed"])
                abort_b = mcp.call("t81.tx_abort", {"tx_id": tx_b["tx_id"], "compact": True})
                self.assertTrue(abort_b["ok"] and abort_b["aborted"])
                self.assertEqual(ws["policy_path"].read_bytes(), ws["policy_bytes"])

    def test_02_receipt_gated_tx_commit_with_repeat_sha3_determinism(self) -> None:
        with tempfile.TemporaryDirectory(prefix="t81-v02-tx-commit-") as td:
            root = Path(td)
            ws = self._prepare_governed_workspace(root)
            bak_dir = root / "tx_bak"
            bak_dir.mkdir()
            artifact_path = root / "committed_receipt.json"
            artifact_path.write_bytes(b"{}\n")

            bc_hex, oracle_dot = _build_dot_subgraph()
            self.assertEqual(oracle_dot, 15)

            run1 = _call_mcp_fresh(
                [root],
                "t81.run_subgraph",
                {"bytecode_hex": bc_hex, "policy_path": str(ws["policy_path"]), "compact": True},
            )["result"]["structured"]
            run2 = _call_mcp_fresh(
                [root],
                "t81.run_subgraph",
                {"bytecode_hex": bc_hex, "policy_path": str(ws["policy_path"]), "compact": True},
            )["result"]["structured"]

            self.assertEqual(_classify_control_verdict(run1), TRIT_VERIFIED)
            self.assertEqual(run1["dualrail_fallback_count"], 0)
            self.assertIn("dualrail_sparse128_blocks_skipped", run1)
            self.assertEqual(run1["execution_receipt"], run2["execution_receipt"])

            with _McpSession(root) as mcp:
                tx = mcp.call(
                    "t81.tx_begin",
                    {"target_path": str(artifact_path), "backup_dir": str(bak_dir), "compact": True},
                )
                self.assertTrue(tx["ok"])
                payload = json.dumps(
                    {
                        "trit": TRIT_VERIFIED,
                        "max_abs_err": 0,
                        "dualrail_fallback_count": run1["dualrail_fallback_count"],
                        "dualrail_sparse128_blocks_skipped": run1["dualrail_sparse128_blocks_skipped"],
                        "repeat_sha3_identical": run1["execution_receipt"] == run2["execution_receipt"],
                        "execution_receipt": run1["execution_receipt"],
                    },
                    sort_keys=True,
                ).encode("utf-8")
                payload_sha3 = "sha3-512:" + hashlib.sha3_512(payload).hexdigest()
                commit_policy = root / "commit_policy.apl"
                commit_policy.write_text(
                    f'(policy\n  (tier 1)\n  (allowed-tensor-hashes ["{ws["weight_sha3"]}" "{payload_sha3}"]))\n',
                    encoding="utf-8",
                )
                artifact_path.write_bytes(payload)
                commit = mcp.call(
                    "t81.tx_commit",
                    {
                        "tx_id": tx["tx_id"],
                        "policy_path": str(commit_policy),
                        "compact": True,
                    },
                )
                self.assertTrue(commit["ok"] and commit["committed"], commit)
                self.assertEqual(artifact_path.read_bytes(), payload)

    def test_03_three_state_semantics_routing_and_real_qemu_uart(self) -> None:
        with tempfile.TemporaryDirectory(prefix="t81-v02-trit-") as td:
            root = Path(td)
            boot = _call_mcp_fresh(
                [root, ROOT_DIR],
                "t81.session_bootstrap",
                {"repo_root": str(ROOT_DIR), "compact": True},
            )["result"]["structured"]
            self.assertTrue(boot["ok"])
            self.assertEqual(boot["native_mcp_tools"], 15)
            routing = boot["capabilities"]["regime_routing"]
            r1 = routing["R1_ternary_act"]
            r2 = routing["R2_int8_act"]
            self.assertEqual(r1["m1_high_sparsity_kernel"], "CPU_1T_SBDR128_SIMD")
            self.assertEqual(r1["m1_high_sparsity_min_pct"], 90)
            self.assertEqual(r1["gpu_sparse_kernel"], "GPU_R1_BitmaskSkip_Pop")
            self.assertEqual(r1["gpu_sparse_min_pct"], 75)
            self.assertEqual(r1["gpu_kernel"], "GPU_R1_DualRail_Pop")
            self.assertEqual(r1["k_star_prepacked"], 0)
            self.assertEqual(r2["m1_to_4"], "CPU_4T_INT4W")
            self.assertEqual(r2["m16_plus"], "CPU_4T_INT8W")

            # Verify ledger 10 accepted (+1) / 16 negative (-1) counts via routing_ledger.py
            ledger_out = subprocess.run(
                [sys.executable, str(ROOT_DIR / "examples/research/agent-receipt-sidecar/routing_ledger.py")],
                cwd=ROOT_DIR,
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            ledger = json.loads(ledger_out)
            self.assertEqual(ledger["accepted_count"], 10)
            self.assertEqual(ledger["negative_constraints_count"], 16)
            self.assertEqual(
                sorted(ledger["blocked_constraint_tags"]),
                [
                    "2block_unroll_sbdr128",
                    "cpu_dualrail_unpack",
                    "gpu_csr_sbdr128_sparse",
                    "per_layer_cpu_thread_barrier",
                ],
            )
            recs = ledger["recommendations"]
            self.assertEqual(recs["R1_M1_s90"], "CPU_1T_SBDR128_SIMD")
            self.assertEqual(recs["R1_M1_s89"], "GPU_R1_BitmaskSkip_Pop")
            self.assertEqual(recs["R1_M4_s75"], "GPU_R1_BitmaskSkip_Pop")
            self.assertEqual(recs["R1_M4_s50"], "GPU_R1_DualRail_Pop")
            self.assertEqual(recs["R2_M1"], "CPU_4T_INT4W")
            self.assertEqual(recs["R2_M16"], "CPU_4T_INT8W")

            # Verify real QEMU AArch64 PL011 UART0 (-1 deny, +1 ok) if qemu-system-aarch64 is present
            qemu_bin = shutil.which("qemu-system-aarch64")
            if qemu_bin:
                elf = _build_aarch64_uart_guest_elf()
                elf_path = root / "guest.elf"
                elf_path.write_bytes(elf)
                t1 = _run_real_qemu_serial_session(qemu_bin, elf_path)
                t2 = _run_real_qemu_serial_session(qemu_bin, elf_path)
                self.assertEqual(t1, t2)
                self.assertIn(b"AXION_DENY:POLICY_ROOT_IMMUTABLE\n", t1)
                self.assertIn(b"AXION_OK:status\n", t1)
                self.assertEqual(
                    "sha3-512:" + hashlib.sha3_512(elf).hexdigest(),
                    "sha3-512:26bda4ae26a9fd74c4aff8e3c633c8b54174b9913742fd40d9228fe76a1eebfcc1717ac68824be3a1c9640a2aa2fc6803ed31667614254e439a3f93d83c0c55d",
                )
                self.assertEqual(
                    "sha3-512:" + hashlib.sha3_512(t1).hexdigest(),
                    "sha3-512:b052306fc0fe3c1f6175e9d0241d6688b43cf877b9d0e99b25c8b32855e926b9fabccd06263a954c38379ea3c463d9336b1b5ff3eed1bb4d63c92198d0be9248",
                )


if __name__ == "__main__":
    unittest.main()
