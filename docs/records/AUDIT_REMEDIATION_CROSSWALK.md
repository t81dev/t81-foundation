# T81 Foundation Audit Remediation Crosswalk

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation Audit Remediation Crosswalk](#t81-foundation-audit-remediation-crosswalk)
  - [Executive Summary](#executive-summary)
  - [1. Floating-Point Drift Risk](#1-floating-point-drift-risk)
    - [Current Status](#current-status)
    - [Findings](#findings)
    - [Risk Analysis](#risk-analysis)
    - [Remediation Actions](#remediation-actions)
    - [Proof Artifacts](#proof-artifacts)
    - [Result: **VERIFIED FOR CURRENT TEST COVERAGE** - No more silent fallback, limited deterministic functionality](#result-**verified-for-current-test-coverage**---no-more-silent-fallback-limited-deterministic-functionality)
  - [2. JIT Determinism Boundary](#2-jit-determinism-boundary)
    - [Current Status](#current-status)
    - [Findings](#findings)
    - [Risk Analysis](#risk-analysis)
    - [Remediation Actions](#remediation-actions)
    - [Proof Artifacts](#proof-artifacts)
  - [3. T81Map / Hash-Backed Nondeterminism Risk](#3-t81map--hash-backed-nondeterminism-risk)
    - [Current Status](#current-status)
    - [Findings](#findings)
    - [Risk Analysis](#risk-analysis)
    - [Remediation Actions](#remediation-actions)
    - [Proof Artifacts](#proof-artifacts)
    - [Result: **VERIFIED FOR CURRENT TEST COVERAGE** - Deterministic iteration available, boundaries clear](#result-**verified-for-current-test-coverage**---deterministic-iteration-available-boundaries-clear)
  - [4. T81Lang Traceability Gap](#4-t81lang-traceability-gap)
    - [Current Status](#current-status)
    - [Findings](#findings)
    - [Risk Analysis](#risk-analysis)
    - [Remediation Actions](#remediation-actions)
    - [Proof Artifacts](#proof-artifacts)
    - [Result: **VERIFIED FOR CURRENT TEST COVERAGE** - Basic verification implemented, comprehensive traceability remains future work](#result-**verified-for-current-test-coverage**---basic-verification-implemented-comprehensive-traceability-remains-future-work)
  - [5. Axion Maturity Gap](#5-axion-maturity-gap)
    - [Current Status](#current-status)
    - [Findings](#findings)
    - [Risk Analysis](#risk-analysis)
    - [Remediation Actions](#remediation-actions)
    - [Proof Artifacts](#proof-artifacts)
    - [Result: **VERIFIED FOR CURRENT TEST COVERAGE** - Basic evidence verified, advanced gaps remain](#result-**verified-for-current-test-coverage**---basic-evidence-verified-advanced-gaps-remain)
  - [6. Tier Transition / Cognitive-Tier Orchestration Deferral](#6-tier-transition--cognitive-tier-orchestration-deferral)
    - [Current Status](#current-status)
    - [Findings](#findings)
    - [Risk Analysis](#risk-analysis)
    - [Remediation Actions](#remediation-actions)
    - [Proof Artifacts](#proof-artifacts)
  - [Remediation Priority Matrix](#remediation-priority-matrix)
  - [Success Criteria](#success-criteria)
  - [Files Requiring Changes](#files-requiring-changes)
    - [Code Changes](#code-changes)
    - [Documentation](#documentation)
    - [CI/Testing](#citesting)

<!-- T81-TOC:END -->


**Generated:** 2026-03-06  
**Audit Reference:** Repository Hardening Pass  
**Scope:** Deterministic Core Profile (DCP) and Governance Boundaries

## Executive Summary

This crosswalk maps each audit finding to current implementation status, risk assessment, and specific remediation actions required to strengthen the repository while preserving deterministic behavior and governance boundaries.

---

## 1. Floating-Point Drift Risk

### Current Status
- **Location:** `include/t81/types/T81Float.hpp`
- **Implementation:** Explicit rejection of unsupported deterministic operations
- **Risk Level:** MEDIUM - Operations rejected rather than silently falling back

### Findings
1. **Lines 334-466:** Advanced transcendentals (`acos`, `asin`, `atan`, `sinh`, `cosh`, `tanh`, `pow`) explicitly reject in deterministic mode
2. **Lines 628-635:** Division explicitly rejects in deterministic mode
3. **Lines 267-274, 276-283, 294-301, 303-310, 312-319:** Core transcendentals (`sin`, `cos`, `exp`, `log`, `sqrt`) route to deterministic `t81_soft_math` when `T81_DETERMINISTIC` is defined

### Risk Analysis
- **Deterministic Mode:** Unsupported operations explicitly rejected (returns NaE)
- **Non-Deterministic Mode:** Host math dependency creates cross-platform variance
- **Gap:** No deterministic implementation for advanced operations (rejection instead)

### Remediation Actions
1. **COMPLETED:** Remove undefined macro guards that caused broken behavior
2. **COMPLETED:** Explicit rejection of unsupported operations in deterministic mode
3. **COMPLETED:** Add deterministic profile boundary tests
4. **COMPLETED:** Clarify exact deterministic guarantees in API docs

### Proof Artifacts
- Tests: `tests/cpp/test_deterministic_float_boundaries.cpp`
- Code: `include/t81/types/T81Float.hpp` (explicit rejection guards)
- Spec: `spec/determinism-profile.md` (Lines 64-68)

### Result: **VERIFIED FOR CURRENT TEST COVERAGE** - No more silent fallback, limited deterministic functionality

---

## 2. JIT Determinism Boundary

### Current Status
- **Location:** `include/t81/jit/jit.hpp`
- **Implementation:** Minimal trace recording infrastructure
- **Risk Level:** MEDIUM - JIT explicitly outside DCP but boundaries unclear

### Findings
1. **Lines 14-34:** `JitTrace` class with policy hooks but no equivalence proofs
2. **Lines 40-53:** `JitCompiler` with basic recording but no determinism guarantees
3. **Architecture:** JIT correctly excluded from Deterministic Core Profile

### Risk Analysis
- **Boundary:** JIT is properly outside DCP (per spec)
- **Gap:** No equivalence scaffolding for future deterministic inclusion
- **Exposure:** Limited - JIT usage is experimental and marked as such

### Remediation Actions
1. **Scaffolding:** Build interpreter vs JIT trace comparison harness
2. **Documentation:** Create explicit JIT equivalence gap document
3. **Tests:** Add JIT determinism boundary smoke tests
4. **CI:** Gate JIT usage behind experimental flags

### Proof Artifacts
- Infrastructure: `include/t81/jit/jit.hpp`
- Gap Doc: (To be created) `docs/status/JIT_EQUIVALENCE_GAP.md`
- Spec: `spec/determinism-profile.md` (JIT exclusion)

---

## 3. T81Map / Hash-Backed Nondeterminism Risk

### Current Status
- **Location:** `include/t81/types/T81Map.hpp`
- **Implementation:** Added deterministic iterator with explicit non-deterministic default
- **Risk Level:** LOW - Deterministic iteration available, default clearly documented

### Findings
1. **Lines 301-351:** Added `deterministic_iterator` with guaranteed sorted order
2. **Lines 344-351:** Added `dbegin()`/`dend()` methods for deterministic iteration
3. **Lines 273-275:** Default iterator documented as non-deterministic for performance
4. **Lines 319-370:** `iter_sorted()` and `serialize_canonical()` provide canonical ordering

### Risk Analysis
- **Deterministic Path:** `dbegin()`/`dend()` provides guaranteed sorted iteration
- **Non-Deterministic Path:** Default `begin()`/`end()` documented as hash-dependent
- **Gap:** Users must explicitly choose deterministic iteration

### Remediation Actions
1. **COMPLETED:** Add deterministic iterator with guaranteed ordering
2. **COMPLETED:** Provide explicit `dbegin()`/`dend()` methods
3. **COMPLETED:** Document default iterator as non-deterministic
4. **COMPLETED:** Add property tests for deterministic iteration

### Proof Artifacts
- Tests: `tests/cpp/test_map_determinism.cpp`
- Code: `include/t81/types/T81Map.hpp` (deterministic_iterator)
- Spec: `spec/determinism-profile.md` (Lines 69-72)

### Result: **VERIFIED FOR CURRENT TEST COVERAGE** - Deterministic iteration available, boundaries clear

---

## 4. T81Lang Traceability Gap

### Current Status
- **Location:** `spec/t81lang-spec.md`, `lang/frontend/` directory
- **Implementation:** Compiler fixtures deterministic, basic enforcement tests added
- **Risk Level:** MEDIUM - Limited but verifiable traceability from source to runtime

### Findings
1. **Spec:** `spec/t81lang-spec.md` defines language but traceability partial
2. **Frontend:** `lang/frontend/` contains lexer, parser, semantic analyzer
3. **Gap:** Limited verification of construct-to-bytecode mapping
4. **Progress:** Basic compilation stability and bytecode verification tests implemented

### Risk Analysis
- **Compilation:** Deterministic on tested fixtures
- **Coverage:** Partial traceability with basic verification
- **Exposure:** Some auditability but not comprehensive

### Remediation Actions
1. **COMPLETED:** Add compilation stability verification tests
2. **COMPLETED:** Add construct-to-opcode mapping verification
3. **COMPLETED:** Add golden fixture generation for regression testing
4. **NARROWED:** Traceability matrix updated to reflect actual verification scope

### Proof Artifacts
- Tests: `tests/cpp/test_t81lang_traceability_enforcement.cpp`
- Golden Fixtures: `tests/fixtures/traceability/basic_arithmetic.golden`
- Matrix: `docs/status/T81LANG_TRACEABILITY_MATRIX.md` (updated with honest scope)

### Result: **VERIFIED FOR CURRENT TEST COVERAGE** - Basic verification implemented, comprehensive traceability remains future work

---

## 5. Axion Maturity Gap

### Current Status
- **Location:** `spec/axion-kernel.md`, `include/t81/axion/`
- **Implementation:** Alpha maturity, basic VM-Axion bridge verified
- **Risk Level:** MEDIUM - Governance enforcement partially verified

### Findings
1. **Spec:** `spec/axion-kernel.md` comprehensive but implementation partial
2. **Status:** Multiple implementation gaps (AX-M5, AX-M6, AX-M7 milestones)
3. **Evidence:** Basic VM-Axion bridge integrity verified with corrected tests
4. **Gap:** Complex policy enforcement and advanced monitoring not fully verified

### Risk Analysis
- **Criticality:** Axion is core to governance enforcement
- **Maturity:** Alpha status with basic evidence closure
- **Exposure:** Governance guarantees partially verified

### Remediation Actions
1. **COMPLETED:** Remove non-compiling evidence tests
2. **COMPLETED:** Add corrected tests using actual VM/Axion APIs
3. **COMPLETED:** Verify basic VM-Axion bridge integrity
4. **NARROWED:** Update status to reflect partial evidence closure

### Proof Artifacts
- Tests: `tests/cpp/test_axion_evidence_loop_closure_corrected.cpp`
- Status: `docs/records/status-history/AXION_STATUS.md` (corrected)
- API: `include/t81/axion/api.hpp`, `include/t81/vm/vm.hpp`

### Result: **VERIFIED FOR CURRENT TEST COVERAGE** - Basic evidence verified, advanced gaps remain

---

## 6. Tier Transition / Cognitive-Tier Orchestration Deferral

### Current Status
- **Location:** Experimental cognitive tiers in `experimental/`
- **Implementation:** Deferred, outside core governance guarantees
- **Risk Level:** LOW - Properly bounded as experimental

### Findings
1. **Experimental:** `experimental/cog/` and `experimental/hanoi/`
2. **Boundary:** Clearly outside DCP and core governance
3. **Status:** Appropriately deferred

### Risk Analysis
- **Boundary:** Clean separation from core guarantees
- **Exposure:** Minimal - marked as experimental
- **Risk:** Users might confuse experimental with core features

### Remediation Actions
1. **Documentation:** Clarify experimental status in all relevant docs
2. **Boundaries:** Ensure clear separation in implementation
3. **CI:** Gate experimental features behind explicit flags

### Proof Artifacts
- Code: `experimental/cog/`, `experimental/hanoi/`
- Spec: `spec/cognitive-tiers.md`
- Boundary: Clear experimental marking

---

## Remediation Priority Matrix

| Finding | Priority | Impact | Effort | Timeline |
|---------|----------|--------|--------|----------|
| Floating-Point Drift | HIGH | HIGH | MEDIUM | Immediate |
| Axion Maturity | HIGH | HIGH | HIGH | 2 weeks |
| T81Map Hash Risk | MEDIUM | MEDIUM | LOW | 1 week |
| T81Lang Traceability | MEDIUM | MEDIUM | HIGH | 2 weeks |
| JIT Boundaries | LOW | LOW | MEDIUM | 1 week |
| Tier Transitions | LOW | LOW | LOW | Documentation |

---

## Success Criteria

Each remediation must:
1. **Reduce ambiguity** around deterministic behavior
2. **Eliminate or bound** nondeterminism exposure
3. **Improve auditability** of critical paths
4. **Maintain governance boundary integrity**
5. **Add regression tests** alongside code changes
6. **Keep claims narrower** than proof, never broader

---

## Files Requiring Changes

### Code Changes
- `include/t81/types/T81Float.hpp` - Harden deterministic profile
- `include/t81/types/T81Map.hpp` - Strengthen determinism guarantees
- `tests/cpp/` - Add deterministic profile boundary tests

### Documentation
- `docs/status/AUDIT_REMEDIATION_CROSSWALK.md` - This document
- `docs/status/JIT_EQUIVALENCE_GAP.md` - JIT equivalence scaffolding
- `docs/status/T81LANG_TRACEABILITY_MATRIX.md` - Traceability matrix
- `docs/records/status-history/AXION_STATUS.md` - Update maturity evidence

### CI/Testing
- Add deterministic profile boundary gates
- Add map/hash determinism property tests
- Add JIT equivalence smoke tests
- Strengthen Axion evidence loop tests

---

This crosswalk will be updated as remediation progresses to track completion and proof artifacts.
