#include "t81/frontend/parser.hpp"
#include "test_utils.hpp"

void run_test(const std::string& source, const std::string& expected) {
  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();

  assert(stmts.size() == 1);

  [[maybe_unused]] AstPrinter printer;
  [[maybe_unused]] std::string result = printer.print(*stmts[0]);

  if (result != expected) {
    std::cerr << "Parser generics test failed!" << std::endl;
    std::cerr << "  Source:   " << source << std::endl;
    std::cerr << "  Expected: " << expected << std::endl;
    std::cerr << "  Actual:   " << result << std::endl;
    exit(1);
  }
  std::cout << "Parser generics test passed: " << source << std::endl;
}

int main() {
  run_test("let x: Vector[T81Int] = 1;", "(let x: (generic Vector T81Int) = 1)");
  run_test("let x: Option[T81Float] = 1;", "(let x: (generic Option T81Float) = 1)");
  run_test("let x: Result[T81Int, Symbol] = 1;", "(let x: (generic Result T81Int Symbol) = 1)");
  run_test("let x: Tensor[T81Int, 5] = 1;", "(let x: (generic Tensor T81Int 5) = 1)");
  run_test("let x: Tensor[T81Int, 5, 10] = 1;", "(let x: (generic Tensor T81Int 5 10) = 1)");
  run_test("fn id[T](x: T) -> T { return x; }", "(fn id [T] (x: T ) -> T (block (return x)))");
  run_test("fn main() -> i32 { return id[i32](7); }",
           "(fn main ( ) -> i32 (block (return (call (generic id i32) 7))))");

  return 0;
}
