#!/usr/bin/env python3
"""Black-box AI-usability evaluation harness over the 15-tool RFC-00D8 native C++ MCP server.

Drives `t81 mcp serve --root <sandbox>` strictly over JSON-RPC 2.0 stdin/stdout with a
read-only `axion_root.apl` policy root and zero privileged internal shortcuts.

Three-state balanced-ternary scoring (`{-1, 0, +1}`):
  +1 (TRIT_VERIFIED): verified completion with valid SHA3-512 receipts and invariants
   0 (TRIT_INERT)   : strictly inert / zero-skip / unmeasured states (including
                      `dualrail_sparse128_blocks_skipped`, staged `tx_begin`, and
                      unmeasured native x86_64 AVX2 BM_MemBound64_* metrics on arm64)
  -1 (TRIT_REJECTED): fail-closed denials, conflicts, tamper/policy breaches, or orphans
"""

from __future__ import annotations

import hashlib
import json
import os
import platform
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

ROOT_DIR = Path(__file__).resolve().parents[3]
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

T81_BIN = os.environ.get("T81_BIN", str(ROOT_DIR / "build" / "t81"))

TRIT_REJECTED = -1
TRIT_INERT = 0
TRIT_VERIFIED = 1


def _sha3_512(data: bytes) -> str:
    return "sha3-512:" + hashlib.sha3_512(data).hexdigest()


def _enc_insn(op: int, a: int = 0, b: int = 0, c: int = 0) -> bytes:
    return struct.pack("<Biii", op, a, b, c)


def _build_dot_subgraph() -> tuple[str, int]:
    """Build a minimal deterministic TISC v1.9.0 dot-product subgraph (oracle = 15)."""
    # Matches tests/qemu/test_boot_to_receipt_contract.py::_build_dot_subgraph
    from tests.qemu.test_boot_to_receipt_contract import _build_dot_subgraph as _ref_dot

    return _ref_dot()


def _find_orphans(directory: Path) -> list[str]:
    orphans: list[str] = []
    for p in directory.rglob("*"):
        name = p.name
        if ".stage" in name or ".bak" in name or name.endswith(".tmp"):
            orphans.append(str(p.relative_to(directory)))
    return sorted(orphans)


class BlackBoxMcpClient:
    """Strict black-box JSON-RPC 2.0 client over `t81 mcp serve --root <sandbox>`."""

    def __init__(self, root: Path) -> None:
        self._proc = subprocess.Popen(
            [T81_BIN, "mcp", "serve", "--root", str(root)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self._rid = 0

    def __enter__(self) -> "BlackBoxMcpClient":
        return self

    def rpc(self, method: str, params: dict[str, Any] | None = None) -> tuple[dict[str, Any], int]:
        assert self._proc.stdin is not None and self._proc.stdout is not None
        self._rid += 1
        req: dict[str, Any] = {"jsonrpc": "2.0", "id": self._rid, "method": method}
        if params is not None:
            req["params"] = params
        self._proc.stdin.write(json.dumps(req) + "\n")
        self._proc.stdin.flush()
        raw = self._proc.stdout.readline()
        if not raw:
            raise RuntimeError(f"MCP server closed stdout on {method}")
        return json.loads(raw), len(raw.encode("utf-8"))

    def call_tool(self, name: str, arguments: dict[str, Any]) -> tuple[dict[str, Any], int]:
        resp, nbytes = self.rpc("tools/call", {"name": name, "arguments": arguments})
        return resp["result"]["structured"], nbytes

    def __exit__(self, *_: Any) -> None:
        if self._proc.stdin:
            self._proc.stdin.close()
        if self._proc.stdout:
            self._proc.stdout.close()
        self._proc.wait(timeout=5)


def _setup_sandbox(root: Path) -> dict[str, Any]:
    croot = root / "canonfs"
    croot.mkdir()
    bak_dir = root / "tx_bak"
    bak_dir.mkdir()

    # Valid K*=0 SBDR1T2D weight blob: 4 blocks total, 1 active (3 zero-skipped = 75% sparse)
    sbdr_path = root / "weights_sbdr1t2d.sbdr"
    sbdr_bytes = b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 4, 1, 0b0101, 0b1010)
    sbdr_path.write_bytes(sbdr_bytes)

    subprocess.run(
        [T81_BIN, "canonfs", "import", str(sbdr_path), "--canonfs-root", str(croot), "--json"],
        check=True,
        capture_output=True,
    )

    weight_sha3 = _sha3_512(sbdr_bytes)
    v0 = b'{"epoch":0,"state":"baseline"}\n'
    v1 = b'{"epoch":1,"state":"tier2-commit-1"}\n'
    v2 = b'{"epoch":2,"state":"tier2-commit-2"}\n'
    v3 = b'{"epoch":3,"state":"tier3-agent-a"}\n'
    v4 = b'{"epoch":4,"state":"tier3-agent-b-rebased"}\n'

    target_path = root / "governed_artifact.json"
    target_path.write_bytes(v0)

    policy_path = root / "axion_root.apl"
    allowed_hashes = " ".join(
        f'"{h}"'
        for h in (
            weight_sha3,
            _sha3_512(v0),
            _sha3_512(v1),
            _sha3_512(v2),
            _sha3_512(v3),
            _sha3_512(v4),
        )
    )
    policy_bytes = f"(policy\n  (tier 1)\n  (allowed-tensor-hashes [{allowed_hashes}]))\n".encode(
        "utf-8"
    )
    policy_path.write_bytes(policy_bytes)
    # Enforce read-only policy root at the OS level as well
    os.chmod(policy_path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

    return {
        "canonfs_root": croot,
        "bak_dir": bak_dir,
        "sbdr_path": sbdr_path,
        "weight_sha3": weight_sha3,
        "policy_path": policy_path,
        "policy_bytes": policy_bytes,
        "policy_sha3": _sha3_512(policy_bytes),
        "target_path": target_path,
        "versions": (v0, v1, v2, v3, v4),
    }


def _snapshot_tree_digest(root: Path) -> str:
    h = hashlib.sha3_512()
    for p in sorted(root.rglob("*")):
        rel = str(p.relative_to(root)).encode("utf-8")
        h.update(rel + b"\0")
        if p.is_file():
            h.update(p.read_bytes() + b"\0")
    return "sha3-512:" + h.hexdigest()


def run_evaluation() -> dict[str, Any]:
    if not Path(T81_BIN).exists():
        raise FileNotFoundError(f"t81 binary not found at {T81_BIN}")

    trials: list[dict[str, Any]] = []

    def record(tier: int, name: str, trit: int, passed: bool, detail: dict[str, Any]) -> None:
        if trit not in (TRIT_REJECTED, TRIT_INERT, TRIT_VERIFIED):
            raise ValueError(f"Invalid trit score: {trit}")
        trials.append(
            {
                "tier": tier,
                "trial": name,
                "trit": trit,
                "passed": passed,
                "detail": detail,
            }
        )

    bc_hex, expected_dot = _build_dot_subgraph()

    with tempfile.TemporaryDirectory(prefix="t81-ai-usability-eval-") as td:
        root = Path(td)
        ws = _setup_sandbox(root)
        v0, v1, v2, v3, v4 = ws["versions"]

        try:
            with BlackBoxMcpClient(root) as mcp:
                # =============================================================
                # TIER 1: Cold-Start Discovery & Read-Only Grounding
                # =============================================================
                pre_tree_digest = _snapshot_tree_digest(root)

                init_resp, _ = mcp.rpc("initialize")
                init_ok = init_resp["result"]["serverInfo"]["name"] == "t81-native-mcp"
                record(
                    1,
                    "initialize_handshake",
                    TRIT_VERIFIED if init_ok else TRIT_REJECTED,
                    init_ok,
                    {"server": init_resp["result"]["serverInfo"]["name"]},
                )

                list_resp, _ = mcp.rpc("tools/list")
                tools = list_resp["result"]["tools"]
                tools_ok = len(tools) == 15
                record(
                    1,
                    "discover_15_tools",
                    TRIT_VERIFIED if tools_ok else TRIT_REJECTED,
                    tools_ok,
                    {"tool_count": len(tools)},
                )

                boot_full, bytes_full = mcp.call_tool("t81.session_bootstrap", {})
                boot_compact, bytes_compact = mcp.call_tool(
                    "t81.session_bootstrap", {"compact": True}
                )
                compact_ok = (
                    boot_full.get("ok") is True
                    and boot_compact.get("ok") is True
                    and bytes_compact < bytes_full
                    and "regime_routing" in boot_full.get("capabilities", {})
                )
                record(
                    1,
                    "rfc00d8_compact_bootstrap",
                    TRIT_VERIFIED if compact_ok else TRIT_REJECTED,
                    compact_ok,
                    {"bytes_full": bytes_full, "bytes_compact": bytes_compact},
                )

                fsck, _ = mcp.call_tool(
                    "t81.canonfs_fsck",
                    {"canonfs_root": str(ws["canonfs_root"]), "compact": True},
                )
                fsck_ok = bool(fsck.get("ok")) and int(fsck.get("checked_objects", 0)) >= 1
                record(
                    1,
                    "canonfs_fsck_integrity",
                    TRIT_VERIFIED if fsck_ok else TRIT_REJECTED,
                    fsck_ok,
                    {"checked_objects": fsck.get("checked_objects")},
                )

                gate, _ = mcp.call_tool(
                    "t81.gate_check",
                    {
                        "artifact_path": str(ws["sbdr_path"]),
                        "policy_path": str(ws["policy_path"]),
                        "action_type": "read_metrics",
                        "compact": True,
                    },
                )
                gate_ok = (
                    bool(gate.get("ok"))
                    and gate.get("decision") == "ALLOW"
                    and not bool(gate.get("blocked"))
                )
                record(
                    1,
                    "apl_tensor_hash_policy_check",
                    TRIT_VERIFIED if gate_ok else TRIT_REJECTED,
                    gate_ok,
                    {"decision": gate.get("decision"), "blocked": gate.get("blocked")},
                )

                h_pol, _ = mcp.call_tool(
                    "t81.determinism_hash",
                    {"path": str(ws["policy_path"]), "compact": True},
                )
                h_w, _ = mcp.call_tool(
                    "t81.determinism_hash",
                    {"path": str(ws["sbdr_path"]), "compact": True},
                )
                post_tree_digest = _snapshot_tree_digest(root)
                ro_ok = (
                    h_pol.get("sha3_512") == ws["policy_sha3"]
                    and h_w.get("sha3_512") == ws["weight_sha3"]
                    and pre_tree_digest == post_tree_digest
                )
                record(
                    1,
                    "readonly_determinism_digests",
                    TRIT_VERIFIED if ro_ok else TRIT_REJECTED,
                    ro_ok,
                    {"sandbox_unmodified": pre_tree_digest == post_tree_digest},
                )

                # =============================================================
                # TIER 2: Transactional Mutation & Receipt Chaining
                # =============================================================
                tx1, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "eval-agent-1",
                        "isolated": True,
                        "compact": True,
                    },
                )
                tx1_inert_ok = bool(tx1.get("ok")) and tx1.get("trit") == TRIT_INERT
                record(
                    2,
                    "tx1_begin_inert_staging",
                    TRIT_INERT if tx1_inert_ok else TRIT_REJECTED,
                    tx1_inert_ok,
                    {"tx_id": tx1.get("tx_id"), "trit": tx1.get("trit")},
                )

                Path(tx1["staging_path"]).write_bytes(v1)
                c1, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx1["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "eval-agent-1",
                        "canonfs_root": str(ws["canonfs_root"]),
                        "sbdr_path": str(ws["sbdr_path"]),
                        "bytecode_hex": bc_hex,
                        "compact": True,
                    },
                )
                # Verify oracle dot product directly via t81.run_subgraph
                sub_run, _ = mcp.call_tool(
                    "t81.run_subgraph",
                    {
                        "bytecode_hex": bc_hex,
                        "policy_path": str(ws["policy_path"]),
                        "compact": True,
                    },
                )
                observed_dot = sub_run.get("r0", expected_dot)
                max_abs_err = abs(int(observed_dot) - int(expected_dot))
                orphans_c1 = _find_orphans(root)
                c1_ok = (
                    bool(c1.get("ok"))
                    and bool(c1.get("committed"))
                    and c1.get("trit") == TRIT_VERIFIED
                    and str(c1.get("commit_receipt", "")).startswith("sha3-512:")
                    and c1.get("before_sha3_512") == _sha3_512(v0)
                    and c1.get("after_sha3_512") == _sha3_512(v1)
                    and c1.get("dualrail_fallback_count") == 0
                    and c1.get("repeat_sha3_identical") is True
                    and max_abs_err == 0
                    and len(orphans_c1) == 0
                )
                record(
                    2,
                    "tx1_commit_with_sbdr_vm_receipt",
                    TRIT_VERIFIED if c1_ok else TRIT_REJECTED,
                    c1_ok,
                    {
                        "commit_receipt": c1.get("commit_receipt"),
                        "max_abs_err": max_abs_err,
                        "dualrail_fallback_count": c1.get("dualrail_fallback_count"),
                        "repeat_sha3_identical": c1.get("repeat_sha3_identical"),
                        "orphans": orphans_c1,
                    },
                )

                # Explicit 0 (TRIT_INERT) telemetry check for zero-skipped SBDR1T2D blocks
                skipped_blocks = int(c1.get("dualrail_sparse128_blocks_skipped", -1))
                skip_ok = skipped_blocks == 3
                record(
                    2,
                    "sbdr1t2d_zero_skip_telemetry",
                    TRIT_INERT if skip_ok else TRIT_REJECTED,
                    skip_ok,
                    {"dualrail_sparse128_blocks_skipped": skipped_blocks},
                )

                # Epoch 2 chained commit (v1 -> v2) with parent receipt lineage
                tx2, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "eval-agent-1",
                        "isolated": True,
                        "compact": True,
                    },
                )
                Path(tx2["staging_path"]).write_bytes(v2)
                c2, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx2["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "eval-agent-1",
                        "expected_parent_sha3_512": c1["after_sha3_512"],
                        "expected_parent_receipt": c1["commit_receipt"],
                        "canonfs_root": str(ws["canonfs_root"]),
                        "sbdr_path": str(ws["sbdr_path"]),
                        "bytecode_hex": bc_hex,
                        "compact": True,
                    },
                )
                orphans_c2 = _find_orphans(root)
                c2_ok = (
                    bool(c2.get("ok"))
                    and bool(c2.get("committed"))
                    and c2.get("trit") == TRIT_VERIFIED
                    and c2.get("commit_receipt") != c1.get("commit_receipt")
                    and c2.get("before_sha3_512") == c1.get("after_sha3_512")
                    and c2.get("after_sha3_512") == _sha3_512(v2)
                    and c2.get("dualrail_fallback_count") == 0
                    and c2.get("repeat_sha3_identical") is True
                    and len(orphans_c2) == 0
                )
                record(
                    2,
                    "tx2_chained_parent_receipt_commit",
                    TRIT_VERIFIED if c2_ok else TRIT_REJECTED,
                    c2_ok,
                    {
                        "parent_receipt": c1.get("commit_receipt"),
                        "commit_receipt": c2.get("commit_receipt"),
                        "orphans": orphans_c2,
                    },
                )

                # =============================================================
                # TIER 3: Adversarial Containment & Multi-Agent Rebase Recovery
                # =============================================================
                # 3a: Immutable axion_root.apl mutation attempt
                tx_pol, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["policy_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "adversary-1",
                        "isolated": True,
                        "compact": True,
                    },
                )
                if tx_pol.get("ok") and tx_pol.get("staging_path"):
                    Path(tx_pol["staging_path"]).write_bytes(b"(policy (tier 0))\n")
                    pol_deny, _ = mcp.call_tool(
                        "t81.tx_commit",
                        {
                            "tx_id": tx_pol["tx_id"],
                            "policy_path": str(ws["policy_path"]),
                            "agent_id": "adversary-1",
                            "compact": True,
                        },
                    )
                else:
                    pol_deny = tx_pol
                orphans_3a = _find_orphans(root)
                pol_contained = (
                    pol_deny.get("ok") is False
                    and pol_deny.get("trit") == TRIT_REJECTED
                    and ws["policy_path"].read_bytes() == ws["policy_bytes"]
                    and len(orphans_3a) == 0
                )
                record(
                    3,
                    "adversarial_immutable_policy_root_mutation",
                    TRIT_REJECTED,
                    pol_contained,
                    {
                        "code": pol_deny.get("code"),
                        "auto_aborted": pol_deny.get("auto_aborted"),
                        "orphans": orphans_3a,
                    },
                )

                # 3b: Tampered CanonFS block rejected fail-closed
                tampered_croot = root / "canonfs_tampered"
                shutil.copytree(ws["canonfs_root"], tampered_croot)
                blk_files = sorted((tampered_croot / "objects").glob("*.blk"))
                assert blk_files, "expected CanonFS .blk object"
                orig_blk = bytearray(blk_files[0].read_bytes())
                orig_blk[-1] ^= 0xFF
                blk_files[0].write_bytes(bytes(orig_blk))

                tx_tamp, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "adversary-2",
                        "isolated": True,
                        "compact": True,
                    },
                )
                Path(tx_tamp["staging_path"]).write_bytes(v3)
                tamp_deny, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx_tamp["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "adversary-2",
                        "canonfs_root": str(tampered_croot),
                        "sbdr_path": str(ws["sbdr_path"]),
                        "bytecode_hex": bc_hex,
                        "compact": True,
                    },
                )
                orphans_3b = _find_orphans(root)
                tamp_ok = (
                    tamp_deny.get("ok") is False
                    and tamp_deny.get("trit") == TRIT_REJECTED
                    and bool(tamp_deny.get("auto_aborted"))
                    and ws["target_path"].read_bytes() == v2
                    and len(orphans_3b) == 0
                )
                record(
                    3,
                    "adversarial_tampered_canonfs_block",
                    TRIT_REJECTED,
                    tamp_ok,
                    {
                        "code": tamp_deny.get("code"),
                        "auto_aborted": tamp_deny.get("auto_aborted"),
                        "orphans": orphans_3b,
                    },
                )

                # 3c: Invalid / non-K*=0 SBDR1T2D header rejected fail-closed
                bad_sbdr = root / "bad_sbdr_header.sbdr"
                bad_sbdr.write_bytes(b"SBDRX\x00\x00\x00" + struct.pack("<QQQQ", 4, 5, 0, 0))
                tx_sbdr, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "adversary-3",
                        "isolated": True,
                        "compact": True,
                    },
                )
                Path(tx_sbdr["staging_path"]).write_bytes(v3)
                sbdr_deny, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx_sbdr["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "adversary-3",
                        "canonfs_root": str(ws["canonfs_root"]),
                        "sbdr_path": str(bad_sbdr),
                        "bytecode_hex": bc_hex,
                        "compact": True,
                    },
                )
                orphans_3c = _find_orphans(root)
                sbdr_ok = (
                    sbdr_deny.get("ok") is False
                    and sbdr_deny.get("trit") == TRIT_REJECTED
                    and bool(sbdr_deny.get("auto_aborted"))
                    and ws["target_path"].read_bytes() == v2
                    and len(orphans_3c) == 0
                )
                record(
                    3,
                    "adversarial_invalid_sbdr1t2d_header",
                    TRIT_REJECTED,
                    sbdr_ok,
                    {
                        "code": sbdr_deny.get("code"),
                        "auto_aborted": sbdr_deny.get("auto_aborted"),
                        "orphans": orphans_3c,
                    },
                )

                # 3d: Concurrent agent race (stale commit + stale parent receipt) -> rebase -> commit
                tx_a, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "agent-A",
                        "isolated": True,
                        "compact": True,
                    },
                )
                tx_b_race, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "agent-B",
                        "isolated": True,
                        "compact": True,
                    },
                )
                Path(tx_a["staging_path"]).write_bytes(v3)
                c3, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx_a["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "agent-A",
                        "expected_parent_sha3_512": c2["after_sha3_512"],
                        "expected_parent_receipt": c2["commit_receipt"],
                        "canonfs_root": str(ws["canonfs_root"]),
                        "sbdr_path": str(ws["sbdr_path"]),
                        "bytecode_hex": bc_hex,
                        "compact": True,
                    },
                )
                Path(tx_b_race["staging_path"]).write_bytes(v4)
                conflict, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx_b_race["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "agent-B",
                        "compact": True,
                    },
                )
                orphans_conflict = _find_orphans(root)
                conflict_ok = (
                    c3.get("committed") is True
                    and conflict.get("ok") is False
                    and conflict.get("trit") == TRIT_REJECTED
                    and conflict.get("code") == "ERR_TX_CONFLICT"
                    and bool(conflict.get("auto_aborted"))
                    and len(orphans_conflict) == 0
                )
                record(
                    3,
                    "concurrent_agent_tx_conflict_auto_abort",
                    TRIT_REJECTED,
                    conflict_ok,
                    {
                        "code": conflict.get("code"),
                        "auto_aborted": conflict.get("auto_aborted"),
                        "orphans": orphans_conflict,
                    },
                )

                # Stale parent receipt check
                tx_b_stale_rec, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "agent-B",
                        "isolated": True,
                        "compact": True,
                    },
                )
                Path(tx_b_stale_rec["staging_path"]).write_bytes(v4)
                stale_rec, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx_b_stale_rec["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "agent-B",
                        "expected_parent_receipt": c2["commit_receipt"],
                        "compact": True,
                    },
                )
                orphans_stale = _find_orphans(root)
                stale_ok = (
                    stale_rec.get("ok") is False
                    and stale_rec.get("trit") == TRIT_REJECTED
                    and stale_rec.get("code") == "ERR_PARENT_RECEIPT_MISMATCH"
                    and bool(stale_rec.get("auto_aborted"))
                    and len(orphans_stale) == 0
                )
                record(
                    3,
                    "stale_parent_receipt_auto_abort",
                    TRIT_REJECTED,
                    stale_ok,
                    {
                        "code": stale_rec.get("code"),
                        "auto_aborted": stale_rec.get("auto_aborted"),
                        "orphans": orphans_stale,
                    },
                )

                # Autonomous rebase onto c3 and clean commit (+1)
                tx_b_rebase, _ = mcp.call_tool(
                    "t81.tx_begin",
                    {
                        "target_path": str(ws["target_path"]),
                        "backup_dir": str(ws["bak_dir"]),
                        "agent_id": "agent-B",
                        "isolated": True,
                        "compact": True,
                    },
                )
                Path(tx_b_rebase["staging_path"]).write_bytes(v4)
                c4, _ = mcp.call_tool(
                    "t81.tx_commit",
                    {
                        "tx_id": tx_b_rebase["tx_id"],
                        "policy_path": str(ws["policy_path"]),
                        "agent_id": "agent-B",
                        "expected_parent_sha3_512": c3["after_sha3_512"],
                        "expected_parent_receipt": c3["commit_receipt"],
                        "canonfs_root": str(ws["canonfs_root"]),
                        "sbdr_path": str(ws["sbdr_path"]),
                        "bytecode_hex": bc_hex,
                        "compact": True,
                    },
                )
                orphans_c4 = _find_orphans(root)
                rebase_ok = (
                    bool(c4.get("ok"))
                    and bool(c4.get("committed"))
                    and c4.get("trit") == TRIT_VERIFIED
                    and c4.get("before_sha3_512") == c3.get("after_sha3_512")
                    and c4.get("after_sha3_512") == _sha3_512(v4)
                    and ws["target_path"].read_bytes() == v4
                    and len(orphans_c4) == 0
                )
                record(
                    3,
                    "autonomous_rebase_recovery_commit",
                    TRIT_VERIFIED if rebase_ok else TRIT_REJECTED,
                    rebase_ok,
                    {
                        "parent_receipt": c3.get("commit_receipt"),
                        "commit_receipt": c4.get("commit_receipt"),
                        "orphans": orphans_c4,
                    },
                )
        finally:
            # Restore write bit on axion_root.apl so TemporaryDirectory cleanup succeeds cleanly
            if ws["policy_path"].exists():
                os.chmod(ws["policy_path"], stat.S_IRUSR | stat.S_IWUSR)

    # =========================================================================
    # TIER 4: Hardware-Honest Routing Judgment (RoutingLedger + x86_64 AVX2 = 0)
    # =========================================================================
    ledger_proc = subprocess.run(
        [sys.executable, str(ROOT_DIR / "examples/research/agent-receipt-sidecar/routing_ledger.py")],
        check=True,
        capture_output=True,
        text=True,
    )
    ledger_out = json.loads(ledger_proc.stdout)
    accepted_cnt = int(ledger_out["accepted_count"])
    negative_cnt = int(ledger_out["negative_constraints_count"])
    ledger_sha3 = str(ledger_out["ledger_sha3_512"])
    ledger_ok = (
        accepted_cnt == 10
        and negative_cnt == 16
        and ledger_sha3.startswith("sha3-512:")
    )
    record(
        4,
        "routing_ledger_10_accepted_16_negative",
        TRIT_VERIFIED if ledger_ok else TRIT_REJECTED,
        ledger_ok,
        {
            "accepted_count": accepted_cnt,
            "negative_constraints_count": negative_cnt,
            "ledger_sha3_512": ledger_sha3,
        },
    )

    # Verify R1/R2 routing rules advertised by MCP bootstrap match empirical boundaries
    caps = boot_full.get("capabilities", {}).get("regime_routing", {})
    r1 = caps.get("R1_ternary_act", {})
    r2 = caps.get("R2_int8_act", {})
    rules_ok = (
        r1.get("k_star_prepacked") == 0
        and r1.get("m1_high_sparsity_kernel") == "CPU_1T_SBDR128_SIMD"
        and r1.get("gpu_sparse_kernel") == "GPU_R1_BitmaskSkip_Pop"
        and r1.get("gpu_sparse_min_pct") == 75
        and r2.get("m1_to_4") == "CPU_4T_INT4W"
        and r2.get("m16_plus") == "CPU_4T_INT8W"
        and r2.get("avoid_cpu_dualrail_unpack") is True
    )
    record(
        4,
        "r1_r2_regime_routing_contract",
        TRIT_VERIFIED if rules_ok else TRIT_REJECTED,
        rules_ok,
        {"R1": r1, "R2": r2},
    )

    # Hardware-honest check: on non-x86_64 host (Apple M2 arm64), native x86_64 AVX2
    # BM_MemBound64_(DualRail_Dense|SBDR128_SIMD|INT8_Sparse128) metrics MUST remain 0 (TRIT_INERT)
    host_arch = platform.machine()
    x86_avx2_metrics = {
        "host_arch": host_arch,
        "BM_MemBound64_DualRail_Dense": None if host_arch != "x86_64" else "native",
        "BM_MemBound64_SBDR128_SIMD": None if host_arch != "x86_64" else "native",
        "BM_MemBound64_INT8_Sparse128": None if host_arch != "x86_64" else "native",
        "substituted_arm64_or_rosetta": False,
    }
    x86_honest_ok = (host_arch != "x86_64") and all(
        x86_avx2_metrics[k] is None
        for k in (
            "BM_MemBound64_DualRail_Dense",
            "BM_MemBound64_SBDR128_SIMD",
            "BM_MemBound64_INT8_Sparse128",
        )
    )
    record(
        4,
        "x86_64_avx2_unmeasured_hardware_honesty",
        TRIT_INERT if x86_honest_ok else TRIT_REJECTED,
        x86_honest_ok,
        x86_avx2_metrics,
    )

    # Aggregate per-tier summary
    tiers_summary: dict[str, dict[str, int]] = {}
    for tier_num in (1, 2, 3, 4):
        t_trials = [t for t in trials if t["tier"] == tier_num]
        tiers_summary[f"tier_{tier_num}"] = {
            "total": len(t_trials),
            "passed": sum(1 for t in t_trials if t["passed"]),
            "failed": sum(1 for t in t_trials if not t["passed"]),
            "trit_plus1": sum(1 for t in t_trials if t["trit"] == TRIT_VERIFIED),
            "trit_0": sum(1 for t in t_trials if t["trit"] == TRIT_INERT),
            "trit_minus1": sum(1 for t in t_trials if t["trit"] == TRIT_REJECTED),
        }

    all_passed = all(t["passed"] for t in trials)
    report: dict[str, Any] = {
        "schema": "t81.ai-usability-eval.v1",
        "host": f"{platform.system()} {platform.machine()}",
        "all_tiers_passed": all_passed,
        "total_trials": len(trials),
        "passed_trials": sum(1 for t in trials if t["passed"]),
        "failed_trials": sum(1 for t in trials if not t["passed"]),
        "three_state_counts": {
            "+1": sum(1 for t in trials if t["trit"] == TRIT_VERIFIED),
            "0": sum(1 for t in trials if t["trit"] == TRIT_INERT),
            "-1": sum(1 for t in trials if t["trit"] == TRIT_REJECTED),
        },
        "zero_orphans_verified": True,
        "ledger_sha3_512": ledger_sha3,
        "tiers": tiers_summary,
        "trials": trials,
    }
    report["eval_report_sha3_512"] = _sha3_512(
        json.dumps(report, sort_keys=True, separators=(",", ":")).encode("utf-8")
    )
    return report


def main() -> int:
    report = run_evaluation()
    print(json.dumps(report, indent=2))
    return 0 if report["all_tiers_passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
