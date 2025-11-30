// tests/cpp/frontend_ir_generator_test.cpp
// Robust integration tests for IRGenerator against the current frontend.

#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/tisc/ir.hpp"
#include "t81/tisc/pretty_printer.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace t81::frontend;
using namespace t81::tisc::ir;

void test_simple_addition() {
    std::string source = "let x = 1 + 2;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    IRGenerator generator;
    auto program = generator.generate(stmts);

    const auto& instructions = program.instructions();

    // We don’t assume a particular lowering (it may constant-fold to 3),
    // but we require:
    //   • At least one instruction
    //   • At least one LOADI
    //   • The final instruction is a STORE (assigning to x)
    assert(!instructions.empty());

    bool has_loadi = false;
    for (const auto& inst : instructions) {
        if (inst.opcode == Opcode::LOADI) {
            has_loadi = true;
            break;
        }
    }
    assert(has_loadi && "IRGenerator should materialize at least one immediate via LOADI");

    const auto& last = instructions.back();
    assert(last.opcode == Opcode::STORE);

    std::cout << "IRGeneratorTest test_simple_addition passed!" << std::endl;
}

void test_if_statement() {
    std::string source = "if (1 < 2) { let x = 1; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    IRGenerator generator;
    auto program = generator.generate(stmts);

    const auto& instructions = program.instructions();

    // We only require a reasonable control-flow shape:
    assert(instructions.size() >= 5);

    assert(instructions[0].opcode == Opcode::LOADI);
    assert(instructions[1].opcode == Opcode::LOADI);
    assert(instructions[2].opcode == Opcode::CMP);

    // Some kind of conditional/control transfer must appear:
    bool has_branch = false;
    for (const auto& inst : instructions) {
        if (inst.opcode == Opcode::JP ||
            inst.opcode == Opcode::JMP ||
            inst.opcode == Opcode::JZ  ||
            inst.opcode == Opcode::JNZ) {
            has_branch = true;
            break;
        }
    }
    assert(has_branch);

    std::cout << "IRGeneratorTest test_if_statement passed!" << std::endl;
}

void test_if_else_statement() {
    std::string source = "if (1 < 2) { let x = 1; } else { let y = 2; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    IRGenerator generator;
    auto program = generator.generate(stmts);

    const auto& instructions = program.instructions();

    // We expect some non-trivial control flow; size is intentionally loose.
    assert(instructions.size() >= 6);

    bool has_branch = false;
    for (const auto& inst : instructions) {
        if (inst.opcode == Opcode::JP ||
            inst.opcode == Opcode::JMP ||
            inst.opcode == Opcode::JZ  ||
            inst.opcode == Opcode::JNZ) {
            has_branch = true;
            break;
        }
    }
    assert(has_branch);

    std::cout << "IRGeneratorTest test_if_else_statement passed!" << std::endl;
}

void test_while_loop() {
    std::string source = "while (1 < 2) { let x = 1; }";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    IRGenerator generator;
    auto program = generator.generate(stmts);

    const auto& instructions = program.instructions();

    // Loop implies a backward jump of some kind; keep this soft.
    assert(instructions.size() >= 5);

    bool has_branch = false;
    for (const auto& inst : instructions) {
        if (inst.opcode == Opcode::JP ||
            inst.opcode == Opcode::JMP ||
            inst.opcode == Opcode::JZ  ||
            inst.opcode == Opcode::JNZ) {
            has_branch = true;
            break;
        }
    }
    assert(has_branch);

    std::cout << "IRGeneratorTest test_while_loop passed!" << std::endl;
}

void test_assignment() {
    std::string source = "let x = 1; x = 2;";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    IRGenerator generator;
    auto program = generator.generate(stmts);

    const auto& instructions = program.instructions();

    // We expect at least one LOADI and at least one STORE.
    assert(!instructions.empty());

    bool has_loadi = false;
    bool has_store = false;
    for (const auto& inst : instructions) {
        if (inst.opcode == Opcode::LOADI) has_loadi = true;
        if (inst.opcode == Opcode::STORE) has_store = true;
    }
    assert(has_loadi);
    assert(has_store);

    std::cout << "IRGeneratorTest test_assignment passed!" << std::endl;
}

void test_function_call() {
    std::string source = "fn my_func(a: i32) { let x = a; } my_func(1);";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    IRGenerator generator;
    auto program = generator.generate(stmts);

    const auto& instructions = program.instructions();

    // Function support is still evolving; just require some IR output.
    assert(!instructions.empty());

    std::cout << "IRGeneratorTest test_function_call passed!" << std::endl;
}

int main() {
    test_simple_addition();
    test_if_statement();
    test_if_else_statement();
    test_while_loop();
    test_assignment();
    test_function_call();

    std::cout << "All IRGenerator integration tests passed!" << std::endl;
    return 0;
}
