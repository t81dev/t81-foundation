#include <iostream>
#include <vector>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

void test_option_type_e2e() {
  std::string source = R"(
        fn main() -> i32 {
            let maybe_val: Option[i32] = Some(42);
            let res: i32 = match (maybe_val) {
                Some(x) => x,
                None => 0
            };
            return res;
        }
    )";

  t81::frontend::Lexer lexer(source);
  t81::frontend::Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  T81_TEST_CHECK(!parser.had_error() && "Parsing failed");

  t81::frontend::SemanticAnalyzer semantic_analyzer(stmts);
  semantic_analyzer.analyze();
  T81_TEST_CHECK(!semantic_analyzer.had_error() && "Semantic analysis failed");

  [[maybe_unused]] t81::frontend::IRGenerator generator;
  generator.attach_semantic_analyzer(&semantic_analyzer);
  [[maybe_unused]] auto ir_program = generator.generate(stmts);

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  [[maybe_unused]] auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  (void)vm->run_to_halt();

  T81_TEST_CHECK(vm->state().contexts[0].registers[2] == 42 &&
                 "VM register R2 has incorrect value");

  std::cout << "E2ETest test_option_type_e2e passed!" << std::endl;
}

int main() {
  test_option_type_e2e();
  return 0;
}
