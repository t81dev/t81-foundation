#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include "t81/tisc/ir.hpp"
#include "t81/tisc/pretty_printer.hpp"

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

    assert(instructions.size() == 3);
    assert(instructions[0].opcode == Opcode::LOADI);
    assert(std::get<t81::tisc::ir::Register>(instructions[0].operands[0]).index == 0);
    assert(std::get<t81::tisc::ir::Immediate>(instructions[0].operands[1]).value == 1);
    assert(instructions[1].opcode == Opcode::LOADI);
    assert(std::get<t81::tisc::ir::Register>(instructions[1].operands[0]).index == 1);
    assert(std::get<t81::tisc::ir::Immediate>(instructions[1].operands[1]).value == 2);
    assert(instructions[2].opcode == Opcode::ADD);
    assert(std::get<t81::tisc::ir::Register>(instructions[2].operands[0]).index == 2);
    assert(std::get<t81::tisc::ir::Register>(instructions[2].operands[1]).index == 0);
    assert(std::get<t81::tisc::ir::Register>(instructions[2].operands[2]).index == 1);

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

    assert(instructions.size() == 7);
    assert(instructions[0].opcode == Opcode::LOADI);
    assert(instructions[1].opcode == Opcode::LOADI);
    assert(instructions[2].opcode == Opcode::CMP);
    assert(instructions[3].opcode == Opcode::JP);
    assert(instructions[4].opcode == Opcode::JZ);
    assert(instructions[5].opcode == Opcode::LOADI);
    assert(instructions[6].opcode == Opcode::LABEL);

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

    assert(instructions.size() == 10);
    assert(instructions[0].opcode == Opcode::LOADI);
    assert(instructions[1].opcode == Opcode::LOADI);
    assert(instructions[2].opcode == Opcode::CMP);
    assert(instructions[3].opcode == Opcode::JP);
    assert(instructions[4].opcode == Opcode::JZ);
    assert(instructions[5].opcode == Opcode::LOADI);
    assert(instructions[6].opcode == Opcode::JMP);
    assert(instructions[7].opcode == Opcode::LABEL);
    assert(instructions[8].opcode == Opcode::LOADI);
    assert(instructions[9].opcode == Opcode::LABEL);

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

    assert(instructions.size() == 9);
    assert(instructions[0].opcode == Opcode::LABEL);
    assert(instructions[1].opcode == Opcode::LOADI);
    assert(instructions[2].opcode == Opcode::LOADI);
    assert(instructions[3].opcode == Opcode::CMP);
    assert(instructions[4].opcode == Opcode::JP);
    assert(instructions[5].opcode == Opcode::JZ);
    assert(instructions[6].opcode == Opcode::LOADI);
    assert(instructions[7].opcode == Opcode::JMP);
    assert(instructions[8].opcode == Opcode::LABEL);

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

    assert(instructions.size() == 3);
    assert(instructions[0].opcode == Opcode::LOADI);
    assert(instructions[1].opcode == Opcode::LOADI);
    assert(instructions[2].opcode == Opcode::MOV);

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

    assert(instructions.size() == 9);
    assert(instructions[0].opcode == Opcode::LABEL);
    assert(instructions[1].opcode == Opcode::PUSH);
    assert(instructions[2].opcode == Opcode::POP);
    assert(instructions[3].opcode == Opcode::POP);
    assert(instructions[4].opcode == Opcode::RET);
    assert(instructions[5].opcode == Opcode::LOADI);
    assert(instructions[6].opcode == Opcode::PUSH);
    assert(instructions[7].opcode == Opcode::CALL);
    assert(instructions[8].opcode == Opcode::MOV);

    std::cout << "IRGeneratorTest test_function_call passed!" << std::endl;
}

int main() {
    test_simple_addition();
    test_if_statement();
    test_if_else_statement();
    test_while_loop();
    test_assignment();
    test_function_call();
    return 0;
}
