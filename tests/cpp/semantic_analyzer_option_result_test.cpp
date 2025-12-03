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

void expect_semantic_failure(const std::string& source,
                             const char* label = "<failure fixture>",
                             const std::string& expected_error = "") {
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
    if (!expected_error.empty()) {
        for (const auto& diag : analyzer.diagnostics()) {
            if (diag.message.find(expected_error) != std::string::npos) {
                return;
            }
        }
        std::cerr << "Test '" << label << "' failed. Expected error containing: '" << expected_error
                  << "', but no matching diagnostic was found." << std::endl;
        std::exit(1);
    }
}

int main() {
#if defined(_WIN32) || defined(_WIN64)
    std::cout << "Semantic analyzer option/result tests skipped on Windows.\n";
    return 0;
#else
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
            let missing = None();
            return 0;
        }
    )";
    // `None` must appear where a contextual `Option[T]` type exists.
    expect_semantic_failure(none_without_context, "none_without_context", "requires a contextual Option[T] type");

    const std::string ok_without_context = R"(
        fn main() -> i32 {
            return Ok(2);
        }
    )";
    expect_semantic_failure(ok_without_context, "ok_without_context", "requires a contextual Result[T, E] type");

    const std::string err_without_context = R"(
        fn main() -> i32 {
            return Err("boom");
        }
    )";
    expect_semantic_failure(err_without_context, "err_without_context", "requires a contextual Result[T, E] type");

    const std::string numeric_widening_success = R"(
        fn widen() -> i32 {
            let a: i8 = 1;
            let b: i32 = a + 2;
            return b;
        }
    )";
    expect_semantic_success(numeric_widening_success);

    const std::string numeric_widening_failure = R"(
        fn fail_widen() -> i8 {
            let x: i2 = 1;
            return x + 1.5;
        }
    )";
    expect_semantic_failure(numeric_widening_failure);

    const std::string int_float_success = R"(
        fn widen_float() -> T81Float {
            let value: i8 = 3;
            let result: T81Float = value + 1.20t81;
            return result;
        }
    )";
    expect_semantic_success(int_float_success);

    const std::string invalid_modulo = R"(
        fn bad_mod() -> i32 {
            return 1.5 % 2.0;
        }
    )";
    expect_semantic_failure(invalid_modulo);

    const std::string float_fraction_mix = R"(
        fn bad_mix() -> T81Float {
            // Mixing T81Fraction literals with float arithmetic should fail.
            return 22/7t81 + 1.20t81;
        }
    )";
    expect_semantic_failure(float_fraction_mix);

    const std::string bool_arith = R"(
        fn bool_add() -> i32 {
            return 1 + true;
        }
    )";
    expect_semantic_failure(bool_arith);

    const std::string nested_option_success = R"(
        fn nested_option() -> Option[Option[i32]] {
            return Some(Some(7));
        }
    )";
    expect_semantic_success(nested_option_success);

    const std::string nested_option_result_success = R"(
        fn nested_option_result() -> Option[Result[Option[i32], T81String]] {
            return Some(Ok(Some(5)));
        }
    )";
    expect_semantic_success(nested_option_result_success);

    const std::string nested_option_result_failure = R"(
        fn nested_option_result_bad() -> Option[Result[Option[i32], T81String]] {
            return Some(Err(Some(5)));
        }
    )";
    expect_semantic_failure(nested_option_result_failure);

    std::cout << "Semantic analyzer option/result tests passed!" << std::endl;
    return 0;
#endif
}
