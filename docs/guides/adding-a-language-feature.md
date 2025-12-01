---
layout: page
title: "Guide: Adding a Language Feature"
---

# Guide: Adding a Feature to T81Lang

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Guide: Adding a Feature to T81Lang](#guide-adding-a-feature-to-t81lang)
  - [1. Frontend Architecture Overview](#1-frontend-architecture-overview)
  - [2. Step 1: Update the Lexer](#2-step-1-update-the-lexer)
    - [2.1 Add the Token Type](#21-add-the-token-type)
    - [2.2 Recognize the Lexeme](#22-recognize-the-lexeme)
  - [3. Step 2: Update the Parser](#3-step-2-update-the-parser)
    - [3.1 Update the Grammar Rule](#31-update-the-grammar-rule)
  - [4. Step 3: Update the IR Generator](#4-step-3-update-the-ir-generator)
    - [4.1 Implement the Visitor Logic](#41-implement-the-visitor-logic)
  - [5. Step 4: Write an End-to-End Test](#5-step-4-write-an-end-to-end-test)

<!-- T81-TOC:END -->








































This guide provides a step-by-step walkthrough for adding a new feature to the T81Lang language. We will use the example of adding a new binary operator, the modulo operator (`%`), to illustrate the process.

**Companion Documents:**
- **Specification:** [`spec/t81lang-spec.md`](../../spec/t81lang-spec.md)
- **Architecture:** [`ARCHITECTURE.md`](../../ARCHITECTURE.md)
- **Key Source Files:**
    - `include/t81/frontend/lexer.hpp`, `parser.hpp`, `ir_generator.hpp`
    - `src/frontend/lexer.cpp`, `parser.cpp`, `ir_generator.cpp`
- **Tests:** `tests/cpp/t81_frontend_*_test.cpp`, `tests/cpp/e2e_*_test.cpp`

______________________________________________________________________

## 1. Frontend Architecture Overview

The T81Lang compiler frontend is responsible for converting `.t81` source code into TISC IR. It follows a classic pipeline:

1.  **Lexer:** Converts source text into a stream of tokens.
2.  **Parser:** Builds an Abstract Syntax Tree (AST) from the token stream.
3.  **Semantic Analyzer:** (Currently a stub) Traverses the AST to check for type errors and resolve symbols. **This is a high-priority area for new development.**
4.  **IR Generator:** Traverses the AST to produce a linear sequence of TISC instructions.

This guide will walk you through modifying the Lexer, Parser, and IR Generator.

______________________________________________________________________

## 2. Step 1: Update the Lexer

First, teach the lexer to recognize the new syntax.

### 2.1 Add the Token Type

In `include/t81/frontend/lexer.hpp`, add a new entry to the `TokenType` enum.

```cpp
// in enum class TokenType
// ...
Plus, Minus, Star, Slash, Percent, // <-- Add Percent
// ...
```

### 2.2 Recognize the Lexeme

In `src/frontend/lexer.cpp`, find the `next_token()` method and add a case for the `%` character in the `switch` statement.

```cpp
// in Lexer::next_token()
switch (c) {
    // ...
    case '%': return make_token(TokenType::Percent);
    // ...
}
```

______________________________________________________________________

## 3. Step 2: Update the Parser

Next, update the parser to understand the operator's precedence. The modulo operator has the same precedence as multiplication and division.

### 3.1 Update the Grammar Rule

In `src/frontend/parser.cpp`, find the `factor()` method. Update the `while` loop to include `TokenType::Percent`.

```cpp
// in Parser::factor()
std::unique_ptr<Expr> Parser::factor() {
    std::unique_ptr<Expr> expr = unary();
    // Add TokenType::Percent to this list
    while (match({TokenType::Slash, TokenType::Star, TokenType::Percent})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}
```
The parser can now correctly place the modulo operator in the AST.

______________________________________________________________________

## 4. Step 3: Update the IR Generator

Finally, teach the IR generator how to convert the new AST node into a TISC instruction.

### 4.1 Implement the Visitor Logic

In `src/frontend/ir_generator.cpp`, find the `visit(const BinaryExpr& expr)` method. Add a `case` for `TokenType::Percent`.

```cpp
// in IRGenerator::visit(const BinaryExpr& expr)
std::any IRGenerator::visit(const BinaryExpr& expr) {
    // ... (visit left and right operands)

    switch (expr.op.type) {
        // ...
        case TokenType::Star:
            emit({tisc::Opcode::Mul, {result, left, right}});
            break;
        case TokenType::Percent: // <-- Add this case
            emit({tisc::Opcode::Mod, {result, left, right}});
            break;
        // ...
    }
    return result;
}
```
The compiler can now generate the `Mod` TISC instruction.

______________________________________________________________________

## 5. Step 4: Write an End-to-End Test

No feature is complete without a test. An end-to-end test is the best way to validate this change.

1.  **Create a Test File:** Add a new file in `tests/cpp/`, such as `e2e_mod_test.cpp`.
2.  **Write the Test:** The test should compile a snippet of T81Lang code using `%` and then execute it on the VM, asserting the final result is correct. See `tests/cpp/e2e_arithmetic_test.cpp` for a complete example.
3.  **Add to CMake:** Add your new test file as an executable and test target in the root `CMakeLists.txt`.

This process—**Lexer -> Parser -> IR Generator -> E2E Test**—is the standard workflow for adding new language features.
