# Freeze Enforcement & Versioning Discipline

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Freeze Enforcement & Versioning Discipline](#freeze-enforcement-&-versioning-discipline)
  - [1. Freeze Boundaries](#1-freeze-boundaries)
    - [A. Data Types](#a-data-types)
    - [B. TISC ISA](#b-tisc-isa)
    - [C. Determinism Guarantees](#c-determinism-guarantees)
    - [D. Public C++ API Surface](#d-public-c++-api-surface)
  - [2. Versioning Rules](#2-versioning-rules)
  - [3. Break Procedure](#3-break-procedure)
  - [4. Determinism Breach Protocol](#4-determinism-breach-protocol)

<!-- T81-TOC:END -->


**Effective Date:** 2024-10-27
**Status:** **Active**
**Authority:** High (Supersedes `CONTRIBUTING.md` for core subsystems)

This document defines the strict enforcement rules for "Frozen" subsystems within the T81 Foundation codebase. It translates the high-level governance model into concrete versioning rules and break procedures.

**Related Governance:**

*   `spec/supplemental/deterministic-core-profile-v1.md` (What is frozen)
*   `docs/governance/SPEC_DRIFT_POLICY.md` (Code/Spec divergence)
*   `docs/status/VERIFIED_SURFACE_AUDIT.md` (Verification status)

---

## 1. Freeze Boundaries

The following subsystems are designated as **Frozen**. Modifications to these areas are strictly regulated to preserve the "Sovereign" guarantees of the v1.0 release.

For a concise summary of frozen vs. experimental components, see the [Deterministic Core Profile](../../spec/supplemental/deterministic-core-profile-v1.md).

### A. Data Types
*   **Scope:** `core/types/`, `spec/t81-data-types.md`
*   **Frozen Surface:**
    *   Binary representation of `Trit`, `Tryte`, `T81Float`, `T81Fraction`.
    *   Canonicalization rules for all composite types.
    *   Arithmetic semantics (overflow behavior, rounding modes).
*   **Forbidden:** Changing the bit-layout of any serialized type.
*   **Allowed:** Performance optimizations that do not alter binary output.

### B. TISC ISA
*   **Scope:** `core/isa/`, `core/vm/`, `spec/tisc-spec.md`
*   **Frozen Surface:**
    *   Opcode values and encoding.
    *   Instruction semantics (state transitions).
    *   Fault conditions and error codes.
*   **Forbidden:** Renumbering opcodes, changing operand ordering, altering fault behavior.
*   **Allowed:** Adding *new* opcodes (via extension RFC) if they do not conflict with existing encoding space. New opcodes must not alter decoding or semantics of any existing opcode.

### C. Determinism Guarantees
*   **Scope:** Execution traces, Floating-point math (`dmath`), Serialization.
*   **Frozen Surface:**
    *   Bit-exact output across supported architectures (x86-64, ARM64).
    *   Cross-platform consistency of `T81Float` operations.
    *   "Supported architectures” are those listed in the root README under Supported Platforms for the current major version.
*   **Forbidden:** Any change that causes a regression in cross-platform reproducibility.

### D. Public C++ API Surface
*   **Scope:** `include/t81/`
*   **Clarification:** “Public C++ API surface” refers specifically to headers under `include/t81/**` only.
*   **Frozen Surface:**
    *   Class/Struct memory layouts.
    *   Function signatures and symbol names.
    *   Header file organization.
*   **Forbidden:** Breaking source or binary compatibility without a Major version bump.

---

## 2. Versioning Rules

We adhere to strict Semantic Versioning (SemVer 2.0.0). The following table maps change types to required version bumps.

| Change Type | Version Bump Required | Notes |
| :--- | :--- | :--- |
| **ISA opcode semantic change** | **MAJOR** | Changing what `ADD` does breaks all existing bytecode. |
| **New opcode (backwards compatible)** | **MINOR** | Adding `NEW_OP` allows old code to run, but new code won't run on old VMs. |
| **Determinism surface regression** | **MAJOR** | Breaking reproducibility is a breaking change for the verification contract. |
| **Public API removal / breaking change** | **MAJOR** | Removing a function or changing its signature. |
| **Public API addition (compatible)** | **MINOR** | Adding a new function or class. |
| **Performance-only change** | **PATCH** | Optimization that preserves exact behavior. |
| **Doc-only change** | **None** | Updates to documentation or comments. |
| **Internal refactor** | **PATCH** | Changes to `.cpp` files that don't affect public headers or behavior. |

---

## 3. Break Procedure

If a freeze boundary **must** be broken (e.g., to fix a critical design flaw or security vulnerability), the following procedure is mandatory.

1.  **Draft Breaking Change RFC**:
    *   Explain *why* the break is unavoidable.
    *   Analyze the impact on the ecosystem (VMs, compilers, data).
    *   Propose a mitigation strategy.
2.  **Update Specification**:
    *   Modify the relevant file in `/spec/`.
    *   Update the version number in the spec header.
3.  **Update Implementation Matrix**:
    *   Update `docs/status/IMPLEMENTATION_MATRIX.md` to reflect the new spec version.
4.  **Add Migration Notes**:
    *   Create a migration guide in `docs/migration/` (e.g., `v1_to_v2_migration.md`).
5.  **Bump MAJOR Version**:
    *   Update the project version in `CMakeLists.txt` and `package.json` (if applicable).
6.  **Update Changelog**:
    *   Add a distinct "BREAKING CHANGES" section to the changelog.
7.  **Deprecate Previous Major**:
    *   Explicitly mark the previous major version as "Maintenance Mode" or "Deprecated".

---

## 4. Determinism Breach Protocol

Breach classification follows RFC-0043 §6 (Deterministic Conformance Validation Framework):

| Class | Trigger | Merge Impact |
| :---- | :------ | :----------- |
| **Hard Divergence** | Final bytes, trap class, trace hash, or canonical serialization differ across platforms or backends | Merge-blocked — Critical Defect |
| **Soft Divergence** | Non-DCP diagnostic strings or non-governed metadata differ | Not blocking unless it crosses a governed boundary |
| **UB Exposure** | Backend depends on undefined behavior; memory layout or ABI mismatch changes results | Treated as Hard Divergence on a verified surface |

If a change is detected that breaks determinism on a verified surface (e.g., CI failure in `repro-ledger.yml`, `t81lang_repro_gate.py`, or any backend equivalence test):

1.  **Immediate Stop**: The PR is blocked. **Do not merge.**
2.  **Classification**: Classify the failure using the RFC-0043 §6 breach taxonomy above.
3.  **Root Cause Analysis**:
    *   Identify the source of nondeterminism (e.g., uninitialized memory, hash map iteration order, floating-point compiler flags, UB-dependent backend).
    *   Produce a minimal reproduction case.
4.  **Postmortem**:
    *   Document the cause and the fix.
    *   Explain why the existing gates failed to catch it (if applicable).
5.  **Regression Test**:
    *   Add a specific regression test to the suite to prevent recurrence.
    *   Only *then* can the fix be merged.
