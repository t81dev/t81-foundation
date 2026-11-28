# T81 SCG-04 Decision Memorandum — Normative Semantic Reconciliation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 SCG-04 Decision Memorandum — Normative Semantic Reconciliation](#t81-scg-04-decision-memorandum-—-normative-semantic-reconciliation)
  - [1. Executive Summary & Governance Authority](#1-executive-summary-&-governance-authority)
  - [2. Reference to the SCG-01 Contradiction Register](#2-reference-to-the-scg-01-contradiction-register)
  - [3. Minimum Normative Decisions Required](#3-minimum-normative-decisions-required)
    - [Decision D1: T81Int Arithmetic Error Propagation Surface](#decision-d1-t81int-arithmetic-error-propagation-surface)
    - [Decision D2: T81Float Binary Conversion Contract & Rounding Policy](#decision-d2-t81float-binary-conversion-contract-&-rounding-policy)
    - [Decision D3: TISC Fault & State-Transition Atomic Boundaries](#decision-d3-tisc-fault-&-state-transition-atomic-boundaries)
    - [Decision D4: Result and T81Result Surface Standardization](#decision-d4-result-and-t81result-surface-standardization)
  - [4. Comprehensive SCG-04 Decision Table](#4-comprehensive-scg-04-decision-table)
  - [5. Architectural Determinism vs. Radix-Specific Advantage](#5-architectural-determinism-vs-radix-specific-advantage)
  - [6. Compatibility Consequences & Maintainer Action Items](#6-compatibility-consequences-&-maintainer-action-items)
    - [6.1 Compatibility Impact Matrix](#61-compatibility-impact-matrix)
    - [6.2 Required Maintainer Authorizations](#62-required-maintainer-authorizations)
  - [7. Disposition and Next Steps](#7-disposition-and-next-steps)

<!-- T81-TOC:END -->


**Document:** `docs/research/continuity/SCG04_PREPARATION_DECISION_MEMORANDUM.md`
**Date:** 2026-11-08
**Task Anchor:** SCG-04 Preparation (Read-Only Specification Decision Instrument)
**Authoritative Baseline:** Current `origin/main` pinned to `2c3501e2807bb893076c5037231c3126b7f2bc72`
**Status:** DRAFT / PROPOSED DECISION INSTRUMENT — NO NORMATIVE OR CODE CHANGES ADOPTED
**Disposition:** READY FOR INDEPENDENT REVIEW

---

## 1. Executive Summary & Governance Authority

Following the successful correctness repairs in SCG-03 (resolving C05 intermediate multiplication truncation and C11 test verdict deactivation under NDEBUG, merged in PR #729), this memorandum prepares the next specification correction under the Scientific Continuity and Governance (SCG) track: **SCG-04 Normative Semantic Reconciliation**.

This document is strictly **read-only and non-normative**. In accordance with T81 governance directives:
- It does not rewrite arithmetic algorithms.
- It does not alter frozen specifications.
- It does not expand or modify the TISC instruction set.
- It does not execute or record performance benchmarks.
- It introduces zero code modifications or automatic merges.

Its sole purpose is to synthesize the outstanding contradictions from the **SCG-01 contradiction register** (specifically contradictions C01, C02, C06, C07, C08, C09, and C10) into an actionable, minimal decision instrument for project maintainers.

---

## 2. Reference to the SCG-01 Contradiction Register

Rather than creating a redundant register, SCG-04 explicitly references the unresolved contradictions established in SCG-01 and analyzed in companion audit instruments (`spec/rfcs/RFC-00D5-checked-floating-point-conversion.md`, `spec/rfcs/RFC-00D6-versioned-tisc-state-transition-contract.md`, and `docs/research/EXACT_RATIONAL_CONVERSION_FEASIBILITY.md`):

1. **C01 / C02 (Error Model Divergence across C++ Types vs Normative Types):**
   - Normative specification (`spec/t81-data-types.md §2.3`, `spec/determinism-profile.md §3`) specifies that arithmetic overflow, underflow, and conversion domain errors propagate through explicit, deterministic algebraic result containers (`Result[T, E]` / `T81Result<T>`).
   - Implementation (`include/t81/types/T81Int.hpp`, `include/t81/types/T81Float.hpp`) relies on host C++ exceptions (`std::overflow_error`, `std::domain_error`) or immediate process termination via `axion::trap_overflow()`.
2. **C06 / C07 (T81Float Conversion Contract & Special Value Handling):**
   - Normative specification prohibits silent precision loss and forbids NaN/Infinity as unhandled numeric entities without explicit error flags.
   - Implementation (`include/t81/types/T81Float.hpp`) implements `from_double(double) noexcept` which silently truncates, returns `NaE` or `Inf` sentinels, and relies on host `<cmath>` transcendentals and floating-point rounding.
3. **C08 / C09 (TISC State Transition & Fault Containment Divergence):**
   - Normative specification (`spec/tisc-spec.md`, `RFC-0002`) prescribes clean instruction-boundary traps where a faulting instruction does not commit partial register state or corrupt program counter progression.
   - Runtime implementation (`vm/vm.cpp`) increments PC and mutates system registers (R75 instruction count, R77 fault reason) before raising traps, exhibiting observable legacy side-effects across fault boundaries.
4. **C10 (Surface Result API Disconnect):**
   - Header `include/t81/types/Result.hpp` aliases `Result<T, E> = T81Result<T>`, ignoring the secondary error template parameter `E` entirely and forcing errors into fixed `T81Error` records.

---

## 3. Minimum Normative Decisions Required

To achieve complete semantic coherence without destabilizing operational components, project maintainers must decide four core policy items:

### Decision D1: T81Int Arithmetic Error Propagation Surface
- **Core Question:** Should `T81Int<N>` arithmetic operators retain standard C++ operator overloads throwing `std::overflow_error` (backed by `axion::trap_overflow`), or migrate to checked monadic APIs returning `T81Result<T81Int<N>>`?
- **Analysis:**
  - Standard C++ expression syntax `a * b + c` is essential for constexpr soft-math lookup tables and template metaprogramming (`include/t81/math/t81_soft_math/`).
  - Replacing operator overloads with `Result` would break hundreds of internal compilation units, or require dual APIs (e.g., unchecked throwing `operator*` vs explicit `checked_mul() -> T81Result<T81Int<N>>`).

### Decision D2: T81Float Binary Conversion Contract & Rounding Policy
- **Core Question:** How should IEEE 754 binary64 ↔ balanced ternary conversion (`from_double` / `to_double`) be standardized across the public boundary?
- **Options (from RFC-00D5 and SI-02):**
  - *Option A (Status Quo Sentinel):* Retain non-throwing `from_double(double) noexcept` returning `T81Float` with `NaE`/`Inf` sentinels, fixing only host `<cmath>` non-determinism via soft-math rational decomposition.
  - *Option B (Additive Checked Profile):* Retain existing `from_double` for unchecked use, but introduce a standardized, additive `checked_from_double(double, RoundingMode) -> T81Result<T81Float>` supporting `exact_only` and `nearest_even`.
  - *Option C (Full Monadic Replacement):* Deprecate unchecked conversion completely; all conversions must return `T81Result<T81Float>`.

### Decision D3: TISC Fault & State-Transition Atomic Boundaries
- **Core Question:** When a TISC opcode faults (e.g. division by zero, arithmetic overflow, memory bounds violation), should the state transition exhibit atomic abort (zero state commit, PC unchanged) or legacy side-effects (PC advanced, R75/R77 updated)?
- **Options (from RFC-00D6):**
  - *Option A (Frozen Legacy CPA-006 Profile):* Preserve exact hosted `vm/vm.cpp` and freestanding Axion OS behavior where PC advances to `pc_before + 1` and R75/R77 record the fault step. This guarantees backward compatibility with all 19 CPA-006 conformance vectors.
  - *Option B (Forward Versioned Profile `tisc-v2.0`):* Define an artifact-bound explicit profile where faulting instructions commit no register mutations and restore PC, while retaining legacy profile under explicit profile tag `cpa-006`.

### Decision D4: Result and T81Result Surface Standardization
- **Core Question:** How should `include/t81/types/Result.hpp` and `include/t81/types/T81Result.hpp` reconcile the two-type-parameter contract `Result[T, E]` with single-parameter `T81Result<T>`?
- **Options:**
  - *Option A (Canonical Monad):* Standardize on `T81Result<T>` where errors are uniformly typed as `t81::T81Error` (containing symbol code, descriptive string, and source origin). Update `Result.hpp` docstrings to formally document that `Result<T, E = T81Error>` ignores custom `E` in favor of uniform error taxonomy.
  - *Option B (Generic Monad):* Refactor `T81Result<T, E = T81Error>` into a true two-parameter sum type (`std::variant<T, E>`), enabling arbitrary userland error payload types.

---

## 4. Comprehensive SCG-04 Decision Table

The following table distinguishes frozen normative requirements, current implementation behavior, and proposed reconciliation policies:

| Decision ID | Focus Area | Frozen Specification (`spec/`) | Existing Implementation (`include/`, `vm/`) | Proposed SCG-04 Reconciliation Policy | Compatibility Consequence |
|---|---|---|---|---|---|
| **D1** | `T81Int` Arithmetic Errors | `spec/t81-data-types.md §2.1` mandates deterministic overflow reporting; `spec/determinism-profile.md` expects `Result[T,E]` in safe subsets. | `T81Int.hpp` overloads `+,-,*` throwing `std::overflow_error` and calling `axion::trap_overflow()`. No dynamic allocation; `constexpr`. | **Dual Surface Architecture:** Preserve existing throwing operator overloads for C++ expression ergonomics and constexpr evaluation; add non-throwing checked methods (`checked_add`, `checked_mul`) returning `T81Result<T81Int<N>>`. | **Zero Breaking Changes:** Full backward compatibility with existing C++ code and soft-math tables, while satisfying normative `Result` requirements for safe language surfaces. |
| **D2** | `T81Float` Conversion & Rounding | `spec/t81-data-types.md §2.3` prohibits NaN/Inf as unhandled values; requires explicit loss errors. RFC-00D5 (draft) defines `exact_only` and `nearest_even`. | `T81Float.hpp` provides `from_double(double) noexcept` returning sentinels (`NaE`/`Inf`) and uses host `<cmath>` functions (`powl`, `log`). | **Additive Checked Contract (RFC-00D5 Profile):** Keep `from_double` as legacy bridge; introduce `t81::checked_from_double(double, ConversionMode) -> T81Result<T81Float>` using exact integer rational decomposition (no host `<cmath>`). | **Non-Breaking Extension:** Existing callers remain intact. High-integrity / governed execution paths gain host-independent deterministic conversion. |
| **D3** | TISC Fault Transitions | `spec/tisc-spec.md` specifies clean trap boundaries; `RFC-0002` specifies deterministic state transitions without corrupt partial state. | `vm/vm.cpp` advances PC to `pc+1`, increments R75, and records fault in R77 before returning `Trap`. Validated by CPA-006 (19 cases). | **Artifact-Bound Profile Selection (RFC-00D6):** Designate existing runtime behavior as frozen profile `tisc-v1.1` (CPA-compatible). Define clean fault semantics strictly under explicit target profile `tisc-v2.0`. | **Preserves CPA Certification:** No regression on 1,190 frozen CPA-006 state records; clean migration path for future toolchains. |
| **D4** | `Result` Surface API | `spec/determinism-profile.md §3` references `Result[T,E]` as a generic two-parameter algebraic sum type. | `Result.hpp` aliases `template <typename T, typename E = T81Error> using Result = T81Result<T>;`. `T81Result` fixes error to `T81Error`. | **Standardized Uniform Error Taxonomy:** Retain `T81Error` as the universal error payload; update documentation and surface headers to clarify that `Result<T, E>` uses canonical `T81Error` metadata. | **Header-Only Clarity:** Eliminates signature confusion without breaking existing consumers of `T81Result<T>`. |

---

## 5. Architectural Determinism vs. Radix-Specific Advantage

A paramount scientific requirement of the T81 Research Program (established in the Master Continuity Charter v1.0 and Hypothesis Register v1.0) is to **strictly separate architectural determinism from claims of ternary superiority**:

1. **Software Determinism is Radix-Independent:**
   - The exactness of balanced-ternary integer convolution in SCG-03 or rational conversion in RFC-00D5 derives entirely from **discrete algorithmic design**, not from the base-3 radix.
   - Equivalent arbitrary-precision or bounded-width arithmetic can be implemented with identical mathematical determinism in binary, decimal, or balanced ternary.
2. **Evaluation of Radix-Specific Hypotheses (H01–H08):**
   - **H02 (Balanced Ternary Arithmetic Cost):** Benchmark and analysis history has closed this negatively for binary-emulated CPU software. Convolution in balanced ternary incurs larger carry propagation overhead than native binary two's-complement arithmetic on general-purpose hardware.
   - **H04 / H06 (Representation Suitability):** Any genuine advantage of balanced ternary or Base-81 digits must be demonstrated empirically in specialized domains (e.g., ternary neural weight compression `BitNet b1.58`, or native ternary hardware logic gates), not inferred from software simulation correctness.
3. **Integrity Boundary:**
   - Solving SCG-03 and preparing SCG-04 establishes **systemic correctness, reproducible testing, and specification coherence**; it does *not* constitute evidence of computational speedups or efficiency gains over binary systems.

---

## 6. Compatibility Consequences & Maintainer Action Items

### 6.1 Compatibility Impact Matrix
- **C++ Library Consumers:** Zero breaking changes under the recommended dual-surface policy (D1, D2, D4). Existing exception-handling and sentinel workflows remain functional.
- **T81VM & Freestanding Axion OS:** Zero breaking changes to existing bytecode execution or frozen CPA test suites under profile-bound preservation (D3).
- **Formal Verification & Conformance:** Provides a clear specification contract against which automated solvers and conformance test generators can validate error states.

### 6.2 Required Maintainer Authorizations
Before any production implementation or normative document editing begins for SCG-04, project maintainers must formally approve:
1. Adoption of the Dual Surface Error Model for `T81Int` (preserving C++ exceptions while exposing checked `Result` methods).
2. Promotion of draft `RFC-00D5` to standards-track with the additive `checked_from_double` contract.
3. Adoption of `RFC-00D6` profile selection matrix to prevent breaking CPA-006 frozen test suites.
4. Formal resolution of the `Result<T, E>` surface mapping in `include/t81/types/Result.hpp`.

---

## 7. Disposition and Next Steps

- **Current Status:** SCG-04 Preparation complete.
- **Recommended Next Step:** Submit this decision memorandum to maintainers for formal review and signature. No code changes to `T81Float.hpp`, `vm/vm.cpp`, or `spec/` should be applied until maintainer decisions D1–D4 are executed.
