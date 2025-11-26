#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void test_let_statement_e2e() {
    std::string source = "fn main() -> T81Int { let x: T81Int = 42t81; return x; }";
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

    // Per TISC calling convention, the return value is in R0.
    assert(vm->state().get_register(0) == 42 && "VM register R0 has incorrect value");

    std::cout << "E2ETest test_let_statement_e2e passed!" << std::endl;
}

int main() {
    test_let_statement_e2e();
    return 0;
}
