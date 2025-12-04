#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace t81::frontend;

void expect_semantic_success(const std::string& source, const char* label = "<success fixture>") {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        std::cerr << "[" << label << "] parser reported errors\n";
    }
    assert(!parser.had_error());

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(!analyzer.had_error());
}

void expect_semantic_failure(const std::string& source, const char* label = "<failure fixture>") {
    (void)label;
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        // Parsing already failed, acceptable for these fixtures.
        return;
    }

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(analyzer.had_error());
}

int main() {
    const std::string float_fraction_failure = R"(
        fn main() -> T81Float {
            return 1.20t81 + 22/7t81;
        }
    )";
    expect_semantic_failure(float_fraction_failure, "float_fraction_failure");

    const std::string bigint_float_success = R"(
        fn main() -> T81Float {
            let big: T81BigInt = 123456;
            let result: T81Float = big + 1.20t81;
            return result;
        }
    )";
    expect_semantic_success(bigint_float_success, "bigint_float_success");

    std::cout << "Semantic analyzer numeric rules tests passed!" << std::endl;
    return 0;
}
