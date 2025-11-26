#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include <cassert>
#include <iostream>

void test_if_statement_true() {
    std::string source = "fn main() -> T81Int { if (1 < 2) { return 1; } return 0; }";
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

    assert(vm->state().get_register(0) == 1 && "VM register R0 has incorrect value for true branch");

    std::cout << "E2ETest test_if_statement_true passed!" << std::endl;
}

void test_if_statement_false() {
    std::string source = "fn main() -> T81Int { if (2 < 1) { return 1; } return 0; }";
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

    assert(vm->state().get_register(0) == 0 && "VM register R0 has incorrect value for false branch");

    std::cout << "E2ETest test_if_statement_false passed!" << std::endl;
}

void test_if_else_statement() {
    std::string source = "fn main() -> T81Int { if (2 < 1) { return 1; } else { return 123; } }";
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

    assert(vm->state().get_register(0) == 123 && "VM register R0 has incorrect value for else branch");

    std::cout << "E2ETest test_if_else_statement passed!" << std::endl;
}

int main() {
    test_if_statement_true();
    test_if_statement_false();
    test_if_else_statement();
    return 0;
}
