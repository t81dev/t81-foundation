#include <iostream>
#include <vector>
#include "t81/axion/engine.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

using namespace t81::frontend;

void test_collections_execution() {
  std::string source = R"(
        fn main() -> i32 {
            std.core.debug("Start");
            var m = Map[String]();
            std.core.debug("Map created");
            std.collections.map_put(m, "k1", "v1");
            std.collections.map_put(m, "k2", "v2");
            std.core.debug("Map put done");
            std.core.assert(std.collections.map_size(m) == 2);
            std.core.debug("Map size ok");

            var v1 = std.collections.map_get(m, "k1");
            std.core.assert(std.option.is_some(v1));
            std.core.assert(std.option.unwrap(v1) == "v1");
            std.core.debug("Map get v1 ok");

            var v3 = std.collections.map_get(m, "k3");
            std.core.assert(std.option.is_none(v3));
            std.core.debug("Map get k3 ok");

            std.core.assert(std.collections.map_has(m, "k2"));
            std.core.debug("Map has k2 ok");

            std.collections.map_remove(m, "k1");
            std.core.assert(std.collections.map_size(m) == 1);
            std.core.assert(!std.collections.map_has(m, "k1"));
            std.core.debug("Map remove ok");

            var s = Set[String]();
            std.core.debug("Set created");
            std.collections.set_add(s, "a");
            std.collections.set_add(s, "b");
            std.collections.set_add(s, "a");
            std.core.assert(std.collections.set_size(s) == 2);
            std.core.assert(std.collections.set_has(s, "a"));
            std.collections.set_remove(s, "a");
            std.core.assert(!std.collections.set_has(s, "a"));
            std.core.debug("Set ok");
            return 0;
        }
    )";

  // Note: I'm using std.collections.* directly because I want to be sure I'm hitting the call sites
  // I modified in IRGenerator. The frontend AST for `m.put(...)` lowers to `CallExpr` which
  // eventually resolves to `std.collections.map_put` if I didn't change the lowering of method
  // calls. Assuming method calls resolve to stdlib functions as per `canonical_stdlib_call_name` in
  // ir_generator.hpp.

  Lexer lexer(source);
  Parser parser(lexer);
  auto statements = parser.parse();

  SemanticAnalyzer analyzer(statements);
  analyzer.analyze();

  if (analyzer.had_error()) {
    std::cerr << "Semantic analysis failed." << std::endl;
    for (const auto& d : analyzer.diagnostics()) {
      std::cerr << d.file << ":" << d.line << ":" << d.column << ": " << d.message << std::endl;
    }
    T81_TEST_CHECK(false);
    return;
  }

  IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);

  t81::tisc::ir::IntermediateProgram iprog;
  try {
    iprog = ir_gen.generate(statements);
  } catch (const std::exception& e) {
    std::cerr << "IR Generation failed: " << e.what() << std::endl;
    T81_TEST_CHECK(false);
    return;
  }

  // Convert IntermediateProgram to Program
  t81::tisc::BinaryEmitter emitter;
  t81::tisc::Program prog = emitter.emit(iprog);

  // Run VM
  auto vm = t81::vm::make_interpreter_vm(t81::axion::make_allow_all_engine());
  vm->load_program(prog);
  auto res = vm->run_to_halt(1000);  // Should be enough steps

  if (!res) {
    std::cerr << "VM Output:\n";
    for (const auto& line : vm->state().printed_output) {
      std::cerr << "  " << line << "\n";
    }
    std::cerr << "VM Execution failed: " << t81::vm::to_string(res.error()) << std::endl;
    T81_TEST_CHECK(false);
  } else {
    T81_TEST_CHECK(true);
  }
}

int main() {
  try {
    test_collections_execution();
    std::cout << "Collections scaffold execution test passed!\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
