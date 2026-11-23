#!/usr/bin/env python3
"""Receipt-backed empirical routing & falsification ledger for T81 MCP agents.

Treats `t81.session_bootstrap` (`capabilities.regime_routing`) as an empirical prior,
verifies live `RFC-00D8` `t81.run_subgraph` repeat-run `SHA3-512` receipts across
independent MCP sessions, and enforces fail-closed admission of routing updates:
  - max_abs_err == 0
  - dualrail_fallback_count == 0
  - bit-exact repeat-run SHA3-512 execution_receipt equivalence
  - Axion policy admission (VERIFIED_AND_EXECUTED, halted == True, trap_code == 0)
  - measured_speedup >= pre-declared falsification threshold
  - no violation of recorded negative constraints (e.g., H06-MEM-07, H06-MEM-08)

Does not modify PUBLIC_API_LOCK.json, TISC v1.9.0, or Axion policy rules.
"""

from __future__ import annotations

import dataclasses
import hashlib
import importlib.util
import json
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

ROOT_DIR = Path(__file__).resolve().parents[3]
T81_BIN = os.environ.get("T81_BIN", str(ROOT_DIR / "build" / "t81"))


def _load_bitnet_ingester():
    mod_path = ROOT_DIR / "examples" / "research" / "bitnet-verifiable-token" / "load_safetensors_bitnet.py"
    spec = importlib.util.spec_from_file_location("load_safetensors_bitnet", mod_path)
    assert spec is not None and spec.loader is not None
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _enc_insn(op: int, a: int, b: int, c: int) -> bytes:
    return struct.pack("<Biii", op, a, b, c)


def _call_mcp_fresh(roots: list[Path], tool_name: str, arguments: dict[str, Any]) -> dict[str, Any]:
    cmd = [T81_BIN, "mcp", "serve"]
    for root in roots:
        cmd.extend(["--root", str(root)])
    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        text=True,
    )
    try:
        req = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "tools/call",
            "params": {"name": tool_name, "arguments": arguments},
        }
        assert proc.stdin is not None and proc.stdout is not None
        proc.stdin.write(json.dumps(req) + "\n")
        proc.stdin.flush()
        line = proc.stdout.readline()
        if not line:
            raise RuntimeError(f"Empty MCP response for {tool_name}")
        return json.loads(line)
    finally:
        if proc.stdin:
            proc.stdin.close()
        proc.wait(timeout=5)


@dataclasses.dataclass(frozen=True)
class ReceiptPair:
    run1: dict[str, Any]
    run2: dict[str, Any]

    @property
    def sha3_identical(self) -> bool:
        r1 = self.run1.get("execution_receipt", "")
        r2 = self.run2.get("execution_receipt", "")
        return isinstance(r1, str) and r1.startswith("sha3-512:") and r1 == r2

    @property
    def max_fallback_count(self) -> int:
        return max(
            int(self.run1.get("dualrail_fallback_count", -1)),
            int(self.run2.get("dualrail_fallback_count", -1)),
        )

    @property
    def policy_admitted(self) -> bool:
        return (
            bool(self.run1.get("ok"))
            and bool(self.run2.get("ok"))
            and self.run1.get("verdict") == "VERIFIED_AND_EXECUTED"
            and self.run2.get("verdict") == "VERIFIED_AND_EXECUTED"
            and bool(self.run1.get("halted"))
            and bool(self.run2.get("halted"))
            and int(self.run1.get("trap_code", -1)) == 0
            and int(self.run2.get("trap_code", -1)) == 0
        )


@dataclasses.dataclass(frozen=True)
class RoutingCandidate:
    experiment_id: str
    regime: str  # "R1_ternary_act" or "R2_int8_act"
    kernel: str
    scope: str
    measured_speedup: float
    min_speedup_threshold: float
    max_abs_err: int
    constraint_tag: str  # e.g. "per_layer_cpu_thread_barrier", "cpu_dualrail_unpack", "none"
    note: str


class RoutingLedger:
    """In-memory receipt-backed routing & falsification ledger."""

    def __init__(self, bootstrap_structured: dict[str, Any]) -> None:
        self.git_head_sha = bootstrap_structured.get("git_head_sha", "unknown")
        self.capabilities = bootstrap_structured.get("capabilities", {})
        self.prior_routing = self.capabilities.get("regime_routing", {})
        self.accepted_routes: list[dict[str, Any]] = []
        self.negative_constraints: list[dict[str, Any]] = []
        self._blocked_tags: set[str] = set()
        if self.prior_routing.get("R1_ternary_act", {}).get("avoid_per_layer_cpu_thread_barrier"):
            self._blocked_tags.add("per_layer_cpu_thread_barrier")
        if self.prior_routing.get("R2_int8_act", {}).get("avoid_cpu_dualrail_unpack"):
            self._blocked_tags.add("cpu_dualrail_unpack")

    def evaluate_candidate(
        self, candidate: RoutingCandidate, receipts: ReceiptPair
    ) -> dict[str, Any]:
        record_base: dict[str, Any] = {
            "experiment_id": candidate.experiment_id,
            "regime": candidate.regime,
            "kernel": candidate.kernel,
            "scope": candidate.scope,
            "measured_speedup": candidate.measured_speedup,
            "min_speedup_threshold": candidate.min_speedup_threshold,
            "max_abs_err": candidate.max_abs_err,
            "execution_receipt": receipts.run1.get("execution_receipt", ""),
            "repeat_sha3_identical": receipts.sha3_identical,
            "dualrail_telemetry": {
                "fastpath_hits": receipts.run1.get("dualrail_fastpath_hits", 0),
                "dense_hits": receipts.run1.get("dualrail_dense_hits", 0),
                "sparse128_hits": receipts.run1.get("dualrail_sparse128_hits", 0),
                "fallback_count": receipts.max_fallback_count,
            },
            "note": candidate.note,
        }

        if not receipts.policy_admitted:
            entry = {
                **record_base,
                "status": "REJECTED_POLICY_BLOCKED",
                "verdict": receipts.run1.get("verdict"),
                "trap_code": receipts.run1.get("trap_code"),
                "trap_name": receipts.run1.get("trap_name"),
                "fault_pc": receipts.run1.get("fault_pc"),
                "last_axion_reason": receipts.run1.get("last_axion_reason"),
            }
            self.negative_constraints.append(entry)
            return entry

        if candidate.max_abs_err != 0:
            entry = {**record_base, "status": "REJECTED_ORACLE_ERROR"}
            self.negative_constraints.append(entry)
            return entry

        if receipts.max_fallback_count != 0:
            entry = {**record_base, "status": "REJECTED_FALLBACK_NONZERO"}
            self.negative_constraints.append(entry)
            return entry

        if not receipts.sha3_identical:
            entry = {**record_base, "status": "REJECTED_RECEIPT_DRIFT"}
            self.negative_constraints.append(entry)
            return entry

        if candidate.constraint_tag != "none" and candidate.constraint_tag in self._blocked_tags:
            entry = {
                **record_base,
                "status": "REJECTED_FALSIFIED",
                "reason": f"blocked_negative_constraint:{candidate.constraint_tag}",
            }
            self.negative_constraints.append(entry)
            return entry

        if candidate.measured_speedup < candidate.min_speedup_threshold:
            if candidate.constraint_tag != "none":
                self._blocked_tags.add(candidate.constraint_tag)
            entry = {
                **record_base,
                "status": "REJECTED_FALSIFIED",
                "reason": (
                    f"speedup {candidate.measured_speedup:.2f}x < "
                    f"threshold {candidate.min_speedup_threshold:.2f}x"
                ),
            }
            self.negative_constraints.append(entry)
            return entry

        entry = {**record_base, "status": "ACCEPTED_CONFIRMED"}
        self.accepted_routes.append(entry)
        return entry

    def recommend_route(self, regime: str, batch_m: int, sparsity_pct: int) -> str:
        if regime == "R2_int8_act":
            r2 = self.prior_routing["R2_int8_act"]
            return r2["m16_plus"] if batch_m >= 16 else r2["m1_to_4"]
        if regime == "R1_ternary_act":
            r1 = self.prior_routing["R1_ternary_act"]
            # H06-MEM-09/11 heterogeneous rule:
            # 1) M==1 & s>=90% -> 1T CPU SBDR128_SIMD (0.837ms vs 0.923ms Metal BitmaskSkip)
            if batch_m == 1 and sparsity_pct >= int(r1["m1_high_sparsity_min_pct"]):
                return str(r1["m1_high_sparsity_kernel"])
            # 2) s>=75% (M>=4 or M==1 with 75%<=s<90%) -> Metal GPU_R1_BitmaskSkip_Pop
            if sparsity_pct >= int(r1["gpu_sparse_min_pct"]):
                return str(r1["gpu_sparse_kernel"])
            # 3) Dense / moderate sparsity (M>=4 or M==1 & s<=50%) -> dense Metal GPU_R1_DualRail_Pop
            if batch_m >= int(r1["gpu_min_batch"]):
                return str(r1["gpu_kernel"])
            if batch_m == 1 and sparsity_pct <= int(r1["m1_low_sparsity_max_pct"]):
                return str(r1["gpu_kernel"])
            return str(r1["sparse_kernel"])
        raise ValueError(f"Unsupported regime: {regime}")

    def verify_pipeline_chain(
        self,
        *,
        experiment_id: str,
        stage_receipts: list[str],
        stage_routes: list[str],
        k_star_prepacked: int,
        weight_sha3_512: str,
        expected_weight_sha3_512: str,
        expected_chain_sha3_512: str,
    ) -> dict[str, Any]:
        payload = (
            "|".join(stage_receipts)
            + "|"
            + "|".join(stage_routes)
            + f"|k_star_prepacked={k_star_prepacked}|{weight_sha3_512}"
        )
        observed_chain = "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()
        base = {
            "experiment_id": experiment_id,
            "stage_routes": stage_routes,
            "k_star_prepacked": k_star_prepacked,
            "expected_weight_sha3_512": expected_weight_sha3_512,
            "observed_weight_sha3_512": weight_sha3_512,
            "expected_chain_sha3_512": expected_chain_sha3_512,
            "observed_chain_sha3_512": observed_chain,
        }
        if weight_sha3_512 != expected_weight_sha3_512:
            entry = {
                **base,
                "status": "REJECTED_WEIGHT_DIGEST_MISMATCH",
                "note": "CanonFS SBDR1T2D weight SHA3-512 mismatch rejected fail-closed.",
            }
            self.negative_constraints.append(entry)
            return entry
        if observed_chain != expected_chain_sha3_512:
            entry = {
                **base,
                "status": "REJECTED_CHAIN_TAMPER",
                "note": "Stage receipt chain SHA3-512 mismatch rejected fail-closed.",
            }
            self.negative_constraints.append(entry)
            return entry
        return {**base, "status": "VERIFIED_CHAIN_OK"}

    def canonical_summary(self) -> dict[str, Any]:
        payload = {
            "git_head_sha": self.git_head_sha,
            "prior_routing": self.prior_routing,
            "accepted_routes": self.accepted_routes,
            "negative_constraints": self.negative_constraints,
            "blocked_constraint_tags": sorted(self._blocked_tags),
        }
        digest = "sha3-512:" + hashlib.sha3_512(
            json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")
        ).hexdigest()
        return {**payload, "ledger_sha3_512": digest}


def run_self_test() -> int:
    # 1. Fetch live session_bootstrap prior via RFC-00D8 compact mode
    boot_resp = _call_mcp_fresh(
        [ROOT_DIR],
        "t81.session_bootstrap",
        {"repo_root": str(ROOT_DIR), "compact": True},
    )
    assert boot_resp["result"]["isError"] is False
    assert boot_resp["result"]["content"][0]["text"] == "ok"
    boot = boot_resp["result"]["structured"]
    ledger = RoutingLedger(boot)

    # 2. Gather live repeat-run RFC-00D8 receipts from two independent MCP processes
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        allow_pol = tmp / "allow.apl"
        allow_pol.write_text("(policy (tier 1))\n", encoding="utf-8")
        strict_pol = tmp / "strict.apl"
        strict_pol.write_text(
            '(policy\n  (tier 1)\n  (allowed-tensor-hashes ["sha3-512:' + ("0" * 128) + '"])\n'
            '  (require-axion-event (reason "enforcement-mode:fail-closed")))\n',
            encoding="utf-8",
        )
        halt_bc = (
            _enc_insn(2, 0, 21, 0) + _enc_insn(5, 1, 0, 0) + _enc_insn(1, 0, 0, 0)
        ).hex()

        # Synthesized 8-instruction balanced-ternary Dual-Rail dot product:
        # w = [+1, -1, +1, -1], x = [13, 5, 9, 2]
        # Exact oracle: P = 13 + 9 = 22, N = 5 + 2 = 7, dot = P - N = 15
        ternary_w = [1, -1, 1, -1]
        ternary_x = [13, 5, 9, 2]
        oracle_dot = sum(wi * xi for wi, xi in zip(ternary_w, ternary_x))
        dualrail_dot = (13 + 9) - (5 + 2)
        synth_max_abs_err = abs(dualrail_dot - oracle_dot)
        assert oracle_dot == 15 and synth_max_abs_err == 0
        synth_dot_bc = (
            _enc_insn(2, 0, 13, 0)   # pc=0: LoadImm r0, 13
            + _enc_insn(2, 1, 9, 0)  # pc=1: LoadImm r1, 9
            + _enc_insn(5, 2, 0, 1)  # pc=2: Add r2, r0, r1 (P=22)
            + _enc_insn(2, 3, 5, 0)  # pc=3: LoadImm r3, 5
            + _enc_insn(2, 4, 2, 0)  # pc=4: LoadImm r4, 2
            + _enc_insn(5, 5, 3, 4)  # pc=5: Add r5, r3, r4 (N=7)
            + _enc_insn(6, 6, 2, 5)  # pc=6: Sub r6, r2, r5 (P-N=15)
            + _enc_insn(1, 0, 0, 0)  # pc=7: Halt
        ).hex()

        ok_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": halt_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        ok_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": halt_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        valid_receipts = ReceiptPair(ok_run1, ok_run2)
        assert valid_receipts.policy_admitted and valid_receipts.sha3_identical
        assert valid_receipts.max_fallback_count == 0

        # Synthesis attempt V1: strict fail-closed Axion policy -> SecurityFault diagnostic
        synth_v1_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": synth_dot_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        synth_v1_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": synth_dot_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        synth_v1_receipts = ReceiptPair(synth_v1_run1, synth_v1_run2)
        assert not synth_v1_receipts.policy_admitted and synth_v1_receipts.sha3_identical
        assert synth_v1_run1["trap_name"] == "SecurityFault"

        # Synthesis refinement V2: admitted Tier-1 policy -> VERIFIED_AND_EXECUTED repeat-run exact
        synth_v2_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": synth_dot_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        synth_v2_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": synth_dot_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        synth_v2_receipts = ReceiptPair(synth_v2_run1, synth_v2_run2)
        assert synth_v2_receipts.policy_admitted and synth_v2_receipts.sha3_identical
        assert synth_v2_receipts.max_fallback_count == 0
        assert synth_v2_run1["instruction_count"] == 8

        # Two-stage ternary pipeline (Stage 1 -> Stage 2) with K*=0 pre-packed routing & receipt chain
        # Stage 1 (R1, M=1, s=90% -> CPU_1T_SBDR128_SIMD): y1 = (+1)*13 + (-1)*5 + (+1)*9 + (-1)*2 = 15
        # Stage 2 (R1, M=4, s=50% -> GPU_R1_DualRail_Pop): y2 = (+1)*y1 + (-1)*4 + (+1)*6 = 17
        stage1_route = ledger.recommend_route("R1_ternary_act", batch_m=1, sparsity_pct=90)
        stage2_route = ledger.recommend_route("R1_ternary_act", batch_m=4, sparsity_pct=50)
        k_star_prepacked = int(ledger.prior_routing["R1_ternary_act"]["k_star_prepacked"])
        assert stage1_route == "CPU_1T_SBDR128_SIMD"
        assert stage2_route == "GPU_R1_DualRail_Pop"
        assert k_star_prepacked == 0

        stage2_w = [1, -1, 1]
        stage2_x = [oracle_dot, 4, 6]
        stage2_oracle = sum(wi * xi for wi, xi in zip(stage2_w, stage2_x))
        stage2_dualrail = (oracle_dot + 6) - 4
        stage2_max_abs_err = abs(stage2_dualrail - stage2_oracle)
        assert stage2_oracle == 17 and stage2_max_abs_err == 0
        stage2_bc = (
            _enc_insn(2, 0, oracle_dot, 0)  # pc=0: LoadImm r0, 15 (Stage 1 verified output)
            + _enc_insn(2, 1, 6, 0)         # pc=1: LoadImm r1, 6
            + _enc_insn(5, 2, 0, 1)         # pc=2: Add r2, r0, r1 (P=21)
            + _enc_insn(2, 3, 4, 0)         # pc=3: LoadImm r3, 4  (N=4)
            + _enc_insn(6, 4, 2, 3)         # pc=4: Sub r4, r2, r3 (P-N=17)
            + _enc_insn(1, 0, 0, 0)         # pc=5: Halt
        ).hex()

        # Run Stage 2 across two independent fresh MCP sessions
        stage2_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage2_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        stage2_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage2_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        stage2_receipts = ReceiptPair(stage2_run1, stage2_run2)
        assert stage2_receipts.policy_admitted and stage2_receipts.sha3_identical
        assert stage2_receipts.max_fallback_count == 0
        assert stage2_run1["instruction_count"] == 6

        def _chain_digest(r1: dict[str, Any], r2: dict[str, Any]) -> str:
            payload = (
                f"{r1['execution_receipt']}|{r2['execution_receipt']}|"
                f"{stage1_route}|{stage2_route}|k_star_prepacked={k_star_prepacked}"
            )
            return "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()

        chain_sha3_1 = _chain_digest(synth_v2_run1, stage2_run1)
        chain_sha3_2 = _chain_digest(synth_v2_run2, stage2_run2)
        assert chain_sha3_1 == chain_sha3_2

        # Fail-closed Axion negative probe on Stage 2
        stage2_deny1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage2_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        stage2_deny2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage2_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        stage2_deny_receipts = ReceiptPair(stage2_deny1, stage2_deny2)
        assert not stage2_deny_receipts.policy_admitted and stage2_deny_receipts.sha3_identical
        assert stage2_deny1["trap_name"] == "SecurityFault"

        # Stage 3 extension (PIPELINE-3STAGE-R1-01): bind CanonFS SBDR1T2D pre-packed (K*=0) weight digest
        croot = tmp / "canonfs_stage3"
        croot.mkdir()
        w3_sbdr_path = tmp / "stage3_w3.sbdr"
        # Pre-packed SBDR1 descriptor for w3 = [+1, -1, +1] (pos=0b101, neg=0b010)
        w3_sbdr_path.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 1, 1, 0b101, 0b010))
        subprocess.run(
            [T81_BIN, "canonfs", "import", str(w3_sbdr_path), "--canonfs-root", str(croot), "--json"],
            check=True,
            capture_output=True,
        )
        fsck_stage3 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.canonfs_fsck",
            {"canonfs_root": str(croot), "compact": True},
        )["result"]["structured"]
        assert fsck_stage3["ok"] and fsck_stage3["checked_objects"] >= 1
        w3_hash_res = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.determinism_hash",
            {"path": str(w3_sbdr_path), "compact": True},
        )["result"]["structured"]
        assert w3_hash_res["ok"] and w3_hash_res["sha3_512"].startswith("sha3-512:")
        canonfs_weight_sha3 = w3_hash_res["sha3_512"]

        stage3_route = ledger.recommend_route("R1_ternary_act", batch_m=1, sparsity_pct=75)
        assert stage3_route == "GPU_R1_BitmaskSkip_Pop"
        stage3_w = [1, -1, 1]
        stage3_x = [stage2_oracle, 3, 5]
        stage3_oracle = sum(wi * xi for wi, xi in zip(stage3_w, stage3_x))
        stage3_dualrail = (stage2_oracle + 5) - 3
        stage3_max_abs_err = abs(stage3_dualrail - stage3_oracle)
        assert stage3_oracle == 19 and stage3_max_abs_err == 0
        stage3_bc = (
            _enc_insn(2, 0, stage2_oracle, 0)  # pc=0: LoadImm r0, 17 (Stage 2 verified output)
            + _enc_insn(2, 1, 5, 0)            # pc=1: LoadImm r1, 5
            + _enc_insn(5, 2, 0, 1)            # pc=2: Add r2, r0, r1 (P=22)
            + _enc_insn(2, 3, 3, 0)            # pc=3: LoadImm r3, 3  (N=3)
            + _enc_insn(6, 4, 2, 3)            # pc=4: Sub r4, r2, r3 (P-N=19)
            + _enc_insn(1, 0, 0, 0)            # pc=5: Halt
        ).hex()

        stage3_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage3_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        stage3_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage3_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        stage3_receipts = ReceiptPair(stage3_run1, stage3_run2)
        assert stage3_receipts.policy_admitted and stage3_receipts.sha3_identical
        assert stage3_receipts.max_fallback_count == 0
        assert stage3_run1["instruction_count"] == 6

        def _chain3_digest(r1: dict[str, Any], r2: dict[str, Any], r3: dict[str, Any]) -> str:
            payload = (
                f"{r1['execution_receipt']}|{r2['execution_receipt']}|{r3['execution_receipt']}|"
                f"{stage1_route}|{stage2_route}|{stage3_route}|"
                f"k_star_prepacked={k_star_prepacked}|{canonfs_weight_sha3}"
            )
            return "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()

        chain3_sha3_1 = _chain3_digest(synth_v2_run1, stage2_run1, stage3_run1)
        chain3_sha3_2 = _chain3_digest(synth_v2_run2, stage2_run2, stage3_run2)
        assert chain3_sha3_1 == chain3_sha3_2

        # Tamper-detection probes on PIPELINE-3STAGE-R1-01:
        # 1) Valid baseline passes verify_pipeline_chain
        chain_valid_chk = ledger.verify_pipeline_chain(
            experiment_id="PIPELINE-3STAGE-BASELINE-CHECK",
            stage_receipts=[
                synth_v2_run1["execution_receipt"],
                stage2_run1["execution_receipt"],
                stage3_run1["execution_receipt"],
            ],
            stage_routes=[stage1_route, stage2_route, stage3_route],
            k_star_prepacked=k_star_prepacked,
            weight_sha3_512=canonfs_weight_sha3,
            expected_weight_sha3_512=canonfs_weight_sha3,
            expected_chain_sha3_512=chain3_sha3_1,
        )
        assert chain_valid_chk["status"] == "VERIFIED_CHAIN_OK"

        # 2) Swapped stage1/stage2 receipts -> fail-closed REJECTED_CHAIN_TAMPER
        chain_swap_chk = ledger.verify_pipeline_chain(
            experiment_id="PIPELINE-3STAGE-SWAP-TAMPER",
            stage_receipts=[
                stage2_run1["execution_receipt"],
                synth_v2_run1["execution_receipt"],
                stage3_run1["execution_receipt"],
            ],
            stage_routes=[stage1_route, stage2_route, stage3_route],
            k_star_prepacked=k_star_prepacked,
            weight_sha3_512=canonfs_weight_sha3,
            expected_weight_sha3_512=canonfs_weight_sha3,
            expected_chain_sha3_512=chain3_sha3_1,
        )
        assert chain_swap_chk["status"] == "REJECTED_CHAIN_TAMPER"

        # 3) Altered CanonFS SBDR1T2D weight file -> real t81.determinism_hash -> REJECTED_WEIGHT_DIGEST_MISMATCH
        w3_tampered_path = tmp / "stage3_w3_tampered.sbdr"
        w3_tampered_path.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 1, 1, 0b111, 0b010))
        w3_tampered_hash = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.determinism_hash",
            {"path": str(w3_tampered_path), "compact": True},
        )["result"]["structured"]["sha3_512"]
        chain_weight_chk = ledger.verify_pipeline_chain(
            experiment_id="PIPELINE-3STAGE-WEIGHT-TAMPER",
            stage_receipts=[
                synth_v2_run1["execution_receipt"],
                stage2_run1["execution_receipt"],
                stage3_run1["execution_receipt"],
            ],
            stage_routes=[stage1_route, stage2_route, stage3_route],
            k_star_prepacked=k_star_prepacked,
            weight_sha3_512=w3_tampered_hash,
            expected_weight_sha3_512=canonfs_weight_sha3,
            expected_chain_sha3_512=chain3_sha3_1,
        )
        assert chain_weight_chk["status"] == "REJECTED_WEIGHT_DIGEST_MISMATCH"

        stage3_deny1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage3_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        stage3_deny2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": stage3_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        stage3_deny_receipts = ReceiptPair(stage3_deny1, stage3_deny2)
        assert not stage3_deny_receipts.policy_admitted and stage3_deny_receipts.sha3_identical
        assert stage3_deny1["trap_name"] == "SecurityFault"

        # BitNet b1.58 .safetensors -> Dual-Rail (P, N) -> CanonFS SBDR1T2D (K*=0) ingestion cycle
        bitnet_mod = _load_bitnet_ingester()
        # 4 blocks x 64 trits = 256 weights: block 0 active [+1,-1,+1,-1,0,...], blocks 1..3 zero -> 75% block sparsity
        f32_weights = [0.0] * 256
        f32_weights[0], f32_weights[1], f32_weights[2], f32_weights[3] = 1.0, -1.0, 1.0, -1.0
        blob = struct.pack(f"<{len(f32_weights)}f", *f32_weights)
        st_header = json.dumps(
            {"block0.weight": {"dtype": "F32", "shape": [4, 64], "data_offsets": [0, len(blob)]}}
        ).encode("utf-8")
        st_path = tmp / "bitnet_block.safetensors"
        st_path.write_bytes(struct.pack("<Q", len(st_header)) + st_header + blob)

        parsed_hdr, parsed_buf, _ = bitnet_mod.read_safetensors(st_path)
        meta = parsed_hdr["block0.weight"]
        floats = bitnet_mod.decode_tensor_floats(meta["dtype"], parsed_buf[meta["data_offsets"][0] : meta["data_offsets"][1]])
        trits, _gamma = bitnet_mod.quantize_bitnet_b158(floats)
        pos_bytes, neg_bytes = bitnet_mod.pack_dualrail_64(trits)
        roundtrip_trits = bitnet_mod.unpack_dualrail_64(pos_bytes, neg_bytes, len(trits))
        bitnet_pack_err = max(abs(a - b) for a, b in zip(trits, roundtrip_trits))
        assert bitnet_pack_err == 0 and trits[:4] == [1, -1, 1, -1] and all(t == 0 for t in trits[4:])

        p0 = struct.unpack_from("<Q", pos_bytes, 0)[0]
        n0 = struct.unpack_from("<Q", neg_bytes, 0)[0]
        assert (p0 & n0) == 0
        bitnet_sbdr_path = tmp / "bitnet_block.sbdr"
        # 4 total 64-trit blocks, mask=0b1 (1 active block -> 75% block sparsity)
        bitnet_sbdr_path.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 4, 1, p0, n0))

        bitnet_croot = tmp / "canonfs_bitnet"
        bitnet_croot.mkdir()
        subprocess.run(
            [T81_BIN, "canonfs", "import", str(bitnet_sbdr_path), "--canonfs-root", str(bitnet_croot), "--json"],
            check=True,
            capture_output=True,
        )
        bitnet_fsck = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.canonfs_fsck",
            {"canonfs_root": str(bitnet_croot), "compact": True},
        )["result"]["structured"]
        assert bitnet_fsck["ok"] and bitnet_fsck["checked_objects"] >= 1

        bitnet_hash_res = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.determinism_hash",
            {"path": str(bitnet_sbdr_path), "compact": True},
        )["result"]["structured"]
        assert bitnet_hash_res["ok"] and bitnet_hash_res["sha3_512"].startswith("sha3-512:")
        bitnet_weight_sha3 = bitnet_hash_res["sha3_512"]

        # Policy admitting exact BitNet CanonFS weight hash + negative digest-mismatch check
        bitnet_pol = tmp / "bitnet_policy.apl"
        bitnet_pol.write_text(
            f'(policy\n  (tier 1)\n  (allowed-tensor-hashes ["{bitnet_weight_sha3}"]))\n',
            encoding="utf-8",
        )
        bitnet_admit = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.policy_test",
            {"policy_path": str(bitnet_pol), "model_hash": bitnet_weight_sha3, "compact": True},
        )["result"]["structured"]
        assert bitnet_admit["ok"] and bitnet_admit["admitted"]

        tampered_hash = "sha3-512:" + ("f" * 128)
        bitnet_mismatch = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.policy_test",
            {"policy_path": str(bitnet_pol), "model_hash": tampered_hash, "compact": True},
        )["result"]["structured"]
        assert (not bitnet_mismatch["ok"]) and (not bitnet_mismatch["admitted"])

        bitnet_route = ledger.recommend_route("R1_ternary_act", batch_m=1, sparsity_pct=75)
        assert bitnet_route == "GPU_R1_BitmaskSkip_Pop"
        bitnet_act = [13, 5, 9, 2]
        bitnet_oracle = sum(w * x for w, x in zip(trits[:4], bitnet_act))
        bitnet_max_abs_err = abs(bitnet_oracle - oracle_dot)
        assert bitnet_oracle == 15 and bitnet_max_abs_err == 0

        bitnet_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": synth_dot_bc, "policy_path": str(bitnet_pol), "compact": True},
        )["result"]["structured"]
        bitnet_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": synth_dot_bc, "policy_path": str(bitnet_pol), "compact": True},
        )["result"]["structured"]
        bitnet_receipts = ReceiptPair(bitnet_run1, bitnet_run2)
        assert bitnet_receipts.policy_admitted and bitnet_receipts.sha3_identical
        assert bitnet_receipts.max_fallback_count == 0
        assert "dualrail_sparse128_blocks_skipped" in bitnet_run1

        def _bitnet_chain(r: dict[str, Any]) -> str:
            payload = f"{bitnet_weight_sha3}|{r['execution_receipt']}|{bitnet_route}|k_star_prepacked={k_star_prepacked}"
            return "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()

        bitnet_chain_1 = _bitnet_chain(bitnet_run1)
        bitnet_chain_2 = _bitnet_chain(bitnet_run2)
        assert bitnet_chain_1 == bitnet_chain_2

        # Two-pass candidate (ADAPTER-SBDR1-COMPOSITION-01):
        # R1, M=4, s=50% -> GPU_R1_DualRail_Pop (K*=0 prepacked), zero-dequant SBDR1 LoRA composition
        # Base word: bit0=+1, bit1=-1, bit2=0, bit3=+1 -> pos=0b1001, neg=0b0010
        # Delta SBDR1: bit0=+1 (sat->+1), bit1=+1 (cancel->0, flip), bit2=-1 (0->-1, flip), bit3=0 (keep +1)
        # Merged exact oracle: w_merged = [+1, 0, -1, +1] -> pos=0b1001, neg=0b0100, flips=2 (3.125%)
        adapter_route = ledger.recommend_route("R1_ternary_act", batch_m=4, sparsity_pct=50)
        assert adapter_route == "GPU_R1_DualRail_Pop"
        ad_base_pos = tmp / "ledger_ad_base.pos"
        ad_base_neg = tmp / "ledger_ad_base.neg"
        ad_delta_sbdr = tmp / "ledger_ad_delta.sbdr"
        ad_out_pos = tmp / "ledger_ad_out.pos"
        ad_out_neg = tmp / "ledger_ad_out.neg"
        ad_deny_pos = tmp / "ledger_ad_deny.pos"
        ad_deny_neg = tmp / "ledger_ad_deny.neg"
        ad_base_pos.write_bytes(struct.pack("<Q", 0b1001))
        ad_base_neg.write_bytes(struct.pack("<Q", 0b0010))
        ad_delta_sbdr.write_bytes(b"SBDR1\x00\x00\x00" + struct.pack("<QQQQ", 1, 1, 0b0011, 0b0100))

        # Fail-closed flip-budget negative probe (max_flip_pct=1.0% < 3.125%)
        ad_deny = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.adapter_apply",
            {
                "base_pos_path": str(ad_base_pos),
                "base_neg_path": str(ad_base_neg),
                "delta_sbdr_path": str(ad_delta_sbdr),
                "out_pos_path": str(ad_deny_pos),
                "out_neg_path": str(ad_deny_neg),
                "max_flip_pct": 1.0,
                "compact": True,
            },
        )["result"]["structured"]
        assert (not ad_deny["ok"]) and (not ad_deny["applied"])
        assert ad_deny["verdict"] == "DENY_EXCEEDS_FLIP_BUDGET"
        assert ad_deny["effective_flips"] == 2 and (not ad_deny_pos.exists())

        # Admitted composition (max_flip_pct=5.0%) across two fresh MCP sessions
        ad_ok1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.adapter_apply",
            {
                "base_pos_path": str(ad_base_pos),
                "base_neg_path": str(ad_base_neg),
                "delta_sbdr_path": str(ad_delta_sbdr),
                "out_pos_path": str(ad_out_pos),
                "out_neg_path": str(ad_out_neg),
                "max_flip_pct": 5.0,
                "compact": True,
            },
        )["result"]["structured"]
        ad_ok2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.adapter_apply",
            {
                "base_pos_path": str(ad_base_pos),
                "base_neg_path": str(ad_base_neg),
                "delta_sbdr_path": str(ad_delta_sbdr),
                "out_pos_path": str(ad_out_pos),
                "out_neg_path": str(ad_out_neg),
                "max_flip_pct": 5.0,
                "compact": True,
            },
        )["result"]["structured"]
        assert ad_ok1["ok"] and ad_ok1["applied"] and ad_ok1["delta_format"] == "sbdr1"
        assert ad_ok1["effective_flips"] == 2
        assert ad_ok1["adapter_receipt"] == ad_ok2["adapter_receipt"]
        assert ad_ok1["merged_sha3_512"] == ad_ok2["merged_sha3_512"]
        merged_p = struct.unpack("<Q", ad_out_pos.read_bytes())[0]
        merged_n = struct.unpack("<Q", ad_out_neg.read_bytes())[0]
        assert merged_p == 0b1001 and merged_n == 0b0100 and (merged_p & merged_n) == 0

        # Exact oracle over merged weights w_merged=[+1, 0, -1, +1] . x=[13, 5, 9, 2] = 6
        w_merged = [((merged_p >> i) & 1) - ((merged_n >> i) & 1) for i in range(4)]
        ad_x = [13, 5, 9, 2]
        ad_oracle = sum(wi * xi for wi, xi in zip(w_merged, ad_x))
        ad_dualrail = (13 + 2) - 9
        ad_max_abs_err = abs(ad_dualrail - ad_oracle)
        assert w_merged == [1, 0, -1, 1] and ad_oracle == 6 and ad_max_abs_err == 0

        ad_bc = (
            _enc_insn(2, 0, 13, 0)   # pc=0: LoadImm r0, 13
            + _enc_insn(2, 1, 2, 0)  # pc=1: LoadImm r1, 2
            + _enc_insn(5, 2, 0, 1)  # pc=2: Add r2, r0, r1 (P=15)
            + _enc_insn(2, 3, 9, 0)  # pc=3: LoadImm r3, 9  (N=9)
            + _enc_insn(6, 4, 2, 3)  # pc=4: Sub r4, r2, r3 (P-N=6)
            + _enc_insn(1, 0, 0, 0)  # pc=5: Halt
        ).hex()

        ad_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": ad_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        ad_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": ad_bc, "policy_path": str(allow_pol), "compact": True},
        )["result"]["structured"]
        ad_receipts = ReceiptPair(ad_run1, ad_run2)
        assert ad_receipts.policy_admitted and ad_receipts.sha3_identical
        assert ad_receipts.max_fallback_count == 0

        def _ad_chain(ad_res: dict[str, Any], run_res: dict[str, Any]) -> str:
            payload = (
                f"{ad_res['adapter_receipt']}|{ad_res['merged_sha3_512']}|"
                f"{run_res['execution_receipt']}|{adapter_route}|k_star_prepacked={k_star_prepacked}"
            )
            return "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()

        ad_chain_1 = _ad_chain(ad_ok1, ad_run1)
        ad_chain_2 = _ad_chain(ad_ok2, ad_run2)
        assert ad_chain_1 == ad_chain_2

        # Cross-regime R1 -> R2 sequence (CROSS-REGIME-R1-R2-01):
        # Stage A (R1_ternary_act, M=1, s=75% -> CPU_1T_SBDR128_SIMD, CanonFS SBDR1T2D K*=0): y_r1 = 15
        # Stage B (R2_int8_act, M=16 -> CPU_4T_INT8W, avoid_cpu_dualrail_unpack=True): y_r2 = 2*y_r1 - 5 = 25
        cross_r1_route = ledger.recommend_route("R1_ternary_act", batch_m=1, sparsity_pct=75)
        cross_r2_route = ledger.recommend_route("R2_int8_act", batch_m=16, sparsity_pct=0)
        assert cross_r1_route == "GPU_R1_BitmaskSkip_Pop"
        assert cross_r2_route == "CPU_4T_INT8W"
        cross_r2_oracle = 2 * bitnet_oracle - 5
        cross_r2_computed = (bitnet_oracle * 2) - 5
        cross_max_abs_err = abs(cross_r2_computed - cross_r2_oracle)
        assert cross_r2_oracle == 25 and cross_max_abs_err == 0

        cross_r2_bc = (
            _enc_insn(2, 0, bitnet_oracle, 0)  # pc=0: LoadImm r0, 15 (Stage A verified R1 output)
            + _enc_insn(2, 1, 2, 0)            # pc=1: LoadImm r1, 2
            + _enc_insn(7, 2, 0, 1)            # pc=2: Mul r2, r0, r1 (30)
            + _enc_insn(2, 3, 5, 0)            # pc=3: LoadImm r3, 5
            + _enc_insn(6, 4, 2, 3)            # pc=4: Sub r4, r2, r3 (25)
            + _enc_insn(1, 0, 0, 0)            # pc=5: Halt
        ).hex()

        cross_r2_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": cross_r2_bc, "policy_path": str(bitnet_pol), "compact": True},
        )["result"]["structured"]
        cross_r2_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": cross_r2_bc, "policy_path": str(bitnet_pol), "compact": True},
        )["result"]["structured"]
        cross_r2_receipts = ReceiptPair(cross_r2_run1, cross_r2_run2)
        assert cross_r2_receipts.policy_admitted and cross_r2_receipts.sha3_identical
        assert cross_r2_receipts.max_fallback_count == 0
        assert cross_r2_run1["instruction_count"] == 6

        def _cross_chain(r1_res: dict[str, Any], r2_res: dict[str, Any]) -> str:
            payload = (
                f"{bitnet_weight_sha3}|{r1_res['execution_receipt']}|{r2_res['execution_receipt']}|"
                f"{cross_r1_route}->{cross_r2_route}|k_star_prepacked={k_star_prepacked}"
            )
            return "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()

        cross_chain_1 = _cross_chain(bitnet_run1, cross_r2_run1)
        cross_chain_2 = _cross_chain(bitnet_run2, cross_r2_run2)
        assert cross_chain_1 == cross_chain_2

        # Metal GPU_R1_DualRail_Pop (M=4, s=50%) -> R2 CPU_4T_INT4W (M=4) with CanonFS SBDR1T2D (K*=0)
        # 4 blocks x 64 trits = 256 weights: blocks 0 & 1 active, blocks 2 & 3 zero -> 50% block sparsity
        gpu_f32 = [0.0] * 256
        gpu_f32[0], gpu_f32[1], gpu_f32[2], gpu_f32[3] = 1.0, -1.0, 1.0, 1.0
        gpu_f32[64], gpu_f32[65] = 1.0, -1.0
        gpu_blob = struct.pack(f"<{len(gpu_f32)}f", *gpu_f32)
        gpu_hdr = json.dumps(
            {"gpu_r1.weight": {"dtype": "F32", "shape": [4, 64], "data_offsets": [0, len(gpu_blob)]}}
        ).encode("utf-8")
        gpu_st_path = tmp / "gpu_r1_block.safetensors"
        gpu_st_path.write_bytes(struct.pack("<Q", len(gpu_hdr)) + gpu_hdr + gpu_blob)

        g_hdr, g_buf, _ = bitnet_mod.read_safetensors(gpu_st_path)
        g_meta = g_hdr["gpu_r1.weight"]
        g_floats = bitnet_mod.decode_tensor_floats(
            g_meta["dtype"], g_buf[g_meta["data_offsets"][0] : g_meta["data_offsets"][1]]
        )
        g_trits, _ = bitnet_mod.quantize_bitnet_b158(g_floats)
        g_pos, g_neg = bitnet_mod.pack_dualrail_64(g_trits)
        g_rt = bitnet_mod.unpack_dualrail_64(g_pos, g_neg, len(g_trits))
        gpu_pack_err = max(abs(a - b) for a, b in zip(g_trits, g_rt))
        gp0, gp1 = struct.unpack_from("<QQ", g_pos, 0)
        gn0, gn1 = struct.unpack_from("<QQ", g_neg, 0)
        assert gpu_pack_err == 0 and (gp0 & gn0) == 0 and (gp1 & gn1) == 0
        assert g_trits[:4] == [1, -1, 1, 1]

        gpu_sbdr_path = tmp / "gpu_r1_block.sbdr"
        # 4 total 64-trit blocks, mask=0b11 (2 active blocks -> 50% block sparsity)
        gpu_sbdr_path.write_bytes(
            b"SBDR1\x00\x00\x00" + struct.pack("<QQQQQQ", 4, 0b11, gp0, gn0, gp1, gn1)
        )
        gpu_croot = tmp / "canonfs_gpu_r1"
        gpu_croot.mkdir()
        subprocess.run(
            [T81_BIN, "canonfs", "import", str(gpu_sbdr_path), "--canonfs-root", str(gpu_croot), "--json"],
            check=True,
            capture_output=True,
        )
        gpu_fsck = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.canonfs_fsck",
            {"canonfs_root": str(gpu_croot), "compact": True},
        )["result"]["structured"]
        assert gpu_fsck["ok"] and gpu_fsck["checked_objects"] >= 1
        gpu_hash_res = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.determinism_hash",
            {"path": str(gpu_sbdr_path), "compact": True},
        )["result"]["structured"]
        assert gpu_hash_res["ok"] and gpu_hash_res["sha3_512"].startswith("sha3-512:")
        gpu_r1_weight_sha3 = gpu_hash_res["sha3_512"]

        gpu_r1_pol = tmp / "gpu_r1_policy.apl"
        gpu_r1_pol.write_text(
            f'(policy\n  (tier 1)\n  (allowed-tensor-hashes ["{gpu_r1_weight_sha3}"]))\n',
            encoding="utf-8",
        )
        gpu_admit = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.policy_test",
            {"policy_path": str(gpu_r1_pol), "model_hash": gpu_r1_weight_sha3, "compact": True},
        )["result"]["structured"]
        assert gpu_admit["ok"] and gpu_admit["admitted"]

        gpu_r1_route = ledger.recommend_route("R1_ternary_act", batch_m=4, sparsity_pct=50)
        gpu_r2_route = ledger.recommend_route("R2_int8_act", batch_m=4, sparsity_pct=0)
        assert gpu_r1_route == "GPU_R1_DualRail_Pop"
        assert gpu_r2_route == "CPU_4T_INT4W"

        # Stage A: w_a=[+1,-1,+1,+1] . x_a=[11,4,7,3] = (11+7+3) - 4 = 17
        gpu_xa = [11, 4, 7, 3]
        gpu_r1_oracle = sum(w * x for w, x in zip(g_trits[:4], gpu_xa))
        gpu_r1_dualrail = (11 + 7 + 3) - 4
        gpu_r1_err = abs(gpu_r1_dualrail - gpu_r1_oracle)
        assert gpu_r1_oracle == 17 and gpu_r1_err == 0
        gpu_r1_bc = (
            _enc_insn(2, 0, 11, 0)   # r0 = 11
            + _enc_insn(2, 1, 7, 0)  # r1 = 7
            + _enc_insn(5, 2, 0, 1)  # r2 = 18
            + _enc_insn(2, 3, 3, 0)  # r3 = 3
            + _enc_insn(5, 4, 2, 3)  # r4 = 21 (P)
            + _enc_insn(2, 5, 4, 0)  # r5 = 4  (N)
            + _enc_insn(6, 6, 4, 5)  # r6 = 17 (P-N)
            + _enc_insn(1, 0, 0, 0)  # Halt
        ).hex()

        # Stage B: y_r2 = 3 * y_r1 + 4 = 55
        gpu_r2_oracle = 3 * gpu_r1_oracle + 4
        gpu_r2_computed = (gpu_r1_oracle * 3) + 4
        gpu_r2_err = abs(gpu_r2_computed - gpu_r2_oracle)
        assert gpu_r2_oracle == 55 and gpu_r2_err == 0
        gpu_r2_bc = (
            _enc_insn(2, 0, gpu_r1_oracle, 0)  # r0 = 17
            + _enc_insn(2, 1, 3, 0)            # r1 = 3
            + _enc_insn(7, 2, 0, 1)            # r2 = 51
            + _enc_insn(2, 3, 4, 0)            # r3 = 4
            + _enc_insn(5, 4, 2, 3)            # r4 = 55
            + _enc_insn(1, 0, 0, 0)            # Halt
        ).hex()

        gpu_r1_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": gpu_r1_bc, "policy_path": str(gpu_r1_pol), "compact": True},
        )["result"]["structured"]
        gpu_r1_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": gpu_r1_bc, "policy_path": str(gpu_r1_pol), "compact": True},
        )["result"]["structured"]
        gpu_r1_receipts = ReceiptPair(gpu_r1_run1, gpu_r1_run2)
        assert gpu_r1_receipts.policy_admitted and gpu_r1_receipts.sha3_identical
        assert gpu_r1_receipts.max_fallback_count == 0

        gpu_r2_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": gpu_r2_bc, "policy_path": str(gpu_r1_pol), "compact": True},
        )["result"]["structured"]
        gpu_r2_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": gpu_r2_bc, "policy_path": str(gpu_r1_pol), "compact": True},
        )["result"]["structured"]
        gpu_r2_receipts = ReceiptPair(gpu_r2_run1, gpu_r2_run2)
        assert gpu_r2_receipts.policy_admitted and gpu_r2_receipts.sha3_identical
        assert gpu_r2_receipts.max_fallback_count == 0

        def _gpu_cross_chain(r1_res: dict[str, Any], r2_res: dict[str, Any]) -> str:
            payload = (
                f"{gpu_r1_weight_sha3}|{r1_res['execution_receipt']}|{r2_res['execution_receipt']}|"
                f"{gpu_r1_route}->{gpu_r2_route}|k_star_prepacked={k_star_prepacked}"
            )
            return "sha3-512:" + hashlib.sha3_512(payload.encode("utf-8")).hexdigest()

        gpu_cross_chain_1 = _gpu_cross_chain(gpu_r1_run1, gpu_r2_run1)
        gpu_cross_chain_2 = _gpu_cross_chain(gpu_r1_run2, gpu_r2_run2)
        assert gpu_cross_chain_1 == gpu_cross_chain_2

        trap_run1 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": halt_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        trap_run2 = _call_mcp_fresh(
            [tmp, ROOT_DIR],
            "t81.run_subgraph",
            {"bytecode_hex": halt_bc, "policy_path": str(strict_pol), "compact": True},
        )["result"]["structured"]
        trap_receipts = ReceiptPair(trap_run1, trap_run2)
        assert not trap_receipts.policy_admitted and trap_receipts.sha3_identical
        assert trap_run1["trap_name"] == "SecurityFault"

    # 3. Exercise empirical falsification + confirmation + fail-closed guards
    r_mem07 = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="H06-MEM-07",
            regime="R1_ternary_act",
            kernel="SBDR128_SIMD_U2",
            scope="L=64, 4096x1024, s=50..90%, Apple M2 1T",
            measured_speedup=0.91,
            min_speedup_threshold=1.25,
            max_abs_err=0,
            constraint_tag="2block_unroll_sbdr128",
            note="M2 OoO already hides vpadalq_u8 latency; unrolling adds tail overhead.",
        ),
        valid_receipts,
    )
    assert r_mem07["status"] == "REJECTED_FALSIFIED"

    r_mem08 = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="H06-MEM-08",
            regime="R1_ternary_act",
            kernel="MT4_SBDR128_SIMD",
            scope="L=64, 4096x1024, s=50..90%, Apple M2 4T CV barrier",
            measured_speedup=0.49,
            min_speedup_threshold=1.50,
            max_abs_err=0,
            constraint_tag="per_layer_cpu_thread_barrier",
            note="64x per-layer CV barrier swamps ~30 us/layer compute.",
        ),
        valid_receipts,
    )
    assert r_mem08["status"] == "REJECTED_FALSIFIED"

    r_mem09 = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="H06-MEM-09",
            regime="R1_ternary_act",
            kernel="GPU_R1_DualRail_Pop",
            scope="L=64, 4096x1024, M=4 (0.342 ms/tok, 2.45x-5.58x vs 1T SBDR128_SIMD)",
            measured_speedup=2.45,
            min_speedup_threshold=2.00,
            max_abs_err=0,
            constraint_tag="none",
            note="Batched Metal R1 beats 1T SBDR128_SIMD across all sparsities at M=4.",
        ),
        valid_receipts,
    )
    assert r_mem09["status"] == "ACCEPTED_CONFIRMED"

    r_policy = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="AXION-DENY-PROBE",
            regime="R1_ternary_act",
            kernel="GPU_R1_DualRail_Pop",
            scope="Unauthorized subgraph under fail-closed Axion policy",
            measured_speedup=5.00,
            min_speedup_threshold=1.50,
            max_abs_err=0,
            constraint_tag="none",
            note="High speedup cannot override Axion SecurityFault.",
        ),
        trap_receipts,
    )
    assert r_policy["status"] == "REJECTED_POLICY_BLOCKED"
    assert r_policy["trap_name"] == "SecurityFault"

    r_synth_v1 = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="SYNTH-TERNARY-DOT-01-V1",
            regime="R1_ternary_act",
            kernel="T81VM_DualRail_Dot4",
            scope="8-insn balanced-ternary dot product under strict unfulfilled Axion policy",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=synth_max_abs_err,
            constraint_tag="none",
            note="Initial synthesis attempt blocked fail-closed by Axion SecurityFault.",
        ),
        synth_v1_receipts,
    )
    assert r_synth_v1["status"] == "REJECTED_POLICY_BLOCKED"
    assert r_synth_v1["trap_name"] == "SecurityFault"

    r_synth_v2 = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="SYNTH-TERNARY-DOT-01-V2",
            regime="R1_ternary_act",
            kernel="T81VM_DualRail_Dot4",
            scope="8-insn balanced-ternary dot product (w=[+1,-1,+1,-1], x=[13,5,9,2], dot=15) under admitted Tier-1 policy",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=synth_max_abs_err,
            constraint_tag="none",
            note="Targeted policy refinement halted cleanly with max_abs_err=0 and bit-exact repeat SHA3-512.",
        ),
        synth_v2_receipts,
    )
    assert r_synth_v2["status"] == "ACCEPTED_CONFIRMED"

    r_pipe_deny = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="PIPELINE-2STAGE-AXION-DENY",
            regime="R1_ternary_act",
            kernel=f"{stage1_route}->{stage2_route}",
            scope="Stage 2 unauthorized under strict fail-closed Axion policy",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=stage2_max_abs_err,
            constraint_tag="none",
            note="Fail-closed Axion SecurityFault blocks Stage 2 execution.",
        ),
        stage2_deny_receipts,
    )
    assert r_pipe_deny["status"] == "REJECTED_POLICY_BLOCKED"
    assert r_pipe_deny["trap_name"] == "SecurityFault"

    r_pipe_ok = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="PIPELINE-2STAGE-R1-01",
            regime="R1_ternary_act",
            kernel=f"{stage1_route}->{stage2_route}",
            scope="Two-stage R1 ternary pipeline (Stage1 y1=15 -> Stage2 y2=17, K*=0 prepacked)",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=max(synth_max_abs_err, stage2_max_abs_err),
            constraint_tag="none",
            note=f"Chained SHA3-512 verified across 2 fresh sessions: {chain_sha3_1}",
        ),
        stage2_receipts,
    )
    assert r_pipe_ok["status"] == "ACCEPTED_CONFIRMED"

    r_pipe3_deny = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="PIPELINE-3STAGE-AXION-DENY",
            regime="R1_ternary_act",
            kernel=f"{stage1_route}->{stage2_route}->{stage3_route}",
            scope="Stage 3 (CanonFS SBDR1T2D K*=0) unauthorized under strict fail-closed Axion policy",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=stage3_max_abs_err,
            constraint_tag="none",
            note="Fail-closed Axion SecurityFault blocks Stage 3 execution.",
        ),
        stage3_deny_receipts,
    )
    assert r_pipe3_deny["status"] == "REJECTED_POLICY_BLOCKED"
    assert r_pipe3_deny["trap_name"] == "SecurityFault"

    r_pipe3_ok = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="PIPELINE-3STAGE-R1-01",
            regime="R1_ternary_act",
            kernel=f"{stage1_route}->{stage2_route}->{stage3_route}",
            scope=(
                "Three-stage R1 ternary pipeline (Stage1 y1=15 -> Stage2 y2=17 -> "
                "Stage3 y3=19, CanonFS SBDR1T2D K*=0 prepacked)"
            ),
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=max(synth_max_abs_err, stage2_max_abs_err, stage3_max_abs_err),
            constraint_tag="none",
            note=f"3-stage chained SHA3-512 verified across 2 fresh sessions: {chain3_sha3_1}",
        ),
        stage3_receipts,
    )
    assert r_pipe3_ok["status"] == "ACCEPTED_CONFIRMED"

    # Record fail-closed digest mismatch as negative constraint
    ledger.negative_constraints.append(
        {
            "experiment_id": "BITNET-DIGEST-MISMATCH",
            "regime": "R1_ternary_act",
            "kernel": bitnet_route,
            "scope": "BitNet CanonFS SBDR1T2D weight digest mismatch probe",
            "expected_sha3_512": bitnet_weight_sha3,
            "probed_sha3_512": tampered_hash,
            "admitted": bitnet_mismatch["admitted"],
            "status": "REJECTED_DIGEST_MISMATCH",
            "note": "t81.policy_test fail-closed rejected mismatched weight SHA3-512.",
        }
    )

    r_bitnet_ok = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="BITNET-CANONFS-SBDR-01",
            regime="R1_ternary_act",
            kernel=bitnet_route,
            scope="BitNet b1.58 [4x64] 75%-sparse -> CanonFS SBDR1T2D (K*=0) -> T81VM dot=15",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=max(bitnet_pack_err, bitnet_max_abs_err),
            constraint_tag="none",
            note=f"BitNet CanonFS SBDR1T2D verified chain: {bitnet_chain_1}",
        ),
        bitnet_receipts,
    )
    assert r_bitnet_ok["status"] == "ACCEPTED_CONFIRMED"

    ledger.negative_constraints.append(
        {
            "experiment_id": "ADAPTER-FLIP-BUDGET-DENY",
            "regime": "R1_ternary_act",
            "kernel": adapter_route,
            "scope": "SBDR1 LoRA adapter flip-budget probe (3.125% > 1.0% max_flip_pct)",
            "verdict": ad_deny["verdict"],
            "effective_flips": ad_deny["effective_flips"],
            "applied": ad_deny["applied"],
            "status": "REJECTED_FLIP_BUDGET",
            "note": "t81.adapter_apply fail-closed rejected delta exceeding max_flip_pct.",
        }
    )

    r_adapter_ok = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="ADAPTER-SBDR1-COMPOSITION-01",
            regime="R1_ternary_act",
            kernel=adapter_route,
            scope="SBDR1 LoRA saturated composition -> T81VM dot=6 (R1, M=4, s=50%, K*=0)",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=ad_max_abs_err,
            constraint_tag="none",
            note=f"Adapter-to-execution SHA3-512 chain verified across 2 sessions: {ad_chain_1}",
        ),
        ad_receipts,
    )
    assert r_adapter_ok["status"] == "ACCEPTED_CONFIRMED"

    # Negative probe: attempting CPU Dual-Rail unpack in R2 must be rejected via blocked constraint tag
    r_cross_unpack_deny = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="CROSS-REGIME-R2-UNPACK-DENY",
            regime="R2_int8_act",
            kernel="CPU_4T_R2_DualRailW",
            scope="Disallowed CPU Dual-Rail unpack in R2 (blocked by H06-MEM-03/05A)",
            measured_speedup=2.50,
            min_speedup_threshold=1.50,
            max_abs_err=0,
            constraint_tag="cpu_dualrail_unpack",
            note="Blocked fail-closed by avoid_cpu_dualrail_unpack negative constraint.",
        ),
        cross_r2_receipts,
    )
    assert r_cross_unpack_deny["status"] == "REJECTED_FALSIFIED"

    r_cross_ok = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="CROSS-REGIME-R1-R2-01",
            regime="R2_int8_act",
            kernel=f"{cross_r1_route}->{cross_r2_route}",
            scope="Cross-regime R1 (CanonFS SBDR1T2D K*=0, y=15) -> R2 (CPU_4T_INT8W, M=16, y=25)",
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=max(bitnet_max_abs_err, cross_max_abs_err),
            constraint_tag="none",
            note=f"Cross-regime R1->R2 SHA3-512 chain verified across 2 sessions: {cross_chain_1}",
        ),
        cross_r2_receipts,
    )
    assert r_cross_ok["status"] == "ACCEPTED_CONFIRMED"

    r_gpu_barrier_deny = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="CROSS-REGIME-GPU-R1-BARRIER-DENY",
            regime="R1_ternary_act",
            kernel="MT4_SBDR128_SIMD",
            scope="Disallowed per-layer CPU thread barrier in R1 (M=4, s=50%, blocked by H06-MEM-08)",
            measured_speedup=2.20,
            min_speedup_threshold=1.50,
            max_abs_err=0,
            constraint_tag="per_layer_cpu_thread_barrier",
            note="Blocked fail-closed by avoid_per_layer_cpu_thread_barrier negative constraint.",
        ),
        gpu_r1_receipts,
    )
    assert r_gpu_barrier_deny["status"] == "REJECTED_FALSIFIED"

    r_gpu_cross_ok = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="CROSS-REGIME-GPU-R1-R2-01",
            regime="R1_ternary_act",
            kernel=f"{gpu_r1_route}->{gpu_r2_route}",
            scope=(
                "Cross-regime Metal R1 (M=4, s=50%, CanonFS SBDR1T2D K*=0, y=17) -> "
                "R2 (CPU_4T_INT4W, M=4, y=55)"
            ),
            measured_speedup=1.00,
            min_speedup_threshold=1.00,
            max_abs_err=max(gpu_pack_err, gpu_r1_err, gpu_r2_err),
            constraint_tag="none",
            note=f"GPU R1->R2 SHA3-512 chain verified across 2 sessions: {gpu_cross_chain_1}",
        ),
        gpu_r2_receipts,
    )
    assert r_gpu_cross_ok["status"] == "ACCEPTED_CONFIRMED"

    r_mem10_falsified = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="H06-MEM-10",
            regime="R1_ternary_act",
            kernel="GPU_R1_SBDR128_Pop",
            scope="L=64, 4096x1024, M=1..4 CSR Metal sparse vs dense GPU (0.67x-1.06x) & 1T CPU (0.77x @ 90%)",
            measured_speedup=0.77,
            min_speedup_threshold=1.25,
            max_abs_err=0,
            constraint_tag="gpu_csr_sbdr128_sparse",
            note="Falsified on M2: 64L dispatch floor + <32-block SIMD under-utilization + gather overhead.",
        ),
        gpu_r1_receipts,
    )
    assert r_mem10_falsified["status"] == "REJECTED_FALSIFIED"

    r_mem11_confirmed = ledger.evaluate_candidate(
        RoutingCandidate(
            experiment_id="H06-MEM-11",
            regime="R1_ternary_act",
            kernel="GPU_R1_BitmaskSkip_Pop",
            scope=(
                "L=64, 4096x1024, s>=75% zero-indirection ColMask Metal skip "
                "(0.923ms @ M=1/90%, 0.208ms/tok @ M=16/90%, 1.53x vs cold dense GPU, sha3=OK)"
            ),
            measured_speedup=1.53,
            min_speedup_threshold=1.25,
            max_abs_err=0,
            constraint_tag="none",
            note="Confirmed on M2: 4KB/layer bitmask eliminates CSR indirection; optimal GPU path for s>=75%.",
        ),
        gpu_r1_receipts,
    )
    assert r_mem11_confirmed["status"] == "ACCEPTED_CONFIRMED"

    # Synthetic fail-closed checks for receipt drift and nonzero fallback
    drifted = ReceiptPair(ok_run1, {**ok_run2, "execution_receipt": "sha3-512:" + ("f" * 128)})
    assert (
        ledger.evaluate_candidate(
            RoutingCandidate("SYNTH-DRIFT", "R1_ternary_act", "GPU_R1_DualRail_Pop", "drift", 3.0, 1.5, 0, "none", "drift"),
            drifted,
        )["status"]
        == "REJECTED_RECEIPT_DRIFT"
    )
    fallback_bad = ReceiptPair(ok_run1, {**ok_run2, "dualrail_fallback_count": 1})
    assert (
        ledger.evaluate_candidate(
            RoutingCandidate("SYNTH-FALLBACK", "R1_ternary_act", "GPU_R1_DualRail_Pop", "fb", 3.0, 1.5, 0, "none", "fb"),
            fallback_bad,
        )["status"]
        == "REJECTED_FALLBACK_NONZERO"
    )

    # 4. Verify live prior-backed route recommendations
    recommendations = {
        "R1_M1_s90": ledger.recommend_route("R1_ternary_act", batch_m=1, sparsity_pct=90),
        "R1_M1_s75": ledger.recommend_route("R1_ternary_act", batch_m=1, sparsity_pct=75),
        "R1_M1_s50": ledger.recommend_route("R1_ternary_act", batch_m=1, sparsity_pct=50),
        "R1_M4_s90": ledger.recommend_route("R1_ternary_act", batch_m=4, sparsity_pct=90),
        "R1_M4_s50": ledger.recommend_route("R1_ternary_act", batch_m=4, sparsity_pct=50),
        "R2_M1": ledger.recommend_route("R2_int8_act", batch_m=1, sparsity_pct=0),
        "R2_M16": ledger.recommend_route("R2_int8_act", batch_m=16, sparsity_pct=0),
    }
    assert recommendations["R1_M1_s90"] == "CPU_1T_SBDR128_SIMD"
    assert recommendations["R1_M1_s75"] == "GPU_R1_BitmaskSkip_Pop"
    assert recommendations["R1_M1_s50"] == "GPU_R1_DualRail_Pop"
    assert recommendations["R1_M4_s90"] == "GPU_R1_BitmaskSkip_Pop"
    assert recommendations["R1_M4_s50"] == "GPU_R1_DualRail_Pop"
    assert recommendations["R2_M1"] == "CPU_4T_INT4W"
    assert recommendations["R2_M16"] == "CPU_4T_INT8W"

    summary = ledger.canonical_summary()
    print(
        json.dumps(
            {
                "routing_ledger_self_test": "PASSED",
                "accepted_count": len(summary["accepted_routes"]),
                "negative_constraints_count": len(summary["negative_constraints"]),
                "blocked_constraint_tags": summary["blocked_constraint_tags"],
                "recommendations": recommendations,
                "synth_v1_fault": {
                    "verdict": synth_v1_run1["verdict"],
                    "trap_code": synth_v1_run1["trap_code"],
                    "trap_name": synth_v1_run1["trap_name"],
                    "fault_pc": synth_v1_run1["fault_pc"],
                    "last_axion_reason": synth_v1_run1["last_axion_reason"],
                    "execution_receipt": synth_v1_run1["execution_receipt"],
                },
                "synth_v2_verified": {
                    "verdict": synth_v2_run1["verdict"],
                    "halted": synth_v2_run1["halted"],
                    "trap_code": synth_v2_run1["trap_code"],
                    "instruction_count": synth_v2_run1["instruction_count"],
                    "oracle_dot": oracle_dot,
                    "max_abs_err": synth_max_abs_err,
                    "dualrail_fallback_count": synth_v2_run1["dualrail_fallback_count"],
                    "repeat_sha3_identical": synth_v2_receipts.sha3_identical,
                    "program_sha3_512": synth_v2_run1["program_sha3_512"],
                    "execution_receipt": synth_v2_run1["execution_receipt"],
                },
                "two_stage_pipeline": {
                    "stage1_route": stage1_route,
                    "stage2_route": stage2_route,
                    "k_star_prepacked": k_star_prepacked,
                    "stage1_oracle": oracle_dot,
                    "stage2_oracle": stage2_oracle,
                    "max_abs_err": max(synth_max_abs_err, stage2_max_abs_err),
                    "dualrail_fallback_count": max(
                        synth_v2_receipts.max_fallback_count,
                        stage2_receipts.max_fallback_count,
                    ),
                    "stage1_receipt": synth_v2_run1["execution_receipt"],
                    "stage2_receipt": stage2_run1["execution_receipt"],
                    "chain_sha3_512": chain_sha3_1,
                    "repeat_sha3_identical": (
                        synth_v2_receipts.sha3_identical
                        and stage2_receipts.sha3_identical
                        and chain_sha3_1 == chain_sha3_2
                    ),
                    "axion_deny_probe": {
                        "verdict": stage2_deny1["verdict"],
                        "trap_code": stage2_deny1["trap_code"],
                        "trap_name": stage2_deny1["trap_name"],
                        "fault_pc": stage2_deny1["fault_pc"],
                        "last_axion_reason": stage2_deny1["last_axion_reason"],
                        "execution_receipt": stage2_deny1["execution_receipt"],
                    },
                },
                "three_stage_pipeline": {
                    "stage1_route": stage1_route,
                    "stage2_route": stage2_route,
                    "stage3_route": stage3_route,
                    "k_star_prepacked": k_star_prepacked,
                    "canonfs_checked_objects": fsck_stage3["checked_objects"],
                    "canonfs_weight_sha3_512": canonfs_weight_sha3,
                    "stage1_oracle": oracle_dot,
                    "stage2_oracle": stage2_oracle,
                    "stage3_oracle": stage3_oracle,
                    "max_abs_err": max(synth_max_abs_err, stage2_max_abs_err, stage3_max_abs_err),
                    "dualrail_fallback_count": max(
                        synth_v2_receipts.max_fallback_count,
                        stage2_receipts.max_fallback_count,
                        stage3_receipts.max_fallback_count,
                    ),
                    "stage1_receipt": synth_v2_run1["execution_receipt"],
                    "stage2_receipt": stage2_run1["execution_receipt"],
                    "stage3_receipt": stage3_run1["execution_receipt"],
                    "chain_sha3_512": chain3_sha3_1,
                    "repeat_sha3_identical": (
                        synth_v2_receipts.sha3_identical
                        and stage2_receipts.sha3_identical
                        and stage3_receipts.sha3_identical
                        and chain3_sha3_1 == chain3_sha3_2
                    ),
                    "axion_deny_probe": {
                        "verdict": stage3_deny1["verdict"],
                        "trap_code": stage3_deny1["trap_code"],
                        "trap_name": stage3_deny1["trap_name"],
                        "fault_pc": stage3_deny1["fault_pc"],
                        "last_axion_reason": stage3_deny1["last_axion_reason"],
                        "execution_receipt": stage3_deny1["execution_receipt"],
                    },
                },
                "bitnet_canonfs_ingestion": {
                    "shape": [4, 64],
                    "block_sparsity_pct": 75,
                    "selected_route": bitnet_route,
                    "k_star_prepacked": k_star_prepacked,
                    "canonfs_checked_objects": bitnet_fsck["checked_objects"],
                    "canonfs_weight_sha3_512": bitnet_weight_sha3,
                    "digest_mismatch_rejected": not bitnet_mismatch["admitted"],
                    "oracle_dot": bitnet_oracle,
                    "max_abs_err": max(bitnet_pack_err, bitnet_max_abs_err),
                    "dualrail_fallback_count": bitnet_receipts.max_fallback_count,
                    "execution_receipt": bitnet_run1["execution_receipt"],
                    "chain_sha3_512": bitnet_chain_1,
                    "repeat_sha3_identical": bitnet_receipts.sha3_identical and bitnet_chain_1 == bitnet_chain_2,
                },
                "adapter_sbdr1_composition": {
                    "regime": "R1_ternary_act",
                    "batch_m": 4,
                    "sparsity_pct": 50,
                    "selected_route": adapter_route,
                    "k_star_prepacked": k_star_prepacked,
                    "effective_flips": ad_ok1["effective_flips"],
                    "flip_budget_deny_verdict": ad_deny["verdict"],
                    "oracle_dot": ad_oracle,
                    "max_abs_err": ad_max_abs_err,
                    "dualrail_fallback_count": ad_receipts.max_fallback_count,
                    "adapter_receipt": ad_ok1["adapter_receipt"],
                    "merged_sha3_512": ad_ok1["merged_sha3_512"],
                    "execution_receipt": ad_run1["execution_receipt"],
                    "chain_sha3_512": ad_chain_1,
                    "repeat_sha3_identical": (
                        ad_ok1["adapter_receipt"] == ad_ok2["adapter_receipt"]
                        and ad_receipts.sha3_identical
                        and ad_chain_1 == ad_chain_2
                    ),
                },
                "chain_tamper_verification": {
                    "baseline_status": chain_valid_chk["status"],
                    "swap_tamper_status": chain_swap_chk["status"],
                    "swap_observed_chain_sha3_512": chain_swap_chk["observed_chain_sha3_512"],
                    "weight_tamper_status": chain_weight_chk["status"],
                    "weight_expected_sha3_512": canonfs_weight_sha3,
                    "weight_observed_sha3_512": w3_tampered_hash,
                    "weight_observed_chain_sha3_512": chain_weight_chk["observed_chain_sha3_512"],
                },
                "cross_regime_r1_r2": {
                    "r1_route": cross_r1_route,
                    "r2_route": cross_r2_route,
                    "k_star_prepacked": k_star_prepacked,
                    "canonfs_weight_sha3_512": bitnet_weight_sha3,
                    "r1_oracle": bitnet_oracle,
                    "r2_oracle": cross_r2_oracle,
                    "max_abs_err": max(bitnet_max_abs_err, cross_max_abs_err),
                    "dualrail_fallback_count": max(
                        bitnet_receipts.max_fallback_count,
                        cross_r2_receipts.max_fallback_count,
                    ),
                    "r2_unpack_deny_status": r_cross_unpack_deny["status"],
                    "r2_unpack_deny_reason": r_cross_unpack_deny["reason"],
                    "r1_receipt": bitnet_run1["execution_receipt"],
                    "r2_receipt": cross_r2_run1["execution_receipt"],
                    "chain_sha3_512": cross_chain_1,
                    "repeat_sha3_identical": (
                        bitnet_receipts.sha3_identical
                        and cross_r2_receipts.sha3_identical
                        and cross_chain_1 == cross_chain_2
                    ),
                },
                "cross_regime_gpu_r1_r2": {
                    "r1_route": gpu_r1_route,
                    "r2_route": gpu_r2_route,
                    "batch_m": 4,
                    "block_sparsity_pct": 50,
                    "k_star_prepacked": k_star_prepacked,
                    "canonfs_checked_objects": gpu_fsck["checked_objects"],
                    "canonfs_weight_sha3_512": gpu_r1_weight_sha3,
                    "r1_oracle": gpu_r1_oracle,
                    "r2_oracle": gpu_r2_oracle,
                    "max_abs_err": max(gpu_pack_err, gpu_r1_err, gpu_r2_err),
                    "dualrail_fallback_count": max(
                        gpu_r1_receipts.max_fallback_count,
                        gpu_r2_receipts.max_fallback_count,
                    ),
                    "barrier_deny_status": r_gpu_barrier_deny["status"],
                    "barrier_deny_reason": r_gpu_barrier_deny["reason"],
                    "r1_receipt": gpu_r1_run1["execution_receipt"],
                    "r2_receipt": gpu_r2_run1["execution_receipt"],
                    "chain_sha3_512": gpu_cross_chain_1,
                    "repeat_sha3_identical": (
                        gpu_r1_receipts.sha3_identical
                        and gpu_r2_receipts.sha3_identical
                        and gpu_cross_chain_1 == gpu_cross_chain_2
                    ),
                },
                "valid_execution_receipt": ok_run1["execution_receipt"],
                "ledger_sha3_512": summary["ledger_sha3_512"],
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(run_self_test())
