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
        // Parsing already failed; that's acceptable for this helper.
        return;
    }

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(analyzer.had_error());
}

int main() {
    const std::string valid_option = R"(
        fn make_option() -> Option[i32] {
            let value: Option[i32] = Some(1);
            return value;
        }

        fn main() -> i32 {
            let other: Option[i32] = make_option();
            return 0;
        }
    )";
    expect_semantic_success(valid_option);

    const std::string invalid_option = R"(
        fn bad_option() -> Option[i32] {
            let value: Option[i32] = Some(true);
            return value;
        }
    )";
    expect_semantic_failure(invalid_option);

    const std::string valid_result = R"(
        fn make_ok() -> Result[i32, T81String] {
            return Ok(7);
        }

        fn make_err() -> Result[i32, T81String] {
            return Err("boom");
        }
    )";
    expect_semantic_success(valid_result);

    const std::string invalid_result = R"(
        fn bad_err() -> Result[i32, T81String] {
            return Err(5);
        }
    )";
    expect_semantic_failure(invalid_result);

    const std::string none_without_context = R"(
        fn main() -> i32 {
            let missing = None;
            return 0;
        }
    )";
    // `None` must appear where a contextual `Option[T]` type exists.
    expect_semantic_failure(none_without_context);

    const std::string ok_without_context = R"(
        fn main() -> i32 {
            return Ok(2);
        }
    )";
    expect_semantic_failure(ok_without_context);

    const std::string err_without_context = R"(
        fn main() -> i32 {
            return Err("boom");
        }
    )";
    expect_semantic_failure(err_without_context);

    const std::string numeric_widening = R"(
        fn widen() -> i32 {
            let a: i8 = 1;
            let b: i32 = a + 2;
            return b;
        }

        fn fail_widen() -> i8 {
            let x: i2 = 1;
            return x + 1.5;
        }
    )";
    expect_semantic_failure(numeric_widening);

    std::cout << "Semantic analyzer option/result tests passed!" << std::endl;
    return 0;
}
