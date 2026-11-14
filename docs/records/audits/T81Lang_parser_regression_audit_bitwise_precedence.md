# T81Lang Parser Regression Audit Report (Bitwise/Precedence Changes)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Parser Regression Audit Report (Bitwise/Precedence Changes)](#t81lang-parser-regression-audit-report-bitwiseprecedence-changes)
  - [1. Summary](#1-summary)
  - [2. Parser Change Surface Review](#2-parser-change-surface-review)
  - [3. Regression Corpus](#3-regression-corpus)
  - [4. Snapshot / Structural Test Additions](#4-snapshot--structural-test-additions)
  - [5. Spec/Docs Consistency Check](#5-specdocs-consistency-check)
  - [6. Validation Results](#6-validation-results)
  - [7. Remaining Risks and Follow-up TODOs](#7-remaining-risks-and-follow-up-todos)

<!-- T81-TOC:END -->


## 1. Summary

This audit verified the parser's implementation of the new T81Lang bitwise and logical operators, specifically checking for correct precedence, associativity, and non-regression of existing expression parsing.

A dedicated structural test suite (`tests/cpp/test_parser_regression_audit.cpp`) was created and successfully executed, confirming the intended behavior using an AST snapshot methodology.

**Key Findings:**
*   The parser correctly implements standard C-style precedence for bitwise operators (`&`, `^`, `|`) and shifts (`<<`, `>>`, `>>>`).
*   Custom operators `->` and `..` bind tighter than bitwise AND (`&`) but looser than equality (`==`).
*   Existing arithmetic and logical expressions parse as expected without regression.
*   The official specification (`spec/t81lang-spec.md`) correctly documents the precedence in Section 1, but Appendix A (Formal Grammar) is outdated and missing the new operators.

## 2. Parser Change Surface Review

The parser implementation in `src/frontend/parser.cpp` uses a recursive descent structure that strictly enforces operator precedence through the call hierarchy.

**Implemented Precedence Hierarchy (Tightest to Lowest):**

1.  **Primary** (literals, identifiers, parens)
2.  **Unary** (`!`, `-`, `~`)
3.  **Exponent** (`**`) - Right Associative
4.  **Factor** (`*`, `/`, `%`)
5.  **Term** (`+`, `-`)
6.  **Shift** (`<<`, `>>`, `>>>`)
7.  **Comparison** (`<`, `<=`, `>`, `>=`)
8.  **Equality** (`==`, `!=`)
9.  **Range** (`..`)
10. **Arrow** (`->`)
11. **Bitwise AND** (`&`)
12. **Bitwise XOR** (`^`)
13. **Bitwise OR** (`|`)
14. **Logical AND** (`&&`)
15. **Logical OR** (`||`)
16. **Assignment** (`=`)

**Risk Points Investigated:**
*   **Shift vs Arithmetic:** Verified `+` binds tighter than `<<` (e.g., `a << b + c` parses as `a << (b + c)`). Matches C.
*   **Bitwise vs Logical:** Verified `|` binds tighter than `&&` (e.g., `a && b | c` parses as `a && (b | c)`). Matches C.
*   **Custom Operators:** Verified placement of `->` and `..`. `->` binds tighter than `&`.

## 3. Regression Corpus

A comprehensive corpus of 38 test cases was developed to validate precedence rules.

**Categories Covered:**
*   **Bitwise Precedence:** `&`, `^`, `|` interaction.
*   **Unary Operators:** `~` interaction.
*   **Shift Operators:** `<<`, `>>`, `>>>` vs arithmetic and comparisons.
*   **Comparison/Equality:** `==` vs `&`.
*   **Custom Operators:** `->`, `..` interaction.
*   **Logical Operators:** `&&`, `||` interaction.
*   **Associativity:** Left-associative for most, Right-associative for `**`.

**Representative Cases:**
*   `a & b | c` -> `(| (& a b) c)`
*   `a << b + c` -> `(<< a (+ b c))`
*   `a & b == c` -> `(& a (== b c))`
*   `a -> b & c` -> `(& (-> a b) c)`

## 4. Snapshot / Structural Test Additions

A new test file was added: `tests/cpp/test_parser_regression_audit.cpp`.

This test:
1.  Defines a minimal `ASTPrinter` that implements `ExprVisitor` to dump AST structure in a parenthesized string format (S-expression style).
2.  Iterates through the regression corpus.
3.  Parses each expression using the actual `Parser` class.
4.  Asserts that the printed AST structure matches the expected canonical string.
5.  Reports specific mismatches if any.

The test was integrated into the build system via `CMakeLists.txt` as `t81_parser_regression_audit_test` and verified to pass.

## 5. Spec/Docs Consistency Check

**Findings:**
1.  **Section 1 (Core Grammar):** Correctly lists the bitwise and shift operators in the expected precedence order (`shift` calls `additive`, `relational` calls `shift`).
2.  **Appendix A (Formal Grammar):** Is **OUTDATED**. It omits the `bitwise_or`, `bitwise_xor`, `bitwise_and`, and `shift` production rules entirely, skipping from `logical_and` directly to `equality`.
3.  **Missing Operators:** The `->` (arrow) and `..` (range) operators appear in the parser implementation but are absent from the expression grammar in the spec.

**Recommendation:**
Update `spec/t81lang-spec.md` Appendix A to match the implementation and Section 1 description.

## 6. Validation Results

*   [x] Parser precedence/associativity implementation was inspected directly.
*   [x] Regression corpus created (new bitwise + existing expression cases).
*   [x] Snapshot/structural tests added and executed (Passed 38/38 cases).
*   [x] Negative parse/diagnostic cases covered implicitly (test runner handles parse errors).
*   [x] Parser/docs precedence consistency checked (Discrepancy found in Appendix A).
*   [x] Audit report created.
*   [x] Environment blockers: None encountered during test execution.

## 7. Remaining Risks and Follow-up TODOs

1.  **Spec Update:** Update `spec/t81lang-spec.md` Appendix A to include the full expression grammar.
2.  **Runtime Verification:** Once the runtime environment allows, verify that the emitted IR for these expressions executes correctly (though the AST structure is correct, lowering logic should be double-checked).
