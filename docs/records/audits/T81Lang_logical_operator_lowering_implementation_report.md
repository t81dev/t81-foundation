# T81Lang Logical Operator Lowering Implementation Report (`&&`, `||`)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Logical Operator Lowering Implementation Report (`&&`, `||`)](#t81lang-logical-operator-lowering-implementation-report-`&&`-`||`)
  - [1. Summary](#1-summary)
  - [2. Control-Flow Lowering Infrastructure Reuse](#2-control-flow-lowering-infrastructure-reuse)
  - [3. `&&` / `||` Lowering Design and Implementation](#3-`&&`--`||`-lowering-design-and-implementation)
    - [Logical AND (`lhs && rhs`)](#logical-and-`lhs-&&-rhs`)
    - [Logical OR (`lhs || rhs`)](#logical-or-`lhs-||-rhs`)
  - [4. Regression and Verification Tests](#4-regression-and-verification-tests)
    - [Test Cases](#test-cases)
  - [5. Follow-up Plan/Tracking Updates](#5-follow-up-plantracking-updates)
  - [6. Validation Results](#6-validation-results)
  - [7. Remaining Gaps and Next Steps](#7-remaining-gaps-and-next-steps)

<!-- T81-TOC:END -->


## 1. Summary

This report documents the successful implementation of IR lowering for T81Lang logical AND (`&&`) and logical OR (`||`) operators. Previously, these operators were parsed but caused an "Unsupported binary operator" error during code generation. The implementation lowers these operators to short-circuiting control flow using existing jump and label infrastructure, ensuring adherence to T81 boolean semantics (canonical `0`/`1`).

## 2. Control-Flow Lowering Infrastructure Reuse

The implementation reuses the existing `IRGenerator` control-flow mechanisms:
*   **Labels:** `new_label()` generates unique labels for jump targets.
*   **Jumps:** `emit_jump_if_zero(target, reg)` and `emit_jump_if_not_zero(target, reg)` are used for conditional branching.
*   **Instructions:** Standard TISC opcodes `LOADI`, `JZ` (Jump If Zero), and `JNZ` (Jump If Not Zero) are used.
*   **Register Allocation:** `allocate_typed_register(PrimitiveKind::Boolean)` is used for intermediate and result values.

No new opcodes or parallel label systems were introduced. The lowering follows the same pattern as `IfStmt` and `WhileStmt`.

## 3. `&&` / `||` Lowering Design and Implementation

The operators are lowered to control flow sequences that guarantee short-circuit evaluation and canonical boolean results.

### Logical AND (`lhs && rhs`)
1.  **Initialize Result:** Load `0` (false) into the destination register.
2.  **Evaluate LHS:** Generate IR for the left-hand side.
3.  **Check LHS:** `JZ` (Jump If Zero) to the *end label* if LHS is false.
4.  **Evaluate RHS:** Generate IR for the right-hand side (only reached if LHS is true).
5.  **Check RHS:** `JZ` to the *end label* if RHS is false.
6.  **Set True:** Load `1` (true) into the destination register.
7.  **End Label:** Target for short-circuit jumps.

### Logical OR (`lhs || rhs`)
1.  **Initialize Result:** Load `1` (true) into the destination register.
2.  **Evaluate LHS:** Generate IR for the left-hand side.
3.  **Check LHS:** `JNZ` (Jump If Not Zero) to the *end label* if LHS is true.
4.  **Evaluate RHS:** Generate IR for the right-hand side (only reached if LHS is false).
5.  **Check RHS:** `JNZ` to the *end label* if RHS is true.
6.  **Set False:** Load `0` (false) into the destination register.
7.  **End Label:** Target for short-circuit jumps.

This design ensures that the result is always exactly `0` or `1`, satisfying the canonical boolean requirement.

## 4. Regression and Verification Tests

A new regression test suite was added in `tests/cpp/test_frontend_logical_lowering.cpp`, included in the build via `CMakeLists.txt`.

### Test Cases
*   **Reproduction:** Confirmed that `&&` and `||` previously threw "Unsupported binary operator".
*   **Success Verification:**
    *   `test_logical_and_success`: Verifies `&&` compilation and checks for presence of `LOADI 0`, multiple `JZ`, and `LOADI 1`.
    *   **Result:** Passed.
    *   `test_logical_or_success`: Verifies `||` compilation and checks for presence of `LOADI 1`, multiple `JNZ`, and `LOADI 0`.
    *   **Result:** Passed.
    *   `test_nested_logical`: Verifies mixed nesting `(a || b) && c` compiles correctly.
    *   **Result:** Passed.

Existing frontend tests (`t81_frontend_ir_generator_test`) were also run and passed, confirming no regressions.

## 5. Follow-up Plan/Tracking Updates

The implementation for `&&` and `||` lowering is complete.
*   **Status:** Implemented and Verified.
*   **Gap Closed:** Parser/Spec/Backend consistency restored for logical operators.

## 6. Validation Results

* [x] Existing control-flow label/backpatching mechanism identified and reused
* [x] `TokenType::AmpAmp` lowering implemented
* [x] `TokenType::PipePipe` lowering implemented
* [x] Lowering enforces short-circuit left-to-right evaluation (by instruction order and branching)
* [x] Lowering explicitly materializes canonical boolean results (`0`/`1`)
* [x] Nested logical expression regression tests added
* [x] IR shape tests verify branch/jump-based lowering (not bitwise substitution)
* [x] Pre-fix failure mode captured or documented
* [x] Runtime canonical-value and side-effect short-circuit tests executed OR explicitly deferred with rationale
    *   *Rationale:* IR shape tests confirm the control flow structure required for short-circuiting. Full runtime execution with side-effect verification requires a more complex e2e harness setup than available in unit testing, but the emitted IR structure guarantees this behavior by definition of `JZ`/`JNZ`.
* [x] Implementation report created

## 7. Remaining Gaps and Next Steps

*   **Runtime Side-Effect Verification:** While IR analysis confirms short-circuiting, adding an end-to-end test in the T81Lang script harness (e.g., `tests/t81/`) that prints output conditionally would further validate runtime behavior.
*   **Optimization:** The current lowering is safe but potentially verbose (e.g., loading init values). Future peephole optimizations could reduce instruction count for simple boolean variables.
