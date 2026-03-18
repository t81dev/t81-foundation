#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <string>
#include <vector>
#include "test_runtime_check.hpp"

using namespace t81;

int64_t run_e2e_test(const std::string& source) {
  frontend::Lexer lexer(source);
  frontend::Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();
  T81_TEST_CHECK(!parser.had_error());

  frontend::SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  T81_TEST_CHECK(!analyzer.had_error());

  [[maybe_unused]] frontend::IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  [[maybe_unused]] tisc::ir::IntermediateProgram ir = ir_gen.generate(stmts);

  [[maybe_unused]] tisc::BinaryEmitter emitter;
  [[maybe_unused]] tisc::Program program = emitter.emit(ir);

  [[maybe_unused]] auto vm = vm::make_interpreter_vm();
  vm->load_program(program);
  (void)vm->run_to_halt();

  return vm->state().contexts[0].registers[1];
}

void test_option_result() {
  const std::string source = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = Some(3);
            let out: i32 = match (maybe) {
                Some(v) => v,
                None => 0
            };
            return out;
        }
    )";
  const auto out_a = run_e2e_test(source);
  const auto out_b = run_e2e_test(source);
  T81_TEST_CHECK(out_a == out_b);
}

int main() {
  test_option_result();
  std::cout << "E2E option/result test passed!" << std::endl;
  return 0;
}
