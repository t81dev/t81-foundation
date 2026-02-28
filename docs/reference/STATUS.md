# T81 Foundation – Current Status

> **Source of Truth:** This document defines the **current operational state** of the project (what is true today). For future plans, see [../roadmaps-plans/ROADMAP.md](../roadmaps-plans/ROADMAP.md). For version history, see [CHANGELOG.md](CHANGELOG.md).

**Last Updated:** February 26, 2026
**Status:** Active development (governance-controlled core with mixed maturity)

## 1. Snapshot

- Core stack is implemented and operational across numerics, language frontend, TISC serialization, VM execution, and Axion/CanonFS enforcement, with maturity varying by subsystem.
- C++23 is the default build language mode; C++20 remains a compatibility lane.
- Current local validation baseline reports full CTest inventory available in this workspace snapshot.
- Determinism guarantees are registry-bounded and must not be overclaimed outside verified surfaces.

## 2. Component Maturity

| Component | Maturity | Spec Version | Test Coverage | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **T81Lang** | Beta | Draft spec / Beta implementation | High | Implementation maturity is Beta under active drift/governance controls. |
| **TISC** | Stable | v1.1.0 | High | Binary serialization and opcode matrix complete. |
| **T81VM** | Beta | v1.1 surface | High | Core execution loop active with continued verification and governance controls. |
| **Axion** | Alpha | Draft surface coverage | High | Policy enforcement active with partial draft-surface coverage. |
| **CanonFS** | Beta | v0.9.x | Medium | Persistent/In-memory drivers implemented; integrity/performance hardening ongoing. |
| **CLI** | Beta | - | Medium | `run`/`compile` stable; `debug`/`trace` improving. |
| **JIT** | Experimental | - | Low | **Threaded Interpreter** only (not machine code JIT). |
| **Cognitive Tiers** | Concept / Experimental | Draft | Mixed | Experimental, non-DCP, non-verified unless promoted through governance. |

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
- Operational status: `../status/SYSTEM_STATUS.md`
- Implementation matrix: `../status/IMPLEMENTATION_MATRIX.md`
- Conformance analysis: `../explanation/ANALYSIS.md`
- Near-term tasks: `../status/TASKS.md`
- CI/gates: `ci.md`
