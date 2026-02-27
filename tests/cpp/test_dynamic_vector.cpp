#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "test_runtime_check.hpp"

using namespace t81::frontend;

void test_dynamic_vector_ir_gen() {
  // We want to compile `var a = 1; var b = [a, 2];` and see if it generates IR without error.

  std::string source = R"(
        fn main() {
            var a = 1;
            var b = [a, 2];
        }
    )";

  Lexer lexer(source);
  Parser parser(lexer);
  auto statements = parser.parse();

  SemanticAnalyzer analyzer(statements);
  analyzer.analyze();

  T81_TEST_CHECK(!analyzer.had_error());

  IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);

  try {
    auto program = ir_gen.generate(statements);
    // If we reach here without exception, it means IR generation succeeded for dynamic vector.
    T81_TEST_CHECK(true);
  } catch (const std::exception& e) {
    std::cerr << "IR Generation failed: " << e.what() << std::endl;
    T81_TEST_CHECK(false);
  }
}

int main() {
  try {
    test_dynamic_vector_ir_gen();
    std::cout << "Dynamic vector IR generation test passed!\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
