# T81Lang Bitwise Follow-up Spec and IR Audit Report

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Bitwise Follow-up Spec and IR Audit Report](#t81lang-bitwise-follow-up-spec-and-ir-audit-report)
  - [1. Summary](#1-summary)
  - [2. Parser-to-Spec Mismatch Review](#2-parser-to-spec-mismatch-review)
  - [3. `spec/t81lang-spec.md` Updates](#3-`spect81lang-specmd`-updates)
  - [4. Targeted IR Snapshot / Lowering Audit](#4-targeted-ir-snapshot--lowering-audit)
  - [5. Negative Parse Coverage Status](#5-negative-parse-coverage-status)
  - [6. Validation Results](#6-validation-results)
  - [7. Remaining Gaps and Next Steps](#7-remaining-gaps-and-next-steps)

<!-- T81-TOC:END -->


## 1. Summary

This audit completes the stabilization of the T81Lang bitwise/shift operator integration. It synchronizes the formal grammar specification with the implemented parser and verifies the lowering structure of precedence-sensitive expressions via targeted IR snapshot tests.

**Key Achievements:**
*   **Spec Synchronization:** `spec/t81lang-spec.md` Appendix A (Formal Grammar) was updated to match the actual recursive descent parser implementation.
*   **IR Verification:** A new test harness `tests/cpp/test_ir_snapshot_audit.cpp` was created to verify that operator precedence is preserved during lowering to TISC IR.
*   **Bug Fix:** Identified and fixed a lowering issue in `IRGenerator` where implicit `Boolean` to `Integer` conversion was missing for bitwise operations on comparison results.
*   **Gap Identification:** Confirmed that logical operators (`&&`, `||`) are parsed correctly but are **not yet implemented** in the `IRGenerator` lowering phase.

## 2. Parser-to-Spec Mismatch Review

Prior to this audit, `spec/t81lang-spec.md` Appendix A was significantly outdated compared to `src/frontend/parser.cpp`.

**Detected Mismatches:**
*   **Missing Productions:** Bitwise (`&`, `|`, `^`), Shift (`<<`, `>>`, `>>>`), Custom (`->`, `..`), and Logical (`&&`, `||`) operators were absent from the expression grammar.
*   **Precedence Hierarchy:** The spec did not reflect the implemented precedence chain (e.g., `bitwise_and` vs `equality`).
*   **Missing Declarations:** `record`, `enum`, and `type` declarations were missing from the top-level grammar.
*   **Missing Expressions:** `match`, `block`, and `if` expressions were missing from `primary`.

**Confirmed Precedence Chain (Tightest to Loosest):**
1.  Primary / Unary (including `!`, `-`, `~`)
2.  Exponent (`**`)
3.  Factor (`*`, `/`, `%`)
4.  Term (`+`, `-`)
5.  Shift (`<<`, `>>`, `>>>`)
6.  Comparison (`<`, `<=`, `>`, `>=`)
7.  Equality (`==`, `!=`)
8.  Range (`..`)
9.  Arrow (`->`)
10. Bitwise AND (`&`)
11. Bitwise XOR (`^`)
12. Bitwise OR (`|`)
13. Logical AND (`&&`)
14. Logical OR (`||`)
15. Assignment (`=`)

## 3. `spec/t81lang-spec.md` Updates

Appendix A was patched to include:
*   Full hierarchy of binary expressions matching the parser.
*   Operator tokens: `~`, `<<`, `>>`, `>>>`, `&`, `|`, `^`, `->`, `..`.
*   `match` expression grammar.
*   `record` and `enum` declaration grammar.
*   Clarified `unary` and `primary` productions to include `call` and block-like expressions.

## 4. Targeted IR Snapshot / Lowering Audit

A new test executable `t81_ir_snapshot_audit_test` was added to `tests/cpp/` and registered in `CMakeLists.txt`.

**Test Strategy:**
The test parses T81Lang source snippets, runs semantic analysis, generates IR, and asserts the relative ordering of TISC IR opcodes to verify that precedence was respected during lowering.

**Results:**

| Case | Expression | Expected IR Sequence | Result |
| :--- | :--- | :--- | :--- |
| **Bitwise vs Equality** | `a & b == c` | `CMP` before `BITAND` | **PASS** |
| **Shift vs Arithmetic** | `a << b + c` | `ADD` before `BITSHL` | **PASS** |
| **Unary vs Arithmetic** | `~a + 1` | `BITNOT` before `ADD` | **PASS** |
| **Comparison vs Shift** | `a < b << c` | `BITSHL` before `CMP` | **PASS** |
| **Right Shifts** | `a >> b` / `a >>> b` | `BITSHR` / `BITUSHR` | **PASS** |
| **Logical vs Bitwise** | `a \| b && c` | `BITOR` vs `JZ` | **SKIPPED** (Gap Found) |

**Note on Logical Operators:**
The test case for `a | b && c` was skipped because the `IRGenerator` does not currently implement lowering for `&&` and `||`. This is a known runtime gap, although parsing is correct.

## 5. Negative Parse Coverage Status

Explicit negative parse tests (e.g., ensuring `a >>>` fails to parse) were **deferred**.
Rationale: The primary focus was on establishing positive confirmation of lowering structure and syncing the spec. The existing `test_parser_regression_audit` provides some implicit coverage, and `IRGenerator` tests handle semantic errors.
**Action Item:** Add `tests/cpp/test_parser_negative.cpp` in a future task.

## 6. Validation Results

*   [x] `spec/t81lang-spec.md` Appendix A updated to include shift + bitwise productions and operators
*   [x] `~` and `>>>` are represented in the formal grammar
*   [x] `->` and `..` expression grammar coverage checked and documented
*   [x] Targeted IR snapshot/lowering tests added (`tests/cpp/test_ir_snapshot_audit.cpp`)
*   [x] IR audit corpus includes `>>` and `>>>` distinction
*   [x] Executed vs unexecuted tests are clearly separated (Logical tests skipped)
*   [x] Follow-up audit report created
*   [x] Any remaining negative-parse explicit coverage gaps documented

## 7. Remaining Gaps and Next Steps

1.  **Implement Logical Operators in IR:** `IRGenerator` needs to support `BinaryExpr` with `TokenType::AmpAmp` and `PipePipe` by emitting control flow (`JZ`/`JNZ`).
2.  **Runtime Semantics Verification:** Once IR support exists, end-to-end execution tests (running the generated bytecode in `T81VM`) should be added for logical operators.
3.  **Negative Testing:** Implement a dedicated suite for syntax errors to ensure robust error reporting.
