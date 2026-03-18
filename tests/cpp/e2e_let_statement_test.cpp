#include <iostream>
#include <vector>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

void test_let_statement_e2e() {
  [[maybe_unused]] std::string source = "fn main() -> T81Int { let x: T81Int = 42; return x; }";
  t81::frontend::Lexer lexer(source);
  t81::frontend::Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  T81_TEST_CHECK(!parser.had_error() && "Parsing failed");

  [[maybe_unused]] t81::frontend::IRGenerator generator;
  [[maybe_unused]] auto ir_program = generator.generate(stmts);

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  std::cout << "Instructions:\n";
  for (size_t i = 0; i < program.insns.size(); ++i) {
    std::cout << i << ": opcode=" << static_cast<int>(program.insns[i].opcode)
              << " a=" << static_cast<int>(program.insns[i].a)
              << " b=" << static_cast<int>(program.insns[i].b)
              << " c=" << static_cast<int>(program.insns[i].c) << "\n";
  }

  [[maybe_unused]] auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  (void)vm->run_to_halt();

  for (int i = 0; i < 5; ++i) {
    std::cout << "R" << i << " = " << vm->state().contexts[0].registers[i] << "\n";
  }

  // Per TISC calling convention, the return value is in R0.
  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 42 &&
                 "VM register R1 has incorrect value");

  std::cout << "E2ETest test_let_statement_e2e passed!" << std::endl;
}

int main() {
  test_let_statement_e2e();
  return 0;
}
