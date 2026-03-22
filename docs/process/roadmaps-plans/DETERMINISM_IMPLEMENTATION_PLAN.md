# T81 Strict Determinism Profile — Minimal Implementation Delta Plan

This plan outlines the minimal set of changes required to enforce the **Strict Determinism Profile (Tier A)** defined in `spec/determinism-profile.md`.

## 1. Core Numerics (T81Float Guardrails)

**Objective:** Prevent inadvertent use of host-dependent floating-point operations in strict mode.

*   **Files:**
    *   `include/t81/types/t81_float.hpp`
    *   `core/types/t81_float.cpp`
*   **Change Scope:**
    *   Introduce `bool strict_mode = false;` thread-local or context flag.
    *   In `operator/`, `sin`, `cos`, `tan`, `log`, `exp`, `sqrt`:
        *   If `strict_mode` is true AND `dmath` (software backend) is NOT active:
        *   Throw `Trap(FLOAT_OP_FORBIDDEN)`.
    *   Ensure `+`, `-`, `*` always use the software integer backend (already compliant).
*   **Risk:** Medium. Existing tests relying on `double` fallback will fail if strict mode is enabled by default.
*   **Effort:** Low.

## 2. VM Trap & Fault Semantics

**Objective:** Implement the new deterministic trap taxonomy.

*   **Files:**
    *   `include/t81/vm/trap.hpp` (or `fault.hpp`)
    *   `vm/vm.cpp`
*   **Change Scope:**
    *   Define new trap codes:
        *   `Trap::FloatOpForbidden`
        *   `Trap::SymbolIdForbidden`
        *   `Trap::EntropyViolation`
        *   `Trap::TimeViolation`
    *   Update `VM::handle_trap` to emit the canonical Axion event reasons defined in the spec.
*   **Risk:** Low. Additive change.
*   **Effort:** Low.

## 3. Axion Policy Enforcement

**Objective:** Add the policy switch to enable Strict Mode.

*   **Files:**
    *   `include/t81/axion/policy.hpp`
    *   `kernel/axion/policy.cpp`
*   **Change Scope:**
    *   Add field `bool require_strict_determinism` to `Policy` struct.
    *   In `Policy::validate_op`, check against forbidden operations (e.g., `Opcode::TIME`, `Opcode::RANDOM`) if strict mode is set.
    *   Propagate strict mode setting to Core Numerics context.
*   **Risk:** Medium. Requires plumbing strict mode state through the execution stack.
*   **Effort:** Medium.

## 4. Symbol Identity Stabilization

**Objective:** Enforce content-addressing and hide runtime IDs.

*   **Files:**
    *   `include/t81/types/symbol.hpp`
    *   `core/types/symbol.cpp`
*   **Change Scope:**
    *   Modify `Symbol::operator==` to check `hash_` instead of `id_` when strict mode is active.
    *   Modify `Symbol::id()` to trap or return a dummy value in strict mode (to prevent ID dependency).
    *   Ensure `Symbol` serialization writes the string content/hash, not the runtime ID.
*   **Risk:** High. Performance impact on equality checks; potential breakage of ID-based optimizations.
*   **Effort:** Medium.

## 5. Error Determinism

**Objective:** Convert errors to canonical value types.

*   **Files:**
    *   `include/t81/types/error.hpp`
    *   `core/types/error.cpp`
*   **Change Scope:**
    *   Ensure `T81Error` (or equivalent) does not capture `std::chrono::system_clock` or thread IDs in its constructor.
    *   Remove `std::exception` inheritance if it leaks host vtable pointers or non-deterministic `what()` strings.
    *   Implement `to_canonical_string()` for errors.
*   **Risk:** Medium. ABI change for error handling.
*   **Effort:** Medium.

## 6. Verification & CI

**Objective:** Gate strict compliance.

*   **Files:**
    *   `tests/cpp/strict_profile_test.cpp` (New)
    *   `CMakeLists.txt`
*   **Change Scope:**
    *   Add a new test suite that:
        *   Enables strict mode.
        *   Attempts forbidden ops (time, float-div, random).
        *   Asserts that they Trap deterministically.
        *   Asserts that Symbols equality works by content.
*   **Risk:** Low. Test-only.
*   **Effort:** Low.

## Summary

*   **Total Files:** ~12
*   **Total Risk:** Medium (Core semantic shifts in strict mode).
*   **Total Effort:** Medium (2-3 days engineering).
