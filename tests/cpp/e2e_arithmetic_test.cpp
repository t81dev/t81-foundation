#include <iostream>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

void test_addition() {
  [[maybe_unused]] std::string source = "fn main() -> T81Int { return 20t81 + 22t81; }";
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

  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 42 &&
                 "VM register R1 has incorrect value after addition");

  std::cout << "E2ETest test_addition passed!" << std::endl;
}

void test_subtraction() {
  [[maybe_unused]] std::string source = "fn main() -> T81Int { return 50t81 - 8t81; }";
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

  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 42 &&
                 "VM register R1 has incorrect value after subtraction");

  std::cout << "E2ETest test_subtraction passed!" << std::endl;
}

void test_multiplication() {
  [[maybe_unused]] std::string source = "fn main() -> T81Int { return 6t81 * 7t81; }";
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

  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 42 &&
                 "VM register R1 has incorrect value after multiplication");

  std::cout << "E2ETest test_multiplication passed!" << std::endl;
}

void test_division() {
  [[maybe_unused]] std::string source = "fn main() -> T81Int { return 84t81 / 2t81; }";
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

  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 42 &&
                 "VM register R1 has incorrect value after division");

  std::cout << "E2ETest test_division passed!" << std::endl;
}

int main() {
  test_addition();
  test_subtraction();
  test_multiplication();
  test_division();
  return 0;
}
