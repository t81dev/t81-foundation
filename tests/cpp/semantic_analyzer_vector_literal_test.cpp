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
    assert(!parser.had_error() && "Parser failed");

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
    const std::string simple_vector = R"(
        fn main() -> i32 {
            let v: Vector[i32] = [1, 2, 3];
            return 0;
        }
    )";
    expect_semantic_success(simple_vector, "simple_vector");

    const std::string float_vector = R"(
        fn main() -> i32 {
            let v: Vector[Float] = [1, 2.5];
            return 0;
        }
    )";
    expect_semantic_success(float_vector, "float_vector");

    const std::string no_context = R"(
        fn main() -> i32 {
            let v = [];
            return 0;
        }
    )";
    expect_semantic_failure(no_context, "no_context");

    std::cout << "Semantic analyzer vector literal tests passed!" << std::endl;
    return 0;
}
