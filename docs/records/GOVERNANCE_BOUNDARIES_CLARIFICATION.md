# Governance Boundaries Clarification

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Governance Boundaries Clarification](#governance-boundaries-clarification)
  - [Executive Summary](#executive-summary)
  - [1. Deterministic Core Profile (DCP) Components](#1-deterministic-core-profile-dcp-components)
    - [1.1 Core DCP - Tier A (Strict Determinism)](#11-core-dcp---tier-a-strict-determinism)
    - [1.2 Conditional DCP - Tier B (Canonical Numeric)](#12-conditional-dcp---tier-b-canonical-numeric)
  - [2. Explicitly Excluded Components](#2-explicitly-excluded-components)
    - [2.1 Outside DCP - Non-Deterministic](#21-outside-dcp---non-deterministic)
    - [2.2 Experimental - Not DCP Guaranteed](#22-experimental---not-dcp-guaranteed)
  - [3. Boundary Enforcement Mechanisms](#3-boundary-enforcement-mechanisms)
    - [3.1 Compile-Time Guards](#31-compile-time-guards)
    - [3.2 Runtime Policy Enforcement](#32-runtime-policy-enforcement)
    - [3.3 API Documentation Boundaries](#33-api-documentation-boundaries)
  - [4. Specification Boundaries](#4-specification-boundaries)
    - [4.1 Normative DCP Specifications](#41-normative-dcp-specifications)
    - [4.2 Non-Normative Experimental Specifications](#42-non-normative-experimental-specifications)
  - [5. CI/CD Enforcement](#5-cicd-enforcement)
    - [5.1 DCP Compliance Gates](#51-dcp-compliance-gates)
- [.github/workflows/deterministic-core.yml](#githubworkflowsdeterministic-coreyml)
    - [5.2 Experimental Feature Gates](#52-experimental-feature-gates)
- [.github/workflows/experimental-features.yml](#githubworkflowsexperimental-featuresyml)
  - [6. Documentation Standards](#6-documentation-standards)
    - [6.1 Component Classification Tags](#61-component-classification-tags)
    - [6.2 API Boundary Markers](#62-api-boundary-markers)
  - [7. Migration Path Guidelines](#7-migration-path-guidelines)
    - [7.1 Experimental → Core Promotion Path](#71-experimental-→-core-promotion-path)
    - [7.2 Core → Experimental Demotion](#72-core-→-experimental-demotion)
  - [8. User Guidance](#8-user-guidance)
    - [8.1 DCP-Only Usage](#81-dcp-only-usage)
- [Compile with strict DCP enforcement](#compile-with-strict-dcp-enforcement)
- [Runtime with DCP validation](#runtime-with-dcp-validation)
    - [8.2 Experimental Usage](#82-experimental-usage)
- [Explicit experimental feature enablement](#explicit-experimental-feature-enablement)
- [Runtime with experimental features](#runtime-with-experimental-features)
    - [8.3 Boundary Violation Detection](#83-boundary-violation-detection)
  - [9. Compliance Verification](#9-compliance-verification)
    - [9.1 Automated Boundary Checking](#91-automated-boundary-checking)
- [scripts/verify_dcp_boundaries.py](#scriptsverify_dcp_boundariespy)
    - [9.2 Documentation Compliance](#92-documentation-compliance)
- [scripts/verify_boundary_documentation.py](#scriptsverify_boundary_documentationpy)
  - [10. Success Criteria](#10-success-criteria)
    - [10.1 Boundary Clarity Metrics](#101-boundary-clarity-metrics)
    - [10.2 Compliance Metrics](#102-compliance-metrics)
  - [11. Implementation Plan](#11-implementation-plan)
    - [Phase 1: Documentation (Week 1)](#phase-1-documentation-week-1)
    - [Phase 2: Enforcement (Week 2)](#phase-2-enforcement-week-2)
    - [Phase 3: User Guidance (Week 3)](#phase-3-user-guidance-week-3)
  - [12. Conclusion](#12-conclusion)

<!-- T81-TOC:END -->


**Generated:** 2026-03-06  
**Purpose:** Explicitly define and document the separation between "deterministic core" and "experimental/excluded" components  
**Status:** Normative - Immediate effect

---

## Executive Summary

This document provides crystal-clear boundaries between T81 Foundation components that are within the Deterministic Core Profile (DCP) and those that are explicitly excluded or experimental. It addresses audit findings about potential confusion between core guarantees and experimental features.

**Core Principle:** **Determinism by Default, Experimental by Exception**

---

## 1. Deterministic Core Profile (DCP) Components

### 1.1 Core DCP - Tier A (Strict Determinism)

| Component | Location | Determinism Level | Maturity | Guarantee |
|-----------|----------|-------------------|----------|-----------|
| **T81Int** | `include/t81/types/T81Int.hpp` | ✅ Bit-Exact | Stable | Cross-platform identical |
| **T81BigInt** | `include/t81/types/T81BigInt.hpp` | ✅ Bit-Exact | Stable | Cross-platform identical |
| **T81Fraction** | `include/t81/types/T81Fraction.hpp` | ✅ Bit-Exact | Stable | Cross-platform identical |
| **Basic T81Float** | `include/t81/types/T81Float.hpp` (add, mul, sub) | ✅ Bit-Exact | Stable | Cross-platform identical |
| **T81String** | `include/t81/types/T81String.hpp` | ✅ Canonical | Stable | UTF-8 normalized |
| **T81Symbol** | `include/t81/types/T81Symbol.hpp` | ✅ Canonical | Stable | Content-addressed |
| **T81List/Vector** | `include/t81/types/T81List.hpp` | ✅ Deterministic | Stable | Ordered iteration |
| **T81Map** | `include/t81/types/T81Map.hpp` | ✅ P2 Guaranteed | Stable | `iter_sorted()` only |
| **T81VM Core** | `core/vm/` | ✅ Deterministic | Stable | Instruction-level |
| **TISC Base** | `isa/` | ✅ Deterministic | Stable | Opcode semantics |

### 1.2 Conditional DCP - Tier B (Canonical Numeric)

| Component | Condition | Determinism Level | Guarantee |
|-----------|-----------|-------------------|-----------|
| **Advanced T81Float** | IEEE-754 host | ⚠️ Platform-dependent | Within epsilon |
| **Basic Transcendentals** | dmath available | ✅ Deterministic | Software-defined |
| **Division** | Native implementation | ❌ Rejected in Tier A | NaE in deterministic mode |

---

## 2. Explicitly Excluded Components

### 2.1 Outside DCP - Non-Deterministic

| Component | Location | Exclusion Reason | Status |
|-----------|----------|-----------------|--------|
| **Host Float Math** | `T81Float` (division, advanced transcendentals) | Platform-dependent | ❌ Explicitly rejected |
| **JIT Compilation** | `include/t81/jit/` | No equivalence proofs | ❌ Experimental |
| **Wall-Clock Access** | Various | Time-dependent | ❌ Forbidden in DCP |
| **Host Entropy** | Various | Non-deterministic | ❌ Forbidden in DCP |
| **Raw Pointers** | Various | Address observability | ❌ Forbidden in DCP |
| **System Handles** | Various | Host-dependent | ❌ Forbidden in DCP |

### 2.2 Experimental - Not DCP Guaranteed

| Component | Location | Experimental Status | Boundary |
|-----------|----------|-------------------|----------|
| **Cognitive Tiers** | `experimental/cog/` | Research phase | ❌ Outside core |
| **HanoiVM** | `experimental/hanoi/` | Alternative VM | ❌ Outside core |
| **Advanced AI Integration** | `examples/ai-integration/` | Demonstration | ❌ Outside core |
| **Distributed Computing** | `experimental/distributed/` | Research | ❌ Outside core |

---

## 3. Boundary Enforcement Mechanisms

### 3.1 Compile-Time Guards

```cpp
// Example: Deterministic profile enforcement
#if defined(T81_DETERMINISTIC)
    #ifdef T81_DETERMINISTIC_DMATH_AVAILABLE
        return core::detail::sin(*this);
    #else
        return nae();  // Explicit rejection
    #endif
#else
    return from_double(std::sin(to_double()));  // Host fallback
#endif
```

### 3.2 Runtime Policy Enforcement

```cpp
// Example: Axion policy enforcement
if (program.requires_tier > current_tier) {
    axion_violation("tier_violation", 
                   "Attempted Tier " + std::to_string(program.requires_tier) + 
                   " in Tier " + std::to_string(current_tier) + " context");
    return SecurityFault;
}
```

### 3.3 API Documentation Boundaries

```cpp
/**
 * @brief JIT compilation trace (EXPERIMENTAL)
 * @warning This component is EXPERIMENTAL and NOT part of the Deterministic Core Profile.
 *          No determinism guarantees are provided. Use for research only.
 */
class JitTrace {
    // ...
};
```

---

## 4. Specification Boundaries

### 4.1 Normative DCP Specifications

| Spec | Status | DCP Coverage |
|------|--------|--------------|
| `spec/determinism-profile.md` | ✅ Normative | Complete DCP definition |
| `spec/t81-data-types.md` | ✅ Normative | Core types only |
| `spec/tisc-spec.md` | ✅ Normative | Base instruction set |
| `spec/t81vm-spec.md` | ✅ Normative | Core VM semantics |

### 4.2 Non-Normative Experimental Specifications

| Spec | Status | DCP Relevance |
|------|--------|---------------|
| `spec/cognitive-tiers.md` | 📋 Draft | ❌ Outside DCP |
| `docs/proposals/` | 💭 Proposal | ❌ Outside DCP |
| `examples/` | 📚 Example | ❌ Outside DCP |

---

## 5. CI/CD Enforcement

### 5.1 DCP Compliance Gates

```yaml
# .github/workflows/deterministic-core.yml
name: Deterministic Core Profile Compliance
on: [push, pull_request]

jobs:
  dcp-compliance:
    runs-on: ubuntu-latest
    steps:
      - name: Test DCP Components
        run: |
          ./test_dcp_components.sh
      - name: Verify Experimental Exclusions
        run: |
          ./verify_experimental_boundaries.sh
      - name: Cross-Platform Determinism
        strategy:
          matrix:
            os: [ubuntu-latest, macos-latest]
        run: |
          ./test_cross_platform_determinism.sh
```

### 5.2 Experimental Feature Gates

```yaml
# .github/workflows/experimental-features.yml
name: Experimental Features (Optional)
on: [workflow_dispatch]

jobs:
  experimental-tests:
    if: github.event_name == 'workflow_dispatch'
    runs-on: ubuntu-latest
    steps:
      - name: Test JIT (Experimental)
        run: |
          ./test_experimental_jit.sh
      - name: Test Cognitive Tiers (Experimental)
        run: |
          ./test_experimental_cognitive.sh
```

---

## 6. Documentation Standards

### 6.1 Component Classification Tags

All components must be clearly classified:

```cpp
/**
 * @file T81Float.hpp
 * @brief Balanced ternary floating-point (PARTIAL DCP)
 * 
 * DCP Status:
 * - ✅ Addition, subtraction, multiplication: FULL DCP
 * - ⚠️ Division: REJECTED in DCP mode (returns NaE)
 * - ❌ Advanced transcendentals: NO DCP guarantee
 * - ❌ Host fallback: NON-DETERMINISTIC
 */
```

### 6.2 API Boundary Markers

```cpp
// ==== DCP BOUNDARY ====
// Above this line: Deterministic Core Profile components
// Below this line: Experimental or excluded components

// ==== EXPERIMENTAL BOUNDARY ====
// Above this line: Core components
// Below this line: Experimental features (no DCP guarantees)
```

---

## 7. Migration Path Guidelines

### 7.1 Experimental → Core Promotion Path

**NOT AUTOMATIC** - Requires explicit governance approval:

1. **Equivalence Proof:** Complete deterministic equivalence proofs
2. **Test Coverage:** 100% DCP compliance test suite
3. **Specification Update:** Update normative specifications
4. **Governance Review:** Formal approval process
5. **Documentation Update:** Update all boundary markings

### 7.2 Core → Experimental Demotion

**RARE** - Only for critical issues:

1. **Issue Discovery:** Critical determinism violation found
2. **Impact Assessment:** Evaluate scope of problem
3. **Governance Decision:** Formal demotion approval
4. **Boundary Update:** Update all documentation
5. **User Communication:** Clear migration guidance

---

## 8. User Guidance

### 8.1 DCP-Only Usage

```bash
# Compile with strict DCP enforcement
./t81_build --mode=deterministic --strict-dcp

# Runtime with DCP validation
./t81_run --dcp-enforce --reject-experimental
```

### 8.2 Experimental Usage

```bash
# Explicit experimental feature enablement
./t81_build --enable-experimental=jit,cognitive

# Runtime with experimental features
./t81_run --allow-experimental --experimental-features=jit
```

### 8.3 Boundary Violation Detection

```cpp
// Runtime boundary enforcement
if (T81_DETERMINISTIC && !component_is_dcp()) {
    throw DCPViolation("Attempted to use non-DCP component in deterministic mode");
}
```

---

## 9. Compliance Verification

### 9.1 Automated Boundary Checking

```python
# scripts/verify_dcp_boundaries.py
def verify_dcp_boundaries():
    """Verify no experimental components are used in DCP context"""
    violations = []
    
    # Check include guards
    for file in dcp_components:
        if has_experimental_includes(file):
            violations.append(f"Experimental include in DCP component: {file}")
    
    # Check API calls
    for file in dcp_components:
        if calls_experimental_apis(file):
            violations.append(f"Experimental API call in DCP component: {file}")
    
    return violations
```

### 9.2 Documentation Compliance

```python
# scripts/verify_boundary_documentation.py
def verify_boundary_documentation():
    """Verify all components have proper boundary documentation"""
    missing_docs = []
    
    for component in all_components:
        if not has_dcp_classification(component):
            missing_docs.append(f"Missing DCP classification: {component}")
    
    return missing_docs
```

---

## 10. Success Criteria

### 10.1 Boundary Clarity Metrics

| Metric | Target | Current |
|--------|--------|---------|
| Components Classified | 100% | 0% |
| API Boundaries Documented | 100% | 0% |
| CI Enforcement Active | ✅ | ❌ |
| User Guidance Complete | 100% | 0% |

### 10.2 Compliance Metrics

| Metric | Target | Current |
|--------|--------|---------|
| DCP Violation Detection | ✅ | ❌ |
| Experimental Isolation | ✅ | ❌ |
| Cross-Boundary Calls | 0 | Unknown |
| Documentation Accuracy | 100% | 0% |

---

## 11. Implementation Plan

### Phase 1: Documentation (Week 1)
- [ ] Add DCP classification to all core components
- [ ] Add experimental warnings to excluded components
- [ ] Create boundary markers in all header files
- [ ] Update API documentation

### Phase 2: Enforcement (Week 2)
- [ ] Implement compile-time DCP guards
- [ ] Add runtime boundary violation detection
- [ ] Create CI compliance gates
- [ ] Implement automated boundary checking

### Phase 3: User Guidance (Week 3)
- [ ] Create DCP usage guidelines
- [ ] Add experimental feature documentation
- [ ] Implement boundary verification tools
- [ ] Create migration documentation

---

## 12. Conclusion

This governance boundaries clarification establishes a clear, enforceable separation between deterministic core components and experimental features. The boundaries are:

1. **Explicitly Documented** - Every component has clear classification
2. **Technically Enforced** - Compile-time and runtime guards
3. **Automatically Verified** - CI compliance checking
4. **User-Guided** - Clear usage patterns and warnings

**Result:** Users can confidently rely on DCP guarantees while safely exploring experimental features without risk of confusion or accidental boundary violations.

---

*This document is normative and takes immediate effect. All new components must comply with these boundary requirements.*
