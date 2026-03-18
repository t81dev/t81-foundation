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

// Helper to compile and run a T81Lang source string and return the final value of register r0.
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

  return vm->state().contexts[0].registers[2];
}

int main() {
  const std::string match_test_source = R"(
        fn main() -> i32 {
            let opt: Option[i32] = Some(123);
            let result: i32 = match (opt) {
                Some(value) => value,
                None => 42
            };
            return result;
        }
    )";

  [[maybe_unused]] int64_t result = run_e2e_test(match_test_source);
  T81_TEST_CHECK(result == 123);

  std::cout << "E2E match expression test passed!" << std::endl;
  return 0;
}
