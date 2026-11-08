# T81 SCG-04 Maintainer Decision Record — Normative Semantic Reconciliation

**Document:** `docs/research/continuity/SCG04_MAINTAINER_DECISION_RECORD.md`
**Date:** 2026-11-08
**Task Anchor:** SCG-04 Maintainer Decision Preparation
**Authoritative Baseline:** Merged PR #730, merge commit `b6ed932285866f2185c7d8da3d1f9ec22d194fc8`
**Status:** PROPOSED DECISION INSTRUMENT — PENDING MAINTAINER SIGNATURE
**Governance Posture:** Strict non-normative instrument; zero code modifications, zero specification alterations, zero ISA expansions, zero benchmark executions, zero Phase C activations, zero automatic merges.

---

## 1. Executive Summary & Context

Under the Scientific Continuity and Governance (SCG) track, task SCG-03 successfully repaired contradiction items C05 (intermediate multiplication truncation and false overflow) and C11 (assertion-based test verdicts compiled away under `NDEBUG`), merged into `main` via PR #729 and closed in PR #730 (commit `b6ed932285866f2185c7d8da3d1f9ec22d194fc8`).

The companion document `docs/research/continuity/SCG04_PREPARATION_DECISION_MEMORANDUM.md` synthesized the remaining unresolved contradictions from SCG-01 (C01, C02, C06, C07, C08, C09, C10) into four essential normative decision points:
- **D1:** `T81Int<N>` arithmetic error propagation surface.
- **D2:** `T81Float` binary conversion contract and rounding policy.
- **D3:** TISC fault and state-transition atomic boundaries.
- **D4:** `Result` and `T81Result` surface type standardization.

This document constitutes the formal, actionable **Maintainer Decision Record** for D1–D4. It provides the detailed justification, trade-off analysis, compatibility impact, unresolved questions, explicit authorizations, dependency sequencing, and a SHA-pinned maintainer ballot with explicit `APPROVE` / `REJECT` / `DEFER` options.

---

## 2. Baseline Cryptographic Inventory

All evaluations, references, and proposed decisions are strictly pinned against the authoritative repository state at commit `b6ed932285866f2185c7d8da3d1f9ec22d194fc8`:

| Component / Artifact | Path | SHA-256 Digest |
|---|---|---|
| SCG-04 Decision Memorandum | `docs/research/continuity/SCG04_PREPARATION_DECISION_MEMORANDUM.md` | `aadf231d258caecf6127070af73991521af324426380510d58ce5e1f32fe3348` |
| RFC-00D5 (Checked Float Conversion) | `spec/rfcs/RFC-00D5-checked-floating-point-conversion.md` | `98a4c0058865ab955ef0d7f5d7dd1a78ffe809caba34fa152912d1b7e3f154e9` |
| RFC-00D6 (TISC State Transition Contract) | `spec/rfcs/RFC-00D6-versioned-tisc-state-transition-contract.md` | `98f1a287bb07705f7dedd527709024bbd9d3900769245f0b1d472481e5cef27a` |
| T81 Integer Header | `include/t81/types/T81Int.hpp` | `42a666a422d10e157b5a9f508c854182478f8d6cef80722a322c78c95913256b` |
| T81 Float Header | `include/t81/types/T81Float.hpp` | `6360035708d3a4292a889af13c4202d928fe43dabf4f6016e5a40cd4cc910032` |
| Result Surface Header | `include/t81/types/Result.hpp` | `1ed885c2b69a6bd3e11a4354f319e5aa82f697f89fcf262526f0ab37b2126df9` |
| Core Result Container Header | `include/t81/types/T81Result.hpp` | `200b4e79668aac8f03c233f8e7a6ea7d4cc0bc59cb54324ec26a3013c68b462e` |
| Hosted VM Implementation | `vm/vm.cpp` | `2ed93572891bc9e32f7525270c7ff6a88edb3d5f22529e3f19195f71cb1ff998` |
| Normative Data Types Spec | `spec/t81-data-types.md` | `b74c75e09740ebd2d0a1b348951d208d75cfe1033f6c04cccf8fd93e500511f0` |
| Normative TISC ISA Spec | `spec/tisc-spec.md` | `c0a1f84a0669a61f1092902470f899c0811ea071f7cf69707d928510e0e5b01c` |

---

## 3. Detailed Decision Proposals: D1–D4

### 3.1 Decision D1: `T81Int<N>` Arithmetic Error Propagation Surface
- **Topic & Contradiction:** Contradictions C01 / C02. Normative specification (`spec/t81-data-types.md §2.1`, `spec/determinism-profile.md §3`) specifies that arithmetic overflow and underflow propagate through explicit, deterministic algebraic result containers (`Result[T, E]` / `T81Result<T>`). In contrast, C++ implementation (`include/t81/types/T81Int.hpp`) overloads operators `+`, `-`, `*` which invoke `axion::trap_overflow()` (throwing `std::overflow_error`), retaining `constexpr` evaluation with zero dynamic allocation.
- **Recommended Option:** **Dual Surface Architecture**
  1. *Preserve Existing Operator Overloads:* Retain standard C++ operator overloads (`+`, `-`, `*`) with `axion::trap_overflow()` invocation throwing `std::overflow_error`. This maintains expression ergonomics and `constexpr` evaluation essential for compile-time soft-math tables (`include/t81/math/t81_soft_math/`).
  2. *Additive Checked Monadic Methods:* Expose explicit non-throwing checked methods:
     ```cpp
     [[nodiscard]] constexpr T81Result<T81Int<N>> checked_add(const T81Int<N>& rhs) const noexcept;
     [[nodiscard]] constexpr T81Result<T81Int<N>> checked_sub(const T81Int<N>& rhs) const noexcept;
     [[nodiscard]] constexpr T81Result<T81Int<N>> checked_mul(const T81Int<N>& rhs) const noexcept;
     ```
- **Alternatives Considered:**
  - *Alternative A (Status Quo Only):* Reject checked methods; keep throwing operator overloads only. Leaves C01/C02 permanently contradictory with normative determinism profiles.
  - *Alternative B (Full Monadic Migration):* Deprecate or remove throwing operator overloads entirely, forcing all arithmetic through `T81Result`. Breaks hundreds of internal call sites, soft-math tables, and makes basic arithmetic expressions like `a * b + c` unergonomic in C++.
- **Compatibility Impact:** **Zero breaking changes.** Existing C++ consumers and constexpr soft-math tables remain 100% binary- and source-compatible. Satisfies normative requirements for deterministic safe profiles.
- **Unresolved Questions:** Whether `T81Result` error construction (`T81Error`) can be made fully `constexpr` without heap allocation or whether a lightweight `T81ErrorCode` is required for constexpr checked methods.
- **Exact Authorization Required:** Formal maintainer approval to maintain throwing operator overloads in C++ while authorizing addition of non-throwing `checked_*` methods returning `T81Result<T81Int<N>>`.

---

### 3.2 Decision D2: `T81Float` Binary Conversion Contract & Rounding Policy
- **Topic & Contradiction:** Contradictions C06 / C07. Normative specification (`spec/t81-data-types.md §2.3`) prohibits silent precision loss and forbids NaN/Infinity as unhandled numeric entities without explicit error flags. Implementation (`include/t81/types/T81Float.hpp`) provides `from_double(double) noexcept` which silently truncates, returns `NaE` or `Inf` sentinels, and relies on non-deterministic host `<cmath>` transcendentals and FPU rounding.
- **Recommended Option:** **Additive Checked Profile via RFC-00D5**
  1. *Preserve Legacy Bridge:* Retain existing `from_double(double) noexcept` and `to_double() noexcept` as legacy adapters with documented non-deterministic host dependencies.
  2. *Adopt Additive Checked Profile:* Promote RFC-00D5 to standards-track and authorize the implementation of:
     ```cpp
     std::expected<Conversion<T81Float<M, E>>, ConversionError>
     checked_from_double(double value, ConversionMode mode = ConversionMode::ExactOnly);

     std::expected<Conversion<double>, ConversionError>
     checked_to_double(const ValidatedFloat<M, E>& value, ConversionMode mode = ConversionMode::ExactOnly);
     ```
  3. *Exact Integer Rational Decomposition:* Mandate host-independent integer decomposition (`fraction * 2^-1074` / `(2^52 + fraction) * 2^(exp-1075)`), strict tie-breaking (even integer mantissa, then lower exponent magnitude), explicit error taxonomy (`inexact`, `underflow`, `overflow`, `nonfinite_input`), and negative zero normalization (`+0E0` with `negative_zero_normalized` flag).
- **Alternatives Considered:**
  - *Alternative A (Status Quo Sentinels):* Keep bare `from_double noexcept`, fixing only host `<cmath>` via soft-math table lookups without returning error states. Cannot distinguish underflow to zero from valid zero, nor report exactness loss.
  - *Alternative B (Full Replacement / Breaking Deprecation):* Delete or alter legacy `from_double`. Breaks VM FloatHandle pools, CLI tensor importers, and changes existing CanonFS content hashes across historical artifacts.
- **Compatibility Impact:** **Non-breaking extension.** Legacy callers remain undisturbed. Governed/DCP execution profiles gain host-independent, bit-exact floating-point conversion.
- **Unresolved Questions:** Final C++ wrapper signature naming (`std::expected` vs `t81::T81Result`); admission bounds for template parameters (RFC-00D5 proposes $4 \le M \le 72, 4 \le E \le 9$).
- **Exact Authorization Required:** Maintainer approval promoting RFC-00D5 to normative standards-track, adopting the finite parameter lattice ($M \le 72, E \le 9$), exact-only and nearest-even rounding rules, and authorizing implementation under a new checked namespace/header.

---

### 3.3 Decision D3: TISC Fault & State-Transition Atomic Boundaries
- **Topic & Contradiction:** Contradictions C08 / C09. Normative specification (`spec/tisc-spec.md`, `RFC-0002`) prescribes atomic instruction-boundary traps where a faulting instruction does not commit partial register state or corrupt program counter progression. Runtime implementation (`vm/vm.cpp`, `ternaryos/hal/cpa004_evaluator.cpp`) increments PC (`pc = pc_before + 1`), advances sequence/tick count, and updates system registers ($R_{75}$, $R_{77}$) before raising a trap.
- **Recommended Option:** **Artifact-Bound Profile Selection Architecture (RFC-00D6 Option C)**
  1. *Preserve Frozen Historical Profile (`tisc-cpa-v1.0`):* Formally designate the implemented VM/freestanding behavior as frozen profile `tisc-cpa-v1.0`. In this profile, observable fault side-effects (PC advances, sequence increments, $R_{75}/R_{77}$ update, destination uncommitted) are ratified as intentional to maintain bit-exact compatibility with the 1,190 state records of CPA-006 (Cases C07, C08, C09).
  2. *Define Forward Specification Profile (`tisc-v2.0`):* Specify clean atomic fault transactions (zero PC advancement, zero tick increment, zero register mutation) strictly under a new, explicitly tagged forward profile (`tisc-v2.0`).
  3. *Fail-Closed Profile Selection:* Implement strict profile resolution (CLI flag `--isa-profile` > container manifest > binary note; contradictory explicit declarations fail closed immediately).
- **Alternatives Considered:**
  - *Alternative A (Retroactive Universal Atomic Faults):* Alter `vm/vm.cpp` to preserve PC on all faults across all executions. Immediately breaks 100% of CPA-006 conformance vectors (19 cases, 1,190 records) and invalidates verified freestanding Axion OS traces.
  - *Alternative B (Abandon Atomic Spec):* Amend `spec/tisc-spec.md` to abandon atomic fault containment and codify legacy PC-advance as universal normative law. Prevents proper transactional exception handling, trap-and-emulate virtualization, and debugger resume capability.
- **Compatibility Impact:** **Guarantees 100% historical CPA evidence preservation.** Completely insulates frozen research milestones from regression while providing an uncompromised path for future compiler targets.
- **Unresolved Questions:** Syntax for container manifest profile binding (`"isa_profile": "tisc-cpa-v1.0"`); allocation of dedicated ELF/COFF note section `.tisc_profile` for freestanding binaries.
- **Exact Authorization Required:** Maintainer sign-off adopting RFC-00D6 profile versioning architecture, ratifying `tisc-cpa-v1.0` as the permanent designation for historical CPA binaries, and establishing `tisc-v2.0` as the forward atomic specification profile.

---

### 3.4 Decision D4: `Result` and `T81Result` Surface Type Standardization
- **Topic & Contradiction:** Contradiction C10. Normative specification (`spec/determinism-profile.md §3`, `spec/t81-data-types.md`) references a generic two-parameter algebraic sum type `Result[T, E]`. Surface header `include/t81/types/Result.hpp` aliases `template <typename T> using Result = T81Result<T>;`, completely ignoring the secondary error parameter `E` and binding all errors to `t81::T81Error`.
- **Recommended Option:** **Standardized Uniform Error Taxonomy with Two-Parameter Alias Compatibility**
  1. *Canonical Error Payload:* Ratify `t81::T81Error` as the canonical error container across the T81 runtime boundary, providing structured metadata (symbol code, explanatory text, occurrence time, entropy fuel spent, source origin).
  2. *Header Alignment:* Update `include/t81/types/Result.hpp` to:
     ```cpp
     template <typename T, typename E = T81Error>
     using Result = T81Result<T>;
     ```
     Document clearly that the core runtime enforces the canonical `T81Error` taxonomy, and custom error types `E` are mapped or adapted into `T81Error` at system boundaries.
- **Alternatives Considered:**
  - *Alternative A (Full Generic Sum Type Refactor):* Rewrite `T81Result<T, E>` into `std::variant<T, E>`. Breaks reflection subsystem (`T81Reflection`), serialized error transport, CanonFS error logging, and requires pervasive codebase refactoring.
  - *Alternative B (Keep Status Quo 1-Parameter Alias):* Leave `template <typename T> using Result = T81Result<T>;`. Leaves compiler syntax errors when code written against normative `Result[T, E]` specifications is ported to C++.
- **Compatibility Impact:** **Zero breaking changes.** Existing callers of `Result<T>` and `T81Result<T>` compile without change. Code specifying `Result<T, ErrorType>` compiles cleanly with default mapping.
- **Unresolved Questions:** Whether `T81Error` can optionally store a type-erased or variant payload for userland domain errors without compromising determinism or introducing non-deterministic timestamps.
- **Exact Authorization Required:** Maintainer approval to standardize on `T81Error` as the universal error taxonomy while updating `Result.hpp` to accept `template <typename T, typename E = T81Error>`.

---

## 4. Dependencies, Sequencing, and Categorization

### 4.1 Dependency and Sequencing Analysis

The four decisions exhibit clear interdependencies and natural execution sequencing:

```
+-------------------------------------------------------------------------+
|                  SCG-04 Decision Sequencing & Flow                      |
+-------------------------------------------------------------------------+

  [ Decision D4: Result Surface Standardization ]  <-- (Foundational Container)
           |
           +-----------------------+
           |                       |
           v                       v
  [ Decision D1: T81Int ]   [ Decision D2: T81Float ]
    Checked Methods           Checked Bridges (RFC-00D5)
           |                       |
           +-----------+-----------+
                       |
                       v
  [ Decision D3: TISC Fault & State Transitions (RFC-00D6) ]
    (Independent Execution Profile Architecture)
```

1. **D4 (Result Surface Standardization) is Foundational:**
   Standardizing `Result<T, E>` and `T81Result<T>` establishes the return container contract needed for checked methods in D1 and D2. D4 should be settled first or concurrently.
2. **D1 and D2 Depend on Container Readiness (D4):**
   - D1 (`checked_add`, `checked_mul`) returns `T81Result<T81Int<N>>`.
   - D2 (`checked_from_double`) returns structured expected/result types.
   - D1 and D2 can proceed independently of each other.
3. **D3 (TISC State Transitions) is Architecturally Independent:**
   D3 governs VM and ISA instruction execution boundaries (`vm/vm.cpp`, `spec/tisc-spec.md`, CPA suites). It has no semantic dependency on C++ type surfaces D1, D2, or D4, and can be evaluated in parallel.

### 4.2 Categorization: Approval Without Code Changes vs. Future Implementation Authorization

To maintain strict governance boundaries, decisions must be cleanly bifurcated into policy approvals and implementation authorizations:

| Decision | Part A: Specification / Policy Approval (No Code Changes Required) | Part B: Implementation Authorization (Future Engineering PR Required) |
|---|---|---|
| **D1 (T81Int)** | **Approvable Immediately:** Approve dual-surface policy (preserving throwing C++ operators for ergonomics/constexpr; endorsing checked methods in spec). | **Requires Future Authorization:** Authorize PR adding `checked_add`, `checked_sub`, `checked_mul` methods and tests to `include/t81/types/T81Int.hpp`. |
| **D2 (T81Float)** | **Approvable Immediately:** Promote RFC-00D5 from draft to approved standards-track specification. Ratify finite lattice ($M \le 72, E \le 9$) and rounding rules. | **Requires Future Authorization:** Authorize implementation PR authoring `checked_from_double` using exact rational arithmetic and integrating into codebase. |
| **D3 (TISC Transitions)** | **Approvable Immediately:** Adopt RFC-00D6 profile selection matrix. Formally ratify `tisc-cpa-v1.0` as preserving historical CPA-006 behavior, and define `tisc-v2.0` as atomic. | **Requires Future Authorization:** Authorize implementation PR adding explicit `--isa-profile` CLI flag and profile-aware trap dispatching in `vm/vm.cpp`. |
| **D4 (Result Surface)** | **Approvable Immediately:** Approve policy standardizing `T81Error` as canonical taxonomy and adopting two-parameter alias compatibility. | **Requires Future Authorization:** Authorize PR updating `include/t81/types/Result.hpp` docstrings and template parameter defaults. |

---

## 5. Architectural Determinism vs. Radix-Specific Advantage

In accordance with the **Master Continuity Charter v1.0** and **Hypothesis Register v1.0**, the maintainer decision record explicitly records that:

1. **Deterministic Semantics are Radix-Independent:**
   - The correctness of balanced-ternary integer arithmetic (SCG-03 convolution), exact floating-point rational conversion (RFC-00D5), and atomic VM state transitions (RFC-00D6) derives strictly from **rigorous discrete mathematical specification**, not from any inherent superiority of radix-3 representation.
   - Any mathematical property implemented here can be realized with identical determinism in radix-2, radix-10, or arbitrary-radix architectures.
2. **Status of Radix Hypotheses (H01–H08):**
   - **H02 (Balanced Ternary Signed Arithmetic Advantage):** Remains closed negative for software running on binary CPUs. Balanced ternary integer convolution and digit normalization incur substantial emulation overhead relative to native binary two's-complement arithmetic.
   - **H04 / H06 (Representation & Ternary Weights):** Empirical advantages, if any, are strictly confined to specialized domains (e.g. 1.58-bit ternary neural network inference or native ternary hardware gates), and cannot be established by VM interpreter software.
3. **Integrity Rule:** Approving D1–D4 establishes specification coherence, cross-profile reproducibility, and system testability; it does not claim or imply computational speedups over binary systems.

---

## 6. Actionable Maintainer Decision Ballot

*This section provides the explicit, SHA-pinned ballot for project maintainer review and determination.*

```
================================================================================
                    T81 FOUNDATION — SCG-04 MAINTAINER BALLOT
================================================================================
Authoritative Baseline: commit b6ed932285866f2185c7d8da3d1f9ec22d194fc8
Date of Evaluation:     2026-11-08
Reviewing Maintainer:   ________________________________________________________

--------------------------------------------------------------------------------
DECISION D1: T81Int<N> Arithmetic Error Propagation Surface
--------------------------------------------------------------------------------
Recommended: Adopt Dual Surface Architecture (preserve throwing operators for
             constexpr/ergonomics; add non-throwing checked_* monadic methods).

[ ] APPROVE Recommendation
[ ] REJECT Recommendation
[ ] DEFER Decision
Comments / Directives: _________________________________________________________

--------------------------------------------------------------------------------
DECISION D2: T81Float Binary Conversion Contract & Rounding Policy
--------------------------------------------------------------------------------
Recommended: Promote RFC-00D5 to Standards-Track with Additive Checked Profile
             (preserve legacy from_double bridge; introduce checked_from_double
             with exact rational decomposition, M<=72, E<=9, nearest-even mode).

[ ] APPROVE Recommendation
[ ] REJECT Recommendation
[ ] DEFER Decision
Comments / Directives: _________________________________________________________

--------------------------------------------------------------------------------
DECISION D3: TISC Fault & State-Transition Atomic Boundaries
--------------------------------------------------------------------------------
Recommended: Adopt RFC-00D6 Profile Selection Architecture
             (freeze legacy VM/CPA behavior as profile tisc-cpa-v1.0; specify
             atomic zero-commit fault semantics under forward profile tisc-v2.0;
             conflicts fail closed).

[ ] APPROVE Recommendation
[ ] REJECT Recommendation
[ ] DEFER Decision
Comments / Directives: _________________________________________________________

--------------------------------------------------------------------------------
DECISION D4: Result and T81Result Surface Standardization
--------------------------------------------------------------------------------
Recommended: Adopt Uniform Error Taxonomy with Two-Parameter Template Alias
             (standardize on T81Error; update Result<T, E = T81Error> alias).

[ ] APPROVE Recommendation
[ ] REJECT Recommendation
[ ] DEFER Decision
Comments / Directives: _________________________________________________________

--------------------------------------------------------------------------------
OVERALL MAINTAINER STAMP & DISPOSITION
--------------------------------------------------------------------------------
[ ] ALL RECOMMENDED POLICIES APPROVED (Proceed to Implementation PR Authorization)
[ ] PARTIALLY APPROVED (See individual directives above)
[ ] REVISION REQUIRED (Address specific findings before re-submitting)
[ ] REJECTED

Formal Maintainer Signature: ___________________________________________________
Date Signed (UTC):           ___________________________________________________
================================================================================
```

---

## 7. Evidence References & Blockers

### 7.1 Authoritative Evidence References
- **SCG-01 Contradiction Register:** Analyzed in `docs/research/continuity/SCG04_PREPARATION_DECISION_MEMORANDUM.md`.
- **SCG-03 Evidence Receipt:** Pinned in `docs/research/continuity/SCG03_EVIDENCE_RECEIPT.md` (baseline `2c3501e2`, formatted in `b6ed9322`).
- **RFC-00D5 (Checked Float Conversion):** `spec/rfcs/RFC-00D5-checked-floating-point-conversion.md`.
- **RFC-00D6 (TISC State Transition Contract):** `spec/rfcs/RFC-00D6-versioned-tisc-state-transition-contract.md`.
- **CPA-006 Conformance Suite:** 19 cases, 1,190 state records verifying hosted T81VM and freestanding Axion OS execution parity.
- **TISC Freeze Integrity Gate:** `scripts/ci/check_tisc_freeze_integrity.py`.

### 7.2 Unresolved External Blockers
- **Maintainer Custody & Signature:** Production code modifications to `T81Float.hpp`, `vm/vm.cpp`, or `spec/` remain strictly blocked until the maintainer ballot in Section 6 is executed and signed.
- **Phase C Benchmark Authorization:** Strictly frozen and unauthorized across all research branches.
- **External CI Infrastructure Outages:**
  1. *FTXUI Archive HTTP 504:* Intermittent CodeQL autobuild failure when external GitHub archive host times out.
  2. *Lychee Link Check HTTP 503:* Lychee crawler reporting transient HTTP 503 when querying external GitHub Actions workflow run URLs.
