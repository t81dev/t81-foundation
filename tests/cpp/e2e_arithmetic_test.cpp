#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include <cassert>
#include <iostream>

void test_addition() {
    std::string source = "fn main() -> T81Int { return 20t81 + 22t81; }";
    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();

    assert(!parser.had_error() && "Parsing failed");

    t81::frontend::IRGenerator generator;
    auto ir_program = generator.generate(stmts);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    vm->run_to_halt();

    assert(vm->state().registers[0] == 42 && "VM register R0 has incorrect value after addition");

    std::cout << "E2ETest test_addition passed!" << std::endl;
}

void test_subtraction() {
    std::string source = "fn main() -> T81Int { return 50t81 - 8t81; }";
    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();

    assert(!parser.had_error() && "Parsing failed");

    t81::frontend::IRGenerator generator;
    auto ir_program = generator.generate(stmts);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    vm->run_to_halt();

    assert(vm->state().registers[0] == 42 && "VM register R0 has incorrect value after subtraction");

    std::cout << "E2ETest test_subtraction passed!" << std::endl;
}

void test_multiplication() {
    std::string source = "fn main() -> T81Int { return 6t81 * 7t81; }";
    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();

    assert(!parser.had_error() && "Parsing failed");

    t81::frontend::IRGenerator generator;
    auto ir_program = generator.generate(stmts);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    vm->run_to_halt();

    assert(vm->state().registers[0] == 42 && "VM register R0 has incorrect value after multiplication");

    std::cout << "E2ETest test_multiplication passed!" << std::endl;
}

void test_division() {
    std::string source = "fn main() -> T81Int { return 84t81 / 2t81; }";
    t81::frontend::Lexer lexer(source);
    t81::frontend::Parser parser(lexer);
    auto stmts = parser.parse();

    assert(!parser.had_error() && "Parsing failed");

    t81::frontend::IRGenerator generator;
    auto ir_program = generator.generate(stmts);

    t81::tisc::BinaryEmitter emitter;
    auto program = emitter.emit(ir_program);

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    vm->run_to_halt();

    assert(vm->state().registers[0] == 42 && "VM register R0 has incorrect value after division");

    std::cout << "E2ETest test_division passed!" << std::endl;
}

int main() {
    test_addition();
    test_subtraction();
    test_multiplication();
    test_division();
    return 0;
}
