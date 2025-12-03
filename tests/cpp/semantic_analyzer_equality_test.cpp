#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace t81::frontend;

void expect_semantic_success(const std::string& source, const char* label) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    assert(!parser.had_error() && label);

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(!analyzer.had_error() && label);
}

void expect_semantic_failure(const std::string& source, const char* label) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) return;

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(analyzer.had_error() && label);
}

int main() {
    const std::string valid_equality = R"(
        fn main() -> bool {
            return 1 == 2;
        }
    )";
    expect_semantic_success(valid_equality, "valid_equality");

    const std::string invalid_equality = R"(
        fn main() -> bool {
            return 1 == true;
        }
    )";
    expect_semantic_failure(invalid_equality, "invalid_equality");

    std::cout << "Semantic analyzer equality tests passed!" << std::endl;
    return 0;
}
