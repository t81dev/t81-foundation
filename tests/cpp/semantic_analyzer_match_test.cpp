#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace t81::frontend;

void expect_semantic_success(const std::string& source, const char* label = "semantic_match_success") {
    Lexer lexer(source);
    const std::string diag = label ? label : "<source>";
    Parser parser(lexer, diag);
    auto stmts = parser.parse();
    assert(!parser.had_error());

    SemanticAnalyzer analyzer(stmts, diag);
    analyzer.analyze();
    assert(!analyzer.had_error());
}

void expect_semantic_failure(const std::string& source, const char* label = "semantic_match_failure") {
    Lexer lexer(source);
    const std::string diag = label ? label : "<source>";
    Parser parser(lexer, diag);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        return;
    }

    SemanticAnalyzer analyzer(stmts, diag);
    analyzer.analyze();
    assert(analyzer.had_error());
}

int main() {
#if defined(_WIN32) || defined(_WIN64)
    std::cout << "Semantic analyzer match tests skipped on Windows.\n";
    return 0;
#else
    const std::string option_match = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = Some(10);
            let value: i32 = match (maybe) {
                Some(v) => v + 1;
                None => 0;
            };
            return value;
        }
    )";
    expect_semantic_success(option_match);

    const std::string missing_none = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = Some(1);
            match (maybe) {
                Some(v) => v;
            };
            return 0;
        }
    )";
    expect_semantic_failure(missing_none);

    const std::string missing_some = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = None;
            match (maybe) {
                None => 0;
            };
            return 0;
        }
    )";
    expect_semantic_failure(missing_some);

    const std::string duplicate_some = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = Some(1);
            match (maybe) {
                Some(v) => v;
                Some(w) => w;
                None => 0;
            };
            return 0;
        }
    )";
    expect_semantic_failure(duplicate_some);

    const std::string invalid_option_variant = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = Some(2);
            match (maybe) {
                Ok(v) => v;
                None => 0;
            };
            return 0;
        }
    )";
    expect_semantic_failure(invalid_option_variant);

    const std::string mismatched_arm = R"(
        fn main() -> i32 {
            let maybe: Option[i32] = Some(1);
            let result: i32 = match (maybe) {
                Some(v) => v;
                None => true;
            };
            return result;
        }
    )";
    expect_semantic_failure(mismatched_arm);

    const std::string invalid_scrutinee = R"(
        fn main() -> i32 {
            let value: i32 = match (1) {
                Some(v) => v;
                None => 0;
            };
            return value;
        }
    )";
    expect_semantic_failure(invalid_scrutinee);

    const std::string result_match = R"(
        fn main() -> Result[i32, T81String] {
            let data: Result[i32, T81String] = Ok(5);
            return match (data) {
                Ok(v) => Ok(v + 1);
                Err(e) => Err(e);
            };
        }
    )";
    expect_semantic_success(result_match);

    const std::string missing_err = R"(
        fn main() -> Result[i32, T81String] {
            let data: Result[i32, T81String] = Ok(5);
            match (data) {
                Ok(v) => Ok(v);
            };
            return Err("boom");
        }
    )";
    expect_semantic_failure(missing_err);

    const std::string missing_ok = R"(
        fn main() -> Result[i32, T81String] {
            let data: Result[i32, T81String] = Err("boom");
            match (data) {
                Err(e) => Err(e);
            };
            return Ok(0);
        }
    )";
    expect_semantic_failure(missing_ok);

    const std::string duplicate_err = R"(
        fn main() -> Result[i32, T81String] {
            let data: Result[i32, T81String] = Ok(5);
            match (data) {
                Ok(v) => Ok(v);
                Err(e) => Err(e);
                Err(e2) => Err(e2);
            };
            return Ok(0);
        }
    )";
    expect_semantic_failure(duplicate_err);

    const std::string invalid_result_variant = R"(
        fn main() -> Result[i32, T81String] {
            let data: Result[i32, T81String] = Ok(5);
            match (data) {
                Some(v) => Ok(v);
                Err(e) => Err(e);
            };
            return Ok(0);
        }
    )";
    expect_semantic_failure(invalid_result_variant);

    const std::string enum_success = R"(
        enum Signal {
            Red;
            Green;
            Data(i32);
        }

        fn main() -> i32 {
            var signal: Signal;
            let value: i32 = match (signal) {
                Red => 1;
                Green => 2;
                Data(v) => v;
            };
            return value;
        }
    )";
    expect_semantic_success(enum_success, "enum_match_success");

    const std::string enum_missing_variant = R"(
        enum Signal {
            Red;
            Green;
        }

        fn main() -> i32 {
            var signal: Signal;
            match (signal) {
                Red => 1;
            };
            return 0;
        }
    )";
    expect_semantic_failure(enum_missing_variant, "enum_match_missing_variant");

    const std::string enum_binding_error = R"(
        enum Color {
            Red;
            Blue;
        }

        fn main() -> i32 {
            var color: Color;
            match (color) {
                Red(value) => value;
                Blue => 0;
            };
            return 0;
        }
    )";
    expect_semantic_failure(enum_binding_error, "enum_match_binding_error");

    std::cout << "Semantic analyzer match tests passed!" << std::endl;
    return 0;
#endif
}
