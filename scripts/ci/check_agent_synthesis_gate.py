#!/usr/bin/env python3
"""Lightweight Agent Synthesis Gate ("By AI" anti-bloat & invariant check).

Catches common AI-authored regressions in <2s without adding dependencies:
1. Public API lock & TISC freeze integrity.
2. Orphan C++ sources under core runtime dirs not wired into CMakeLists.txt.
3. 1:1 parity between MCP tool schemas, dispatch handlers, and test_native_mcp.py.
4. Dual-Rail TMatMul SIMD path & SBDR1T2D VM test presence (anti-silent-fallback).
5. Repeat-run SHA3-512 determinism on `t81 mcp serve` (when build/t81 exists).
6. Back-to-back receipt-backed routing ledger (`routing_ledger.py`) determinism & invariant checks.
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
CORE_SRC_DIRS = ("vm", "fs", "kernel/axion", "tools/cli/core")

# Shrink-only baseline of pre-existing unlinked legacy .cpp files (ratcheted 30 -> 17 -> 11 on 2026-11-23).
# Remaining 11 entries are invoked by vm/run_*_tests.sh, tools/experimental/test_completeness_checker.cpp, or RFC-00D1 docs.
# Any NEW orphan .cpp file outside this set fails the gate immediately.
KNOWN_LEGACY_ORPHAN_CPP = frozenset(
    {
        "vm/advanced_ai_integration.cpp",
        "vm/csi_integration.cpp",
        "vm/test_advanced_ai_syntax.cpp",
        "vm/test_csi_end_to_end.cpp",
        "vm/test_csi_integration.cpp",
        "vm/test_csi_syntax.cpp",
        "vm/test_csi_vm_dispatch.cpp",
        "fs/canonfs_enhanced_errors.cpp",
        "fs/canonical_optimization_decision.cpp",
        "fs/deep_learning_optimizer.cpp",
        "fs/ternary_optimization.cpp",
    }
)

# Zero untested MCP tools enforced (15/15 live coverage in test_native_mcp.py after PR #847 + RFC-00D8).
KNOWN_UNTESTED_MCP_TOOLS: frozenset[str] = frozenset()


def check_subscript(rel_path: str, errors: list[str]) -> None:
    res = subprocess.run(
        [sys.executable, str(REPO / rel_path)],
        cwd=REPO,
        capture_output=True,
        text=True,
    )
    if res.returncode != 0:
        errors.append(f"{rel_path} failed:\n{(res.stdout + res.stderr).strip()}")


def check_orphan_cpp(errors: list[str]) -> None:
    cmake_text = "\n".join(
        p.read_text(encoding="utf-8")
        for p in [REPO / "CMakeLists.txt", *sorted((REPO / "src").rglob("CMakeLists.txt"))]
    )
    for legacy in sorted(KNOWN_LEGACY_ORPHAN_CPP):
        if not (REPO / legacy).exists():
            errors.append(
                f"stale KNOWN_LEGACY_ORPHAN_CPP entry (file deleted; shrink the baseline!): {legacy}"
            )
    for rel_dir in CORE_SRC_DIRS:
        base = REPO / rel_dir
        if not base.exists():
            continue
        for cpp in sorted(base.glob("*.cpp")):
            rel = cpp.relative_to(REPO).as_posix()
            if cpp.name not in cmake_text and rel not in KNOWN_LEGACY_ORPHAN_CPP:
                errors.append(f"new orphan C++ source not referenced in CMakeLists.txt: {rel}")


def check_benchmark_rigor(errors: list[str]) -> None:
    bm = (REPO / "benchmarks/BM_DualRailMemoryBound.cpp").read_text(encoding="utf-8")
    for marker, label in (
        ("SkipWithError", "exact oracle mismatch guard"),
        ("weight_MB", "working-set footprint counter"),
        ("BM_MemBound_INT8_Dense", "SIMD INT8 dense control"),
        ("BM_MemBound_INT4_Dense", "packed INT4 dense control"),
    ):
        if marker not in bm:
            errors.append(f"benchmarks/BM_DualRailMemoryBound.cpp missing {label} ({marker})")


def check_mcp_surface_parity(errors: list[str]) -> None:
    mcp_cpp = (REPO / "tools/cli/core/mcp_server.cpp").read_text(encoding="utf-8")
    mcp_test = (
        REPO / "examples/research/agent-receipt-sidecar/test_native_mcp.py"
    ).read_text(encoding="utf-8")

    schema_block = mcp_cpp.split("build_tools_list_json()", 1)[1].split(
        "detect_host_capabilities_json()", 1
    )[0]
    schema_tools = set(re.findall(r'\\"name\\":\\"(t81\.[a-z0-9_]+)\\"', schema_block))
    handler_tools = set(re.findall(r'name == "(t81\.[a-z0-9_]+)"', mcp_cpp))
    tested_tools = set(re.findall(r'"(t81\.[a-z0-9_]+)"', mcp_test))

    if not schema_tools:
        errors.append("failed to parse any MCP tool names from build_tools_list_json()")
        return
    if schema_tools != handler_tools:
        errors.append(
            f"MCP schema/handler mismatch: "
            f"schema_only={sorted(schema_tools - handler_tools)}, "
            f"handler_only={sorted(handler_tools - schema_tools)}"
        )
    stale_untested = sorted(KNOWN_UNTESTED_MCP_TOOLS - (schema_tools - tested_tools))
    if stale_untested:
        errors.append(
            f"stale KNOWN_UNTESTED_MCP_TOOLS entries (now tested or removed; shrink the baseline!): "
            f"{stale_untested}"
        )
    untested = sorted((schema_tools - tested_tools) - KNOWN_UNTESTED_MCP_TOOLS)
    if untested:
        errors.append(f"new MCP tools missing from test_native_mcp.py: {untested}")


def check_dualrail_non_bypass(errors: list[str]) -> None:
    th = (REPO / "vm/tensor_helpers.cpp").read_text(encoding="utf-8")
    for marker, label in (
        ("try_dualrail_ternary_matmul_2d", "Dual-Rail TMatMul fast path"),
        ("try_decode_sbdr1t2d_and_cache", "SBDR1T2D CanonFS direct cache loader"),
        ("dualrail_telemetry", "Dual-Rail fastpath/fallback telemetry"),
        ("build_sparse128_if_beneficial", "regime-aware 128-trit SIMD sparse routing"),
        ("__ARM_NEON", "ARM64 NEON Dual-Rail kernel"),
        ("__AVX2__", "x86_64 AVX2 Dual-Rail kernel"),
    ):
        if marker not in th:
            errors.append(f"vm/tensor_helpers.cpp missing {label} ({marker})")
    sbdr_test = (REPO / "tests/cpp/dualrail_sbdr_vm_test.cpp").read_text(encoding="utf-8")
    if "fallback_count == 0" not in sbdr_test:
        errors.append("tests/cpp/dualrail_sbdr_vm_test.cpp missing zero-fallback assertion")
    if "sparse128_hits == 1" not in sbdr_test:
        errors.append("tests/cpp/dualrail_sbdr_vm_test.cpp missing sparse128 regime assertion")


def check_mcp_repeat_determinism(errors: list[str]) -> None:
    t81_bin = REPO / "build/t81"
    if not t81_bin.is_file():
        return
    req = json.dumps(
        {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "tools/call",
            "params": {
                "name": "t81.determinism_hash",
                "arguments": {"path": str(REPO / "docs/governance/PUBLIC_API_LOCK.json")},
            },
        }
    )
    digests = []
    for _ in range(2):
        res = subprocess.run(
            [str(t81_bin), "mcp", "serve", "--root", str(REPO)],
            input=req + "\n",
            capture_output=True,
            text=True,
            timeout=5,
        )
        if res.returncode != 0 or not res.stdout.strip():
            errors.append(f"t81 mcp serve determinism probe failed (rc={res.returncode})")
            return
        payload = json.loads(res.stdout.splitlines()[0])["result"]["structured"]
        if not payload.get("ok") or not str(payload.get("sha3_512", "")).startswith("sha3-512:"):
            errors.append(f"unexpected determinism_hash response: {payload}")
            return
        digests.append(payload["sha3_512"])
    if digests[0] != digests[1]:
        errors.append(f"non-deterministic SHA3-512 across identical runs: {digests}")


def check_routing_ledger_determinism(errors: list[str]) -> None:
    t81 = REPO / "build" / "t81"
    ledger_script = REPO / "examples/research/agent-receipt-sidecar/routing_ledger.py"
    if not ledger_script.exists():
        errors.append(f"missing routing ledger script: {ledger_script}")
        return
    if not t81.exists():
        return

    verified_sections = (
        "synth_v2_verified",
        "two_stage_pipeline",
        "three_stage_pipeline",
        "bitnet_canonfs_ingestion",
        "adapter_sbdr1_composition",
        "cross_regime_r1_r2",
        "cross_regime_gpu_r1_r2",
    )
    ledger_digests: list[str] = []
    for run_idx in range(1, 3):
        try:
            proc = subprocess.run(
                [sys.executable, str(ledger_script)],
                cwd=REPO,
                capture_output=True,
                text=True,
                timeout=30,
            )
        except subprocess.TimeoutExpired:
            errors.append(f"routing_ledger.py run {run_idx} timed out after 30s")
            return
        if proc.returncode != 0:
            detail = (proc.stderr or proc.stdout or "").strip()
            errors.append(f"routing_ledger.py run {run_idx} exited {proc.returncode}: {detail}")
            return
        raw = proc.stdout.strip()
        if not raw:
            errors.append(f"routing_ledger.py run {run_idx} produced empty stdout")
            return
        try:
            payload = json.loads(raw)
        except json.JSONDecodeError as exc:
            errors.append(f"routing_ledger.py run {run_idx} returned invalid JSON: {exc}")
            return
        if payload.get("routing_ledger_self_test") != "PASSED":
            errors.append(
                f"routing_ledger.py run {run_idx} self-test did not pass: "
                f"{payload.get('routing_ledger_self_test')!r}"
            )
            return
        accepted = int(payload.get("accepted_count", -1))
        neg_count = int(payload.get("negative_constraints_count", -1))
        if accepted < 8 or neg_count < 14:
            errors.append(
                f"routing_ledger.py run {run_idx} counts regressed: "
                f"accepted_count={accepted} (expected >=8), "
                f"negative_constraints_count={neg_count} (expected >=14)"
            )
            return
        tamper = payload.get("chain_tamper_verification")
        if (
            not isinstance(tamper, dict)
            or tamper.get("baseline_status") != "VERIFIED_CHAIN_OK"
            or tamper.get("swap_tamper_status") != "REJECTED_CHAIN_TAMPER"
            or tamper.get("weight_tamper_status") != "REJECTED_WEIGHT_DIGEST_MISMATCH"
        ):
            errors.append(
                f"routing_ledger.py run {run_idx} chain_tamper_verification regressed: {tamper!r}"
            )
            return
        digest = payload.get("ledger_sha3_512", "")
        if not isinstance(digest, str) or not digest.startswith("sha3-512:") or len(digest) <= 16:
            errors.append(f"routing_ledger.py run {run_idx} missing valid ledger_sha3_512: {digest!r}")
            return
        for sec in verified_sections:
            sec_obj = payload.get(sec)
            if not isinstance(sec_obj, dict):
                errors.append(f"routing_ledger.py run {run_idx} missing section {sec!r}")
                return
            if sec_obj.get("max_abs_err") != 0:
                errors.append(
                    f"routing_ledger.py run {run_idx} section {sec} max_abs_err="
                    f"{sec_obj.get('max_abs_err')!r} (expected 0)"
                )
                return
            if sec_obj.get("dualrail_fallback_count") != 0:
                errors.append(
                    f"routing_ledger.py run {run_idx} section {sec} dualrail_fallback_count="
                    f"{sec_obj.get('dualrail_fallback_count')!r} (expected 0)"
                )
                return
            if sec_obj.get("repeat_sha3_identical") is not True:
                errors.append(
                    f"routing_ledger.py run {run_idx} section {sec} repeat_sha3_identical="
                    f"{sec_obj.get('repeat_sha3_identical')!r} (expected True)"
                )
                return
        ledger_digests.append(digest)

    if ledger_digests[0] != ledger_digests[1]:
        errors.append(
            f"non-deterministic routing_ledger.py ledger_sha3_512 across back-to-back runs: "
            f"{ledger_digests}"
        )


def main() -> int:
    errors: list[str] = []
    check_subscript("scripts/governance/check_public_api_semver.py", errors)
    check_subscript("scripts/ci/check_tisc_freeze_integrity.py", errors)
    check_orphan_cpp(errors)
    check_benchmark_rigor(errors)
    check_mcp_surface_parity(errors)
    check_dualrail_non_bypass(errors)
    check_mcp_repeat_determinism(errors)
    check_routing_ledger_determinism(errors)

    if errors:
        print("agent synthesis gate: FAILED", file=sys.stderr)
        for i, err in enumerate(errors, 1):
            print(f"{i}. {err}", file=sys.stderr)
        return 1
    print("agent synthesis gate: PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
