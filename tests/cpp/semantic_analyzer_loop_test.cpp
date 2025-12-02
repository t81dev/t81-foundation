#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace t81::frontend;

void expect_semantic_success(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    assert(!parser.had_error());

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(!analyzer.had_error());
}

void expect_semantic_failure(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        return;
    }

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(analyzer.had_error());
}

int main() {
    const std::string loop_program = R"(
        fn main() -> i32 {
            @bounded(infinite)
            loop {
                return 0;
            }
        }
    )";
    expect_semantic_success(loop_program);

    const std::string static_loop = R"(
        fn main() -> i32 {
            @bounded(5)
            loop {
                return 0;
            }
        }
    )";
    expect_semantic_success(static_loop);

    const std::string missing_annotation = R"(
        fn main() -> i32 {
            loop {
                return 0;
            }
        }
    )";
    expect_semantic_failure(missing_annotation);

    const std::string invalid_static = R"(
        fn main() -> i32 {
            @bounded(0)
            loop {
                return 0;
            }
        }
    )";
    expect_semantic_failure(invalid_static);

    std::cout << "Semantic analyzer loop tests passed!" << std::endl;
    return 0;
}
