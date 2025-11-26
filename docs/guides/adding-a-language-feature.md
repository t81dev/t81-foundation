# Developer Guide: Adding a Feature to T81Lang

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Developer Guide: Adding a Feature to T81Lang](#developer-guide-adding-a-feature-to-t81lang)
    - [Prerequisites](#prerequisites)
    - [Step 1: Update the Lexer](#step-1-update-the-lexer)
      - [1.1 Add the Token Type](#11-add-the-token-type)
      - [1.2 Recognize the Lexeme](#12-recognize-the-lexeme)
    - [Step 2: Update the Parser](#step-2-update-the-parser)
      - [2.1 Update the Grammar Rule](#21-update-the-grammar-rule)
    - [Step 3: Update the IR Generator](#step-3-update-the-ir-generator)
      - [3.1 Implement the Visitor Logic](#31-implement-the-visitor-logic)
    - [Step 4: Write a Test](#step-4-write-a-test)
      - [4.1 Create a Test File](#41-create-a-test-file)
      - [4.2 Write the Test Code](#42-write-the-test-code)
      - [4.3 Add the Test to CMake](#43-add-the-test-to-cmake)
- [In CMakeLists.txt](#in-cmakeliststxt)
    - [Conclusion](#conclusion)

<!-- T81-TOC:END -->



















This guide provides a step-by-step walkthrough for adding a new feature to the T81Lang language. We will use the example of adding a new binary operator, the modulo operator (`%`), to illustrate the process.

Following this guide will familiarize you with the key components of the new C++ compiler toolchain.

______________________________________________________________________

### Prerequisites

Before you begin, ensure you have a working development environment and can successfully build the project and run the test suite by following the instructions in the main `README.md`.

______________________________________________________________________

### Step 1: Update the Lexer

The first step is to teach the lexer how to recognize the new syntax.

#### 1.1 Add the Token Type

Open `include/t81/frontend/lexer.hpp` and add a new entry to the `TokenType` enum for the modulo operator.

```cpp
// in enum class TokenType
// ...
Plus, Minus, Star, Slash, Percent, // <-- Add Percent here
Equal, EqualEqual, Bang, BangEqual,
// ...
```

#### 1.2 Recognize the Lexeme

Open `src/frontend/lexer.cpp` and find the `next_token()` method. In the main `switch` statement, add a case for the `%` character.

```cpp
// in Lexer::next_token()
// ...
switch (c) {
    case '+': return make_token(TokenType::Plus);
    case '*': return make_token(TokenType::Star);
    case '%': return make_token(TokenType::Percent); // <-- Add this line
    case '/': return make_token(TokenType::Slash);
// ...
```

The lexer can now produce `Percent` tokens.

______________________________________________________________________

### Step 2: Update the Parser

Next, we need to update the parser to understand the precedence and semantics of the new operator. The modulo operator has the same precedence as multiplication and division.

#### 2.1 Update the Grammar Rule

Open `src/frontend/parser.cpp` and find the `factor()` method. This method handles the `*`, `/`, and now `%` operators. Update the `while` loop to include `TokenType::Percent`.

```cpp
// in Parser::factor()
std::unique_ptr<Expr> Parser::factor() {
    std::unique_ptr<Expr> expr = unary();
    // Add TokenType::Percent to this match list
    while (match({TokenType::Slash, TokenType::Star, TokenType::Percent})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}
```

The parser can now correctly place the modulo operator in the Abstract Syntax Tree (AST) with the correct precedence.

______________________________________________________________________

### Step 3: Update the IR Generator

The final step is to teach the IR (Intermediate Representation) generator how to convert the new AST node into TISC instructions.

#### 3.1 Implement the Visitor Logic

Open `src/frontend/ir_generator.cpp` and find the `visit(const BinaryExpr& expr)` method. Add a `case` for `TokenType::Percent` in the `switch` statement. For this example, we will assume there is a `MOD` opcode in the TISC instruction set.

```cpp
// in IRGenerator::visit(const BinaryExpr& expr)
std::any IRGenerator::visit(const BinaryExpr& expr) {
    // ... (visit left and right operands)

    switch (expr.op.type) {
        // ...
        case TokenType::Star:
            emit(tisc::Instruction{tisc::Opcode::MUL, {result, left, right}});
            break;
        case TokenType::Percent: // <-- Add this case
            emit(tisc::Instruction{tisc::Opcode::MOD, {result, left, right}});
            break;
        // ...
    }
    return result;
}
```

The compiler can now generate TISC code for the modulo operator.

______________________________________________________________________

### Step 4: Write a Test

No feature is complete without a test. To validate our new operator, we need to create a new test case.

#### 4.1 Create a Test File

Create a new file in `tests/cpp/` named `frontend_modulo_test.cpp`.

#### 4.2 Write the Test Code

Add C++ code to the new file that:

1. Creates a piece of T81Lang source code using the `%` operator (e.g., `let x = 10 % 3;`).
2. Runs the lexer, parser, and IR generator.
3. Asserts that the generated TISC IR contains the expected `MOD` instruction.

```cpp
// Example test code for tests/cpp/frontend_modulo_test.cpp
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/ir_generator.hpp"
#include <cassert>

int main() {
    std::string source = "let x = 10 % 3;";
    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();
    t81::frontend::IRGenerator generator;
    auto program = generator.generate(stmts);

    bool found_mod = false;
    for (const auto& instr : program.instructions) {
        if (instr.opcode == t81::tisc::Opcode::MOD) {
            found_mod = true;
            break;
        }
    }
    assert(found_mod && "MOD instruction was not generated for '%' operator.");
    return 0;
}
```

#### 4.3 Add the Test to CMake

Finally, open the root `CMakeLists.txt` and add a new test target for our file, similar to the other `t81_frontend_*_test` targets.

```cmake
# In CMakeLists.txt
add_executable(t81_frontend_modulo_test tests/cpp/frontend_modulo_test.cpp)
target_link_libraries(t81_frontend_modulo_test PRIVATE t81_frontend t81_tisc)
add_test(NAME t81_frontend_modulo_test COMMAND $<TARGET_FILE:t81_frontend_modulo_test>)
```

______________________________________________________________________

### Conclusion

You have now successfully added a new binary operator to the T81Lang language. This process—**Lexer -> Parser -> IR Generator -> Test**—is the standard workflow for adding new syntax and features to the compiler.
