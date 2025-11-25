#include "test_utils.hpp"
#include "t81/frontend/parser.hpp"

void run_test(const std::string& source, const std::string& expected) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    assert(stmts.size() == 1);

    AstPrinter printer;
    std::string result = printer.print(*stmts[0]);

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
    run_test("let x: Vector[T81Int] = 1;", "(let x: (generic Vector T81Int) 1)");
    run_test("let x: Option[T81Float] = 1;", "(let x: (generic Option T81Float) 1)");
    run_test("let x: Result[T81Int, Symbol] = 1;", "(let x: (generic Result T81Int Symbol) 1)");
    run_test("let x: Tensor[T81Int, 5] = 1;", "(let x: (generic Tensor T81Int 5) 1)");
    run_test("let x: Tensor[T81Int, 5, 10] = 1;", "(let x: (generic Tensor T81Int 5 10) 1)");

    return 0;
}
