# RFC-0029: T81Lang Feature Registry & Drift Prevention

Version 0.2 — Standards Track
Status: Accepted
Updated: 2026-03-15
Author: T81 Foundation Architecture Team
Applies to: T81Lang Specification, Frontend Compiler

______________________________________________________________________

# Summary

This RFC introduces a formal strategy for gating experimental features within the T81Lang frontend to reconcile the implementation with the formal specification. It establishes a `spec/t81lang_features.md` registry, explicit profile-aware diagnostics in the Semantic Analyzer, and automated specification conformance tests to permanently resolve specification drift.

# Motivation

The T81Lang frontend is currently classified as "Beta / Draft Spec" status. It contains experimental features (e.g., keywords like `recurse`, `distributed`) and experimental types (`T81Promise`, `T81Agent`) that represent implementation drift from the frozen core specifications. Without strict gating, these experimental constructs risk polluting the core AST/IR generation paths when building under the standard Deterministic Core Profile (DCP), violating the strict separation of stable and experimental surfaces.

# Proposal

### 1. Language Feature Registry
A formal registry, `spec/t81lang_features.md`, must be established as the single source of truth for feature maturity.
Example structure:
| Feature     | Status       | Spec Section | Enabled In        |
| ----------- | ------------ | ------------ | ----------------- |
| `recurse`     | Experimental | RFC-0032     | cognitive profile |
| `distributed` | Experimental | RFC-0033     | experimental      |
| `reflect`     | Experimental | RFC-0034     | research          |

### 2. Isolation Strategy for Experimental Features
A strict Lexical/AST gating mechanism must be implemented, governed by C++23 compiler directives and runtime profile flags linked to the Feature Registry.
*   **Lexical Gating:** The Lexer must condition the tokenization of experimental keywords. These should be isolated via a build-time macro (`#ifdef T81_EXPERIMENTAL_COGNITIVE`). In standard DCP builds, the lexer will reject them or treat them as standard identifiers.
*   **AST Pruning:** In the parser and IR generator, the instantiation of experimental types (`T81Promise`, `T81Agent`, `T81Symbolic`) must be wrapped in identical preprocessor directives, isolating them to `/experiments/ai/` contexts or explicit opt-in builds.

### 3. Semantic Analyzer Updates
The Semantic Analyzer must enforce profile awareness to guarantee graceful degradation and prevent undefined behavior.
*   **Profile-Aware Diagnostics:** If the frontend encounters a legacy code construct that attempts to utilize an experimental feature (e.g., `T81Agent`) while in standard DCP mode, the `SemanticAnalyzer` must safely trap the unresolvable type symbol.
*   **Deterministic Fault Emission:** The Semantic Analyzer must emit a specific diagnostic error: `CompilerError::FeatureNotAvailableInProfile` during analysis, halting compilation deterministically rather than falling back to polyfills.
*   **VM Safeguard:** The `core/vm/vm.cpp` main dispatch loop must natively trap any experimental instructions that bypass frontend checks, immediately emitting a `DecodeFault` (as defined in `spec/t81lang-spec.md`) and halting execution.

### 4. Spec-as-Executable Tests and IR Normalization Gate
*   **Spec-as-Executable:** Introduce automated checks via `tests/spec/t81lang_feature_matrix_test.cpp` to continuously confirm that the active implementation aligns exactly with the Feature Registry.
*   **IR Normalization Gate:** Add a CI job, `spec_ir_conformance_check`, to compile standard T81Lang source to an AST, lower to IR, and emit canonical serialization. The resultant hash must be compared against locked spec fixtures to automatically detect regression or drift.

# Impact

## Backward Compatibility
Existing experimental code will fail to compile under the default DCP profile, generating a clean, actionable diagnostic error. Valid DCP code remains unaffected.

## Performance
Negligible overhead in the lexer and parser. Zero overhead at runtime.

## Security
Prevents unauthorized experimental opcodes from executing within the secure Axion boundaries.

# Alternatives Considered

*   **Runtime Feature Flags Only:** Using only runtime checks delays failure to execution time, which violates the fail-fast principle and allows experimental code to enter the compiler pipeline. Strict compile-time gating is required.

# References

* RFC-0001: Architecture Principles
* `docs/status/DRIFT_DECOMPOSITION.md`

______________________________________________________________________

## Acceptance Note

**Status advanced `Draft → Accepted` on 2026-03-15.**

The core requirements of this RFC are satisfied by the existing implementation:

| Criterion | Evidence |
| :--- | :--- |
| [A-0029-01] Feature registry as single source of truth | `kBuiltinTable` in `include/t81/frontend/builtin_registry.hpp` — authoritative per-feature record with `min_tier`, `is_effect_surface`, `ir_class`, and opcode; human-readable projection at `spec/t81lang_features.md` |
| [A-0029-02] Lexical/SA tier gating for experimental features | `minimum_tier_for_call_surface()` reads `min_tier` from `kBuiltinTable`; `collect_tier_violations()` + `enforce_active_tier_minimum()` (SA ~line 2047) emit compile-time errors for tier violations |
| [A-0029-03] Distributed builtins blocked in DCP builds | All `std.distributed.*` builtins carry `min_tier = {4}`; a standard DCP build without `@tier(4)` annotations cannot reach them |
| [A-0029-04] Effect surface blocking in `@pure` / Tier ≤ 1 | `is_effect_surface_call()` enforced at SA call-resolution; `@pure` functions and Tier-1-or-lower contexts emit an error on any effect surface call |
| [A-0029-05] VM safeguard | VM dispatch emits `DecodeFault` for unrecognised or profile-disabled opcodes, halting execution deterministically |
| [A-0029-06] Formal feature registry document | `spec/t81lang_features.md` created 2026-03-15 |

The `t81lang_feature_matrix_test.cpp` CI job specified in §4 is noted as
a future hardening item; tier-gate enforcement is already covered by the
36 T81Lang frontend conformance tests (`t81_lang_frontend_test`).
