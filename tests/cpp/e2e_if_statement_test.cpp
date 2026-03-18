#include <iostream>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

void test_if_statement_true() {
  [[maybe_unused]] std::string source =
      "fn main() -> T81Int { if (1 < 2) { return 1; } return 0; }";
  t81::frontend::Lexer lexer(source);
  t81::frontend::Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  T81_TEST_CHECK(!parser.had_error() && "Parsing failed");

  [[maybe_unused]] t81::frontend::IRGenerator generator;
  [[maybe_unused]] auto ir_program = generator.generate(stmts);

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  [[maybe_unused]] auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  (void)vm->run_to_halt();

  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 1 &&
                 "VM register R1 has incorrect value for true branch");

  std::cout << "E2ETest test_if_statement_true passed!" << std::endl;
}

void test_if_statement_false() {
  [[maybe_unused]] std::string source =
      "fn main() -> T81Int { if (2 < 1) { return 1; } return 0; }";
  t81::frontend::Lexer lexer(source);
  t81::frontend::Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  T81_TEST_CHECK(!parser.had_error() && "Parsing failed");

  [[maybe_unused]] t81::frontend::IRGenerator generator;
  [[maybe_unused]] auto ir_program = generator.generate(stmts);

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  [[maybe_unused]] auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  (void)vm->run_to_halt();

  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 0 &&
                 "VM register R1 has incorrect value for false branch");

  std::cout << "E2ETest test_if_statement_false passed!" << std::endl;
}

void test_if_else_statement() {
  [[maybe_unused]] std::string source =
      "fn main() -> T81Int { if (2 < 1) { return 1; } else { return 123; } }";
  t81::frontend::Lexer lexer(source);
  t81::frontend::Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  T81_TEST_CHECK(!parser.had_error() && "Parsing failed");

  [[maybe_unused]] t81::frontend::IRGenerator generator;
  [[maybe_unused]] auto ir_program = generator.generate(stmts);

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  [[maybe_unused]] auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  (void)vm->run_to_halt();

  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 123 &&
                 "VM register R1 has incorrect value for else branch");

  std::cout << "E2ETest test_if_else_statement passed!" << std::endl;
}

int main() {
  test_if_statement_true();
  test_if_statement_false();
  test_if_else_statement();
  return 0;
}
