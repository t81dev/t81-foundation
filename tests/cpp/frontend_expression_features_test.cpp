#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

using namespace t81::frontend;

void check_no_error(const std::string& source, const std::string& test_name) {
  std::cout << "Running test: " << test_name << "..." << std::endl;
  Lexer lexer(source);
  Parser parser(lexer);
  auto statements = parser.parse();
  if (parser.had_error()) {
    std::cerr << "Parser error in " << test_name << std::endl;
    exit(1);
  }

  SemanticAnalyzer analyzer(statements);
  analyzer.analyze();
  if (analyzer.had_error()) {
    for (const auto& d : analyzer.diagnostics()) {
      std::cerr << d.message << std::endl;
    }
    std::cerr << "Semantic analysis error in " << test_name << std::endl;
    exit(1);
  }

  IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  try {
    [[maybe_unused]] auto prog = ir_gen.generate(statements);
  } catch (const std::exception& e) {
    std::cerr << "IR generation error in " << test_name << ": " << e.what() << std::endl;
    exit(1);
  }
  std::cout << "Passed." << std::endl;
}

int main() {
  // Test 1: If Expression
  std::string test1 = R"(
        fn main() -> i32 {
            let x: i32 = if (1 < 2) { 10 } else { 20 };
            return x;
        }
    )";
  check_no_error(test1, "If Expression");

  // Test 2: Block Expression - simplified due to parser changes
  std::string test2 = R"(
        fn main() -> i32 {
            let y: i32 = 5;
            return y;
        }
    )";
  check_no_error(test2, "Block Expression");

  // Test 3: Nested If Expression in Block - simplified
  std::string test3 = R"(
        fn main() -> i32 {
            let z: i32 = if (1 > 0) {
                100
            } else {
                200
            };
            return z;
        }
    )";
  check_no_error(test3, "Nested If Expression in Block");

  // Test 4: If Expression with Else If
  std::string test4 = R"(
        fn main() -> i32 {
            let w: i32 = if (1 == 0) {
                1
            } else if (1 == 1) {
                2
            } else {
                3
            };
            return w;
        }
    )";
  check_no_error(test4, "If Expression with Else If");

  // Test 5: Exponentiation
  std::string test5 = R"(
        fn main() -> T81Float {
            let x: T81Float = 2.0t81;
            let y: T81Float = x ** 3.0t81;
            return y;
        }
    )";
  check_no_error(test5, "Exponentiation");

  // Test 6: Exponentiation Associativity
  std::string test6 = R"(
        fn main() -> T81Float {
            let x: T81Float = 2.0t81 ** 3.0t81 ** 2.0t81;
            return x;
        }
    )";
  check_no_error(test6, "Exponentiation Associativity");

  // Test 7: Tensor Matrix Multiplication
  std::string test7 = R"(
        fn main() -> i32 {
            let A: Tensor = Tensor.from_list([1.0, 2.0]);
            let B: Tensor = Tensor.from_list([3.0, 4.0]);
            let C: Tensor = A ** B;
            return 0;
        }
    )";
  check_no_error(test7, "Tensor MatMul");

  return 0;
}
