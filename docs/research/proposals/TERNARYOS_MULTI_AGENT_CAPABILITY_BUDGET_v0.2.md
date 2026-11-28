# Multi-Agent Capability, Budget & Isolation Specification (TernaryOS v0.2)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Multi-Agent Capability, Budget & Isolation Specification (TernaryOS v0.2)](#multi-agent-capability-budget-&-isolation-specification-ternaryos-v02)
  - [1. Purpose](#1-purpose)
  - [2. Typed Capability Scopes](#2-typed-capability-scopes)
  - [3. Fail-Closed Error, Fallback & Latency Budgets](#3-fail-closed-error-fallback-&-latency-budgets)
  - [4. Multi-Agent Transactional Isolation (`tx_begin` / `tx_commit` / `tx_abort`)](#4-multi-agent-transactional-isolation-`tx_begin`--`tx_commit`--`tx_abort`)
  - [5. Verification Anchor](#5-verification-anchor)

<!-- T81-TOC:END -->


**Status:** Draft research specification (coexistence control-plane contract; does not claim production multi-tenant OS isolation).  
**Dependencies:** [TERNARYOS_ARCHITECTURE_v0.2.md](./TERNARYOS_ARCHITECTURE_v0.2.md), [TERNARYOS_BOOT_TO_RECEIPT_CONTRACT_v0.1.md](./TERNARYOS_BOOT_TO_RECEIPT_CONTRACT_v0.1.md), `docs/governance/PUBLIC_API_LOCK.json` (`v2.2.0`), frozen `TISC v1.9.0`, and the 15-tool native C++ MCP server (`./build/t81 mcp serve`).

---

## 1. Purpose

When multiple autonomous AI agents collaborate inside `t81-foundation` (e.g., a synthesis agent, a verification/oracle agent, and a benchmark/calibration agent), prose coordination is brittle and non-deterministic. This specification defines how agents negotiate **typed capability scopes**, **fail-closed error/fallback/latency budgets**, and **`tx_begin` / `tx_commit` / `tx_abort` workspace isolation** over `RFC-00D8` compact MCP envelopes without encroaching on human governance.

---

## 2. Typed Capability Scopes

Every agent session initializes via `t81.session_bootstrap` (`compact: true`) and declares a bounded capability tuple enforced by `./build/t81 mcp serve`:

| Field | Enforcement Mechanism | Authority Boundary |
| :--- | :--- | :--- |
| **`filesystem_roots`** | `--root <dir>` canonical path jail (blocks `..` traversal and outbound symlinks) | Agent write scope limited strictly to jailed workspace paths |
| **`policy_root`** | Read-only `axion_root.apl` (`policy_root_sha3_512`, `tier >= 1`, `enforcement-mode:fail-closed`) | **Human-only write authority**; agent mutations fail closed (`-1`) |
| **`canonfs_store`** | Content-addressed `SBDR1T2D` (`K* = 0`), validated by `t81.canonfs_fsck` + `t81.policy_test` | Read-only consumption inside compute paths |
| **`regime_routing`** | `R1`/`R2` dispatch (`CPU_1T_SBDR128_SIMD @ 90%`, `GPU_R1_BitmaskSkip_Pop @ 75%`, `GPU_R1_DualRail_Pop`, `CPU_4T_INT4W`/`CPU_4T_INT8W`) | Negative constraints (`16` locked `-1` entries) cannot be overridden |
| **`merge_and_ledger_authority`** | Branch protection + human PR review + `routing_ledger.py` negative locks | **Human-only**; never delegated to autonomous agents |

---

## 3. Fail-Closed Error, Fallback & Latency Budgets

Each multi-agent handoff carries a machine-verifiable budget contract evaluated before any `t81.tx_commit`:

1. **Numerical Exactness Budget:** `max_abs_err == 0` against an independent reference oracle. Any non-zero drift (`SYNTH-ORACLE-ERR`) classifies as `-1` and triggers immediate `t81.tx_abort`.
2. **Zero-Fallback Budget:** `dualrail_fallback_count == 0` on governed `R1` fast paths, with `dualrail_sparse128_blocks_skipped` reported explicitly as a non-negative integer (`0` inert/skip telemetry).
3. **Determinism Budget:** `repeat_sha3_identical == true` across two independent fresh sessions (`execution_receipt`, `boot_receipt_sha3_512`, and when QEMU AArch64 is active, `boot_image_sha3_512` + `serial_transcript_sha3_512`).
4. **Control-Plane Latency & Token Budget:** `RFC-00D8` compact mode (`compact: true`, measured `44.6%` payload reduction, `< 1 ms` `gate_check` p50) prevents multi-agent context bloat.
5. **Hardware-Truth Budget (`0` Inert State):** Unmeasured target architectures (e.g., native `x86_64` AVX2 timings while running on Apple M2 `arm64`) must remain explicitly classified as `0` (Inert / Unmeasured) — never substituted by `arm64` NEON/Metal numbers, Rosetta, or cross-compile syntax checks.

---

## 4. Multi-Agent Transactional Isolation (`tx_begin` / `tx_commit` / `tx_abort`)

- **Disjoint Speculative Workspaces:** Each authoring agent opens its own `t81.tx_begin` session with an isolated `backup_dir` over its owned workspace files; concurrent agents never share mutable target paths without explicit ownership boundaries.
- **Multi-Step Atomic Rollback:** If an early stage passes (`+1`, e.g., `CanonFS` `SBDR1T2D` import + `t81.policy_test` admission) and any downstream stage violates policy (`SecurityFault`, `trap_code: 6`), numerical exactness (`max_abs_err != 0`), zero-fallback (`dualrail_fallback_count != 0`), or repeatability (`repeat_sha3_identical == false`), the orchestrator invokes `t81.tx_abort` across every open `tx_id` in reverse order, restoring exact pre-transaction bytes with zero side effects.
- **Receipt-Gated Commit:** `t81.tx_commit` succeeds (`+1`, `committed: true`) only when the mutated artifact's `SHA3-512` is admitted by `policy_path` and all budget invariants hold.

---

## 5. Verification Anchor

Implemented and verified by [tests/qemu/test_ternaryos_v02_semantics.py](../../../tests/qemu/test_ternaryos_v02_semantics.py) (`test_01` through `test_05`) and [tests/qemu/test_boot_to_receipt_contract.py](../../../tests/qemu/test_boot_to_receipt_contract.py) (`test_01` through `test_04`).
