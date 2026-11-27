# T81 Foundation – Current Status

> **Source of Truth:** This document defines the **current operational state** of the project (what is true today). For future plans, see [../process/roadmaps-plans/ROADMAP.md](../process/roadmaps-plans/ROADMAP.md). For version history, see [CHANGELOG.md](CHANGELOG.md).

**Last Updated:** 2026-11-27
**Status:** Active development (governance-controlled core with mixed maturity; pinned to `v2.2.0` frozen C++ API / `main@5be621553`, post-Wave 5 & backlog cleared)

## 1. Snapshot

- Core stack is implemented and operational across balanced-ternary numerics, T81Lang frontend, frozen TISC `v1.9.0` serialization (`RFC-00D6` accepted), `T81VM` interpreter (with Dual-Rail `(P, N)` NEON/AVX2 `TMatMul`, `SBDR1T2D` caching, Apple Metal `R3` dispatch, and 2B BitNet chunked streaming), fail-closed Axion policy enforcement, CanonFS content-addressed storage (`RFC-00D1` v1 seed), native C++23 RFC 8032/Wycheproof Ed25519 receipt sealing (`RFC-00D4` accepted, `t81 receipt export`), a 15-tool native C++ MCP + `DPE` transactional control plane (`t81 mcp serve`, KEV/advisory fail-closed gating, Three-Plane `ERR_PLANE_BYPASS_DENIED`, TLA+ `DpeBatchCommit` & `PlaneTransitionInvariant`), and the `t81==2.2.0` Python M3 SDK (`31` PyPI files, CPython `3.9–3.14`).
- C++23 is the default build language mode; C++20 remains a compatibility lane.
- Open PR backlog is cleared to **only the 15 governance-held PRs** (`#859`, `#804`–`#814`, `#789`–`#791`); unique historical research reports (`#573`–`#803`) are preserved under `docs/research/archive/`.
- Determinism guarantees are strictly registry-bounded ([`DETERMINISM_SURFACE_REGISTRY.md`](../governance/DETERMINISM_SURFACE_REGISTRY.md)) and must not be overclaimed outside verified DCP surfaces.

## 2. Component Maturity

| Component | Maturity | Spec Version | Test Coverage | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Core Data Types** | Frozen (DCP Verified) | `spec/t81-data-types.md` | High | `PUBLIC_API_LOCK.json` `v2.2.0`; bit-exact canonical encoding and soft-float rounding (`RFC-0030`). |
| **TISC ISA** | Frozen (DCP Verified) | `v1.9.0-Stable` | High | `174` frozen core opcodes (`0–173`), `209` registry entries, `244` named C++ header entries (`0–220`, `224–246`). See `spec/tisc-spec.md §6.3` and draft `RFC-00D6`. |
| **T81VM** | Stable (Interpreter DCP Verified) | `spec/t81vm-spec.md` | High | Interpreter path is DCP Verified; includes NEON/AVX2 Dual-Rail `(P, N)` `TMatMul` with block-zero skip and lazy `SBDR1T2D` `CanonDualRailRhsCache` (`t81_dualrail_sbdr_vm_test`). Broad AI/accelerator determinism remains governed non-DCP. |
| **T81Lang** | Stable (Governed non-DCP) | `spec/t81lang-spec.md` | High | Matches `MATURITY_CANONICAL_STATUS.json`; fixture-based bytecode reproducibility verified via `t81lang_repro_gate.py`. |
| **Axion Governance** | Stable (Bounded, Governed non-DCP) | `spec/axion-kernel.md` | High | Fail-closed `AxionVerdict::Deny` default; immutable `axion_root.apl` enforcement; wired into `AgentInvoke`, `TLoadHash`, `WeightsLoad`, and MCP commit paths. |
| **CanonFS** | Stable (Bounded, Governed non-DCP) | `RFC-0054` (accepted) / `RFC-00D1` (v1 seed) | High | Content-addressed `h1-` hex persistent store + `t81 canonfs import/export` with v1 JSON schemas and negative-path fail-closed export guarantees. Content identity does not establish provenance truth. |
| **Native C++ MCP Server** | Stable (Bounded, Governed non-DCP) | `RFC-00D8` / `TERNARYOS_ARCHITECTURE_v0.2.md` | High | `15` JSON-RPC 2.0 tools, `ScopedJsonParser`, `--root` filesystem jail, `tx_begin`/`tx_commit`/`tx_abort` isolation (`23/23` `ai_usability_eval.py` trials; TLC `14,227,489` states / `0` errors). |
| **Python SDK (`t81`)** | Stable (Bounded) | `v2.2.0` (PyPI) | High | `31` PyPI files (`cp39–cp314`); `27` public symbols in `ffi/python/t81/__init__.py`; tested via `test_installed_sdk.py` across wheels. |
| **CLI** | Stable (Bounded) | `v2.2.0` | High | Grouped `--help` (`24` documented topics verified by `check_cli_docs_parity.py`). |
| **JIT** | Experimental (Non-DCP) | `RFC-0028` / `RFC-0047` | Low | **Threaded Interpreter** only (not machine-code JIT); full equivalence not yet DCP Verified. |
| **TernaryOS / Cognitive Tiers** | Experimental (Non-DCP) | `RFC-00B0..00CE` / `spec/cognitive-tiers.md` | Mixed | Headless AArch64 QEMU enclave (`qemu-verification.yml`) and `T1–T5` cognitive tiers; explicitly excluded from DCP and general-purpose OS claims. |

**Maturity Levels:**
- **Stable:** Production-ready, API frozen, full test coverage.
- **Beta:** Functionally complete, potential API changes, good coverage.
- **Experimental:** Active research/prototyping, no stability guarantees.
- **Stub:** Skeleton code exists but no functional logic.

## 3. Cognitive Tier Status

| Tier | Name | Status | Notes |
| :--- | :--- | :--- | :--- |
| T1 | Symbolic | Experimental implementation | Non-DCP unless promoted through governance. |
| T2 | Reflective | Experimental implementation | Non-DCP unless promoted through governance. |
| T3 | Recursive | Experimental implementation | Non-DCP unless promoted through governance. |
| T4 | Distributed | Experimental implementation | Non-DCP unless promoted through governance. |
| T5 | Infinite | Experimental / partial | Non-DCP unless promoted through governance. |

## 4. Supported Toolchains

The following environments are explicitly supported and verified in CI:

| Platform | OS | Compiler | Status |
| :--- | :--- | :--- | :--- |
| **Linux (x86_64)** | Ubuntu 24.04 | GCC 14, Clang 18 | **Primary Tier** (Determinism Gate) |
| **Linux (ARM64)** | Ubuntu 24.04 | Clang 18 | **Primary Tier** (Determinism Gate) |
| **macOS (ARM64)** | macOS 14 | Clang (Apple) | Supported |
| **macOS (x86_64)** | macOS 13 | GCC 14 | Supported |
| **Windows (x86_64)** | Windows Server 2022 | MSVC (VS 2022) | Supported (Best Effort) |

## 5. Validation Ritual

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Single-threaded safe mode:

```bash
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure -j1
```

## 6. Source-of-Truth Links

- Architecture: `../architecture/OVERVIEW.md`
- Control center: `../status/PROJECT_CONTROL_CENTER.md`
- Implementation matrix: `../status/IMPLEMENTATION_MATRIX.md`
- Conformance analysis: `../explanation/ANALYSIS.md`
- Near-term tasks: `../status/TASKS.md`
- CI/gates: `ci.md`
