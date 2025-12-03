#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

using namespace t81::frontend;

void expect_semantic_success(const std::string& source, const char* label = "<success fixture>") {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        std::cerr << "Parser failed while checking success fixture (" << label << ")" << std::endl;
        std::exit(1);
    }

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    if (analyzer.had_error()) {
        std::cerr << "Semantic analyzer reported error for success fixture (" << label << ")" << std::endl;
        std::exit(1);
    }
}

void expect_semantic_failure(const std::string& source, const char* label = "<failure fixture>") {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        return;
    }

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    if (!analyzer.had_error()) {
        std::cerr << "Expected semantic failure but analysis succeeded (" << label << ")" << std::endl;
        std::exit(1);
    }
}

struct AnalyzerFixture {
    std::vector<std::unique_ptr<Stmt>> statements;
    SemanticAnalyzer analyzer;

    explicit AnalyzerFixture(std::vector<std::unique_ptr<Stmt>> stmts)
        : statements(std::move(stmts)),
          analyzer(statements) {
        analyzer.analyze();
        if (analyzer.had_error()) {
            std::cerr << "Semantic analysis failed while building nested fixture" << std::endl;
            std::exit(1);
        }
    }
};

std::vector<std::unique_ptr<Stmt>> parse_statements(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        std::cerr << "Parser failed while analyzing nested fixture" << std::endl;
        std::exit(1);
    }
    return stmts;
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
    expect_semantic_success(loop_program, "loop_program");

    const std::string static_loop = R"(
        fn main() -> i32 {
            @bounded(5)
            loop {
                return 0;
            }
        }
    )";
    expect_semantic_success(static_loop, "static_loop");

    const std::string missing_annotation = R"(
        fn main() -> i32 {
            loop {
                return 0;
            }
        }
    )";
    expect_semantic_failure(missing_annotation, "missing_annotation");

    const std::string invalid_static = R"(
        fn main() -> i32 {
            @bounded(0)
            loop {
                return 0;
            }
        }
    )";
    expect_semantic_failure(invalid_static, "invalid_static");

    const std::string guard_loop = R"(
        fn main() -> i32 {
            var counter: i32 = 0;
            @bounded(loop(counter < 5))
            loop {
                counter = counter + 1;
                if (counter == 5) {
                    return counter;
                }
            }
        }
    )";
    expect_semantic_success(guard_loop, "guard_loop");

    const std::string invalid_guard = R"(
        fn main() -> i32 {
            var value: i32 = 0;
            @bounded(loop(value))
            loop {
                return value;
            }
        }
    )";
    expect_semantic_failure(invalid_guard, "invalid_guard");

    const std::string match_inside_loop = R"(
        fn main() -> i32 {
            @bounded(infinite)
            loop {
                let data: Option[i32] = Some(1);
                return match (data) {
                    Some(v) => v;
                    None => 0;
                };
            }
        }
    )";
    expect_semantic_success(match_inside_loop, "match_inside_loop");

    const std::string loop_inside_match = R"(
        fn run_forever(v: i32) -> i32 {
            @bounded(infinite)
            loop {
                return v;
            }
        }

        fn main() -> i32 {
            return match (Some(3)) {
                Some(v) => run_forever(v);
                None => 0;
            };
        }
    )";
    expect_semantic_success(loop_inside_match, "loop_inside_match");

    {
        AnalyzerFixture fixture(parse_statements(guard_loop));
        const auto& loops = fixture.analyzer.loop_metadata();
        if (loops.size() != 1) {
            std::cerr << "Expected one guard loop metadata entry but found " << loops.size() << std::endl;
            return 1;
        }
        const auto& guard_meta = loops[0];
        if (guard_meta.bound_kind != LoopStmt::BoundKind::Guarded ||
            !guard_meta.guard_present ||
            guard_meta.bound_value.has_value()) {
            std::cerr << "Guard loop metadata missing guard annotation" << std::endl;
            return 1;
        }
    }

    const std::string nested_match_loop = R"(
        fn main() -> i32 {
            var counter: i32 = 0;
            @bounded(infinite)
            loop {
                @bounded(loop(counter < 3))
                loop {
                    counter = counter + 1;
                    return match (Some(counter)) {
                        Some(v) => v;
                        None => 0;
                    };
                }
                @bounded(3)
                loop {
                    return counter;
                }
            }
        }
    )";
    expect_semantic_success(nested_match_loop, "nested_match_loop");
    {
        AnalyzerFixture fixture(parse_statements(nested_match_loop));
        const auto& loops = fixture.analyzer.loop_metadata();
        bool saw_infinite = false;
        bool saw_guard = false;
        bool saw_static = false;
        for (const auto& meta : loops) {
            if (meta.bound_kind == LoopStmt::BoundKind::Infinite) {
                saw_infinite = true;
            }
            if (meta.bound_kind == LoopStmt::BoundKind::Guarded) {
                saw_guard = true;
            }
            if (meta.bound_kind == LoopStmt::BoundKind::Static && meta.bound_value.value_or(0) == 3) {
                saw_static = true;
            }
        }
        if (loops.size() != 3 || !saw_infinite || !saw_guard || !saw_static) {
            std::cerr << "Nested match loop metadata did not record all variants" << std::endl;
            return 1;
        }
    }

    std::cout << "Semantic analyzer loop tests passed!" << std::endl;
    return 0;
}
