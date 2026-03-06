#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "test_runtime_check.hpp"

using namespace t81;

static std::vector<std::string> run_and_capture_prints(const std::string& source) {
  frontend::Lexer lexer(source);
  frontend::Parser parser(lexer);
  auto stmts = parser.parse();
  if (parser.had_error()) {
    throw std::runtime_error("Parser failed in run_and_capture_prints");
  }

  frontend::SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  if (analyzer.had_error()) {
    throw std::runtime_error("Semantic analysis failed in run_and_capture_prints");
  }

  frontend::IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  tisc::ir::IntermediateProgram ir = ir_gen.generate(stmts);

  tisc::BinaryEmitter emitter;
  tisc::Program program = emitter.emit(ir);

  auto vm = vm::make_interpreter_vm();
  vm->load_program(program);
  auto run = vm->run_to_halt();
  if (!run.has_value()) {
    throw std::runtime_error("VM execution failed in run_and_capture_prints");
  }

  return vm->state().printed_output;
}

static void test_print_runtime_captures_scalars_in_order() {
  const std::string source = R"(
    fn main() -> i32 {
      let i: i32 = 42;
      let s: T81String = "alpha";
      print(i);
      print(s);
      print(7);
      return 0;
    }
  )";

  const auto output = run_and_capture_prints(source);
  T81_TEST_CHECK(output.size() == 3);
  T81_TEST_CHECK(output[0] == "42");
  T81_TEST_CHECK(output[1] == "alpha");
  T81_TEST_CHECK(output[2] == "7");
}

static void test_print_runtime_supports_t81_literals_and_bool() {
  const std::string source = R"(
    fn main() -> i32 {
      let bi: T81BigInt = 12t81;
      let tf: T81Float = 1.20t81;
      let fr: T81Fraction = 22/7t81;
      print(bi);
      print(tf);
      print(fr);
      print(true);
      print(false);
      return 0;
    }
  )";

  const auto output = run_and_capture_prints(source);
  T81_TEST_CHECK(output.size() == 5);
  T81_TEST_CHECK(output[0] == "12");
  T81_TEST_CHECK(output[1].find("t81") != std::string::npos);
  T81_TEST_CHECK(output[2] == "22/7t81");
  T81_TEST_CHECK(output[3] == "true");
  T81_TEST_CHECK(output[4] == "false");
}

static void test_runtime_arithmetic_for_t81_numeric_families() {
  const std::string source = R"(
    fn main() -> i32 {
      let i1: T81BigInt = 12t81;
      let i_two: T81BigInt = 5t81;
      let isum: T81BigInt = i1 + i_two;

      let f1: T81Float = 1;
      let f2: T81Float = 2;
      let fdiv: T81Float = f1 / f2;

      let q1: T81Fraction = 22/7t81;
      let q2: T81Fraction = 1/7t81;
      let qsub: T81Fraction = q1 - q2;

      print(isum);
      print(fdiv);
      print(qsub);
      return 0;
    }
  )";

  const auto output = run_and_capture_prints(source);
  T81_TEST_CHECK(output.size() == 3);
  T81_TEST_CHECK(output[0] == "17");
  T81_TEST_CHECK(output[1] == "0.5t81");
  T81_TEST_CHECK(output[2] == "3/1t81");
}

static void test_runtime_preserves_oversized_bigint_literals() {
  const std::string source = R"(
    fn main() -> i32 {
      let big: T81BigInt = 9223372036854775808t81;
      let delta: T81BigInt = 2t81;
      let sum: T81BigInt = big + delta;
      print(big);
      print(sum);
      return 0;
    }
  )";

  const auto output = run_and_capture_prints(source);
  T81_TEST_CHECK(output.size() == 2);
  T81_TEST_CHECK(output[0] == "9223372036854775808");
  T81_TEST_CHECK(output[1] == "9223372036854775810");
}

static void test_runtime_materializes_bigint_construction_paths() {
  const std::string source = R"(
    fn main() -> i32 {
      let from_context: T81BigInt = 7;
      let from_call: T81BigInt = std.math.bigint.from_int(42);
      print(from_context);
      print(from_call);
      return 0;
    }
  )";

  const auto output = run_and_capture_prints(source);
  T81_TEST_CHECK(output.size() == 2);
  T81_TEST_CHECK(output[0] == "7");
  T81_TEST_CHECK(output[1] == "42");
}

static void test_base81_float_parse_print_roundtrip_is_stable() {
  const std::vector<std::string_view> literals = {
      "1.20t81", "0.5t81", "-2.25t81", "123456.5t81", "0.1t81",
  };

  for (const auto literal : literals) {
    const std::string source_a =
        "fn main() -> i32 { let x: T81Float = " + std::string(literal) + "; print(x); return 0; }";
    const auto output_a = run_and_capture_prints(source_a);
    T81_TEST_CHECK(output_a.size() == 1);

    const std::string source_b =
        "fn main() -> i32 { let x: T81Float = " + output_a[0] + "; print(x); return 0; }";
    const auto output_b = run_and_capture_prints(source_b);
    T81_TEST_CHECK(output_b.size() == 1);
    T81_TEST_CHECK(output_b[0] == output_a[0]);
  }
}

int main() {
  test_print_runtime_captures_scalars_in_order();
  test_print_runtime_supports_t81_literals_and_bool();
  test_runtime_arithmetic_for_t81_numeric_families();
  test_runtime_preserves_oversized_bigint_literals();
  test_runtime_materializes_bigint_construction_paths();
  test_base81_float_parse_print_roundtrip_is_stable();
  std::cout << "e2e print runtime test passed!\n";
  return 0;
}
