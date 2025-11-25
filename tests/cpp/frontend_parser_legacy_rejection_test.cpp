#include "t81/frontend/parser.hpp"
#include <iostream>
#include <cassert>

using namespace t81::frontend;

void run_rejection_test(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer);
    parser.parse();

    if (!parser.had_error()) {
        std::cerr << "Parser legacy rejection test failed!" << std::endl;
        std::cerr << "  Source:   " << source << std::endl;
        std::cerr << "  Expected: Parser error" << std::endl;
        std::cerr << "  Actual:   No error" << std::endl;
        exit(1);
    }
    std::cout << "Parser legacy rejection test passed: " << source << std::endl;
}

int main() {
    run_rejection_test("let x: Vector<T81Int> = 1;");
    run_rejection_test("fn foo(bar: Option<T81Float>) {}");

    return 0;
}
