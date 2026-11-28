# Minimal "TernaryOS For AI, By AI" Boot-to-Receipt Contract (v0.1 Draft)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Minimal "TernaryOS For AI, By AI" Boot-to-Receipt Contract (v0.1 Draft)](#minimal-"ternaryos-for-ai-by-ai"-boot-to-receipt-contract-v01-draft)
  - [1. Scope & Authority Boundary](#1-scope-&-authority-boundary)
  - [2. Four-Stage Deterministic Boot-to-Receipt Pipeline](#2-four-stage-deterministic-boot-to-receipt-pipeline)
  - [3. Verification & Limitations](#3-verification-&-limitations)

<!-- T81-TOC:END -->


**Status:** Draft specification & control-plane verification contract (minimal headless guest/enclave scope; does not claim general-purpose OS readiness).  
**Frozen Dependencies:** `docs/governance/PUBLIC_API_LOCK.json` (`v2.2.0`), frozen `TISC v1.9.0` (`209` opcodes), `Axion` fail-closed policy engine, `CanonFS` `SBDR1T2D` (`K* = 0`), and `RFC-00D8` MCP receipts.

---

## 1. Scope & Authority Boundary

`ternaryos/` is scoped strictly as a **minimal deterministic headless guest/enclave** for policy-governed AI compute, coexisting with binary hosts (`Linux`/`macOS`/`Windows`).

- **Outside Autonomous Agent Mutation Authority (Human-Governed Only):**
  - Immutable `Axion` root policy (`.apl` tier floors, `allowed-tensor-hashes`, `enforcement-mode:fail-closed`).
  - Frozen `PUBLIC_API_LOCK.json` (`v2.2.0`) and `TISC v1.9.0` opcode contracts.
  - Branch merges, release promotions, and protected/frozen research records.
- **Inside Autonomous Agent Execution Authority (Fail-Closed):**
  - Read-only `CanonFS` `SBDR1T2D` weight loading, verified `TISC` subgraph execution inside `T81VM`, `regime_routing` selection (`R1` / `R2`), and deterministic `SHA3-512` receipt emission.

---

## 2. Four-Stage Deterministic Boot-to-Receipt Pipeline

1. **Stage 1 — Deterministic Guest Boot (`boot_image_sha3_512`):**
   - Headless guest boots with fixed initial state and zero host wall-clock/RNG entropy injected into compute paths, pinning its boot image digest (`boot_image_sha3_512`).
2. **Stage 2 — Immutable `Axion` Policy Root & `CanonFS` `SBDR1T2D` Integrity Gate:**
   - Hash and parse the read-only `Axion` policy root (`policy_root_sha3_512`).
   - Verify `CanonFS` store integrity (`t81.canonfs_fsck`, `ok == true`, `checked_objects >= 1`) and content-addressed `SBDR1T2D` weight digest (`t81.determinism_hash` + `t81.policy_test`, `admitted == true`). Any mismatch or policy violation halts fail-closed **before** weight execution or side effects.
3. **Stage 3 — Governed `TISC v1.9.0` / `T81VM` Execution:**
   - Execute verified `TISC` bytecode under `Axion` hooks (`t81.run_subgraph`, `compact: true`) with explicit `regime_routing` (`CPU_1T_SBDR128_SIMD` for `R1, M == 1, s >= 90%`; `GPU_R1_BitmaskSkip_Pop` for `R1, s >= 75%`; `GPU_R1_DualRail_Pop` otherwise; `CPU_4T_INT4W`/`CPU_4T_INT8W` for `R2`).
   - Enforce `max_abs_err == 0`, `dualrail_fallback_count == 0`, and explicit `dualrail_sparse128_blocks_skipped` telemetry.
4. **Stage 4 — Canonical `SHA3-512` Boot-to-Receipt Emission:**
   - Emit `boot_receipt_sha3_512 = SHA3-512(boot_image_sha3_512 | policy_root_sha3_512 | canonfs_weight_sha3_512 | execution_receipt | route | k_star_prepacked=0)`.
   - Must satisfy `repeat_sha3_identical == true` across two independent fresh sessions.

---

## 3. Verification & Limitations

- **Implemented Control-Plane Verification:** `TERNARYOS-BOOT-RECEIPT-01` in `examples/research/agent-receipt-sidecar/routing_ledger.py` (enforced by `scripts/ci/check_agent_synthesis_gate.py`) verifies Stages 2–4 end-to-end against live `./build/t81 mcp serve` processes, with a pinned deterministic `boot_image_sha3_512` fixture.
- **Explicit Limitation:** Full bare-metal / QEMU AArch64 kernel boot-to-receipt integration depends on `ternaryos/` QEMU serial plumbing (tracked separately in open PR `#856`) and is not claimed as completed by this control-plane verification.
