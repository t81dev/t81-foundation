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
    const std::string matching_tensor = R"(
        fn main() -> i32 {
            var a: Tensor[T81Int, 2, 3];
            var b: Tensor[T81Int, 2, 3];
            b = a;
            return 0;
        }
    )";
    expect_semantic_success(matching_tensor);

    const std::string mismatched_tensor = R"(
        fn main() -> i32 {
            var a: Tensor[T81Int, 2, 3];
            var b: Tensor[T81Int, 3, 3];
            b = a;
            return 0;
        }
    )";
    expect_semantic_failure(mismatched_tensor);

    const std::string runtime_constant = R"(
        let RANK: i32 = 3;
        fn main() -> i32 {
            var parametric: Tensor[T81Int, RANK];
            return 0;
        }
    )";
    expect_semantic_success(runtime_constant);

    std::cout << "Semantic analyzer generic tests passed!" << std::endl;
    return 0;
}
