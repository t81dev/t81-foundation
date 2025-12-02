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
    if (parser.had_error()) {
        std::cerr << "[" << label << "] parser reported errors\n";
    }
    assert(!parser.had_error());

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(!analyzer.had_error());
}

void expect_semantic_failure(const std::string& source, const char* label) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) {
        std::cerr << "[" << label << "] parser reported errors (expected)\n";
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
    expect_semantic_success(matching_tensor, "matching_tensor");

    const std::string mismatched_tensor = R"(
        fn main() -> i32 {
            var a: Tensor[T81Int, 2, 3];
            var b: Tensor[T81Int, 3, 3];
            b = a;
            return 0;
        }
    )";
    expect_semantic_failure(mismatched_tensor, "mismatched_tensor");

    const std::string runtime_constant = R"(
        let RANK: i32 = 3;
        fn main() -> i32 {
            var parametric: Tensor[T81Int, RANK];
            return 0;
        }
    )";
    expect_semantic_success(runtime_constant, "runtime_constant");

    const std::string option_inference = R"(
        fn main() -> i32 {
            let inferred: Option = Some(42);
            return 0;
        }
    )";
    expect_semantic_success(option_inference, "option_inference");

    const std::string option_inference_failure = R"(
        fn main() -> i32 {
            let inferred: Option = Some(true);
            let value: i32 = match (inferred) {
                Some(v) => v;
                None => 0;
            };
            return value;
        }
    )";
    expect_semantic_failure(option_inference_failure, "option_inference_failure");

    const std::string result_inference = R"(
        fn main() -> i32 {
            let inferred: Result = Ok(1);
            return 0;
        }
    )";
    expect_semantic_success(result_inference, "result_inference");

    const std::string result_inference_failure = R"(
        fn main() -> i32 {
            let inferred: Result = Ok("boom");
            let value: i32 = match (inferred) {
                Ok(v) => v;
                Err(_) => 0;
            };
            return value;
        }
    )";
    expect_semantic_failure(result_inference_failure, "result_inference_failure");

    const std::string vector_inference = R"(
        fn main() -> i32 {
            let inferred: Vector = Vector[T81Int, 4];
            let value: Vector[T81Int, 4] = inferred;
            return 0;
        }
    )";
    expect_semantic_success(vector_inference, "vector_inference");

    const std::string vector_inference_failure = R"(
        fn main() -> i32 {
            let inferred: Vector = Vector[T81Int, 4];
            let value: Vector[T81Float, 4] = inferred;
            return 0;
        }
    )";
    expect_semantic_failure(vector_inference_failure, "vector_inference_failure");

    const std::string tensor_inference = R"(
        fn main() -> i32 {
            let inferred: Tensor = Tensor[T81Int, 2, 2];
            let value: Tensor[T81Int, 2, 2] = inferred;
            return 0;
        }
    )";
    expect_semantic_success(tensor_inference, "tensor_inference");

    const std::string tensor_inference_failure = R"(
        fn main() -> i32 {
            let inferred: Tensor = Tensor[T81Int, 2, 2];
            let value: Tensor[T81Int, 2, 3] = inferred;
            return 0;
        }
    )";
    expect_semantic_failure(tensor_inference_failure, "tensor_inference_failure");

    const std::string custom_generic = R"(
        fn main() -> i32 {
            let inferred: Box = Box[i32, 4];
            let value: Box[i32, 4] = inferred;
            return 0;
        }
    )";
    expect_semantic_success(custom_generic, "custom_generic");

    const std::string custom_generic_failure = R"(
        fn main() -> i32 {
            let inferred: Box = Box[i32, 4];
            let value: Box[i32, 5] = inferred;
            return 0;
        }
    )";
    expect_semantic_failure(custom_generic_failure, "custom_generic_failure");

    const std::string tensor_match_inference = R"(
        fn main() -> i32 {
            let inferred: Option = Some(Tensor[T81Int, 2, 3]);
            let value: Tensor[T81Int, 2, 3] = match (inferred) {
                Some(t) => t;
                None => Tensor[T81Int, 2, 3];
            };
            return 0;
        }
    )";
    expect_semantic_success(tensor_match_inference, "tensor_match_inference");

    const std::string tensor_match_inference_failure = R"(
        fn main() -> i32 {
            let inferred: Option = Some(Tensor[T81Int, 2, 3]);
            let value: Tensor[T81Int, 3, 2] = match (inferred) {
                Some(t) => t;
                None => Tensor[T81Int, 3, 2];
            };
            return 0;
        }
    )";
    expect_semantic_failure(tensor_match_inference_failure, "tensor_match_inference_failure");

    const std::string tensor_loop_inference = R"(
        fn main() -> i32 {
            let inferred: Tensor = Tensor[T81Int, 2, 3];
            @bounded(infinite)
            loop {
                let value: Tensor[T81Int, 2, 3] = inferred;
                return 0;
            }
        }
    )";
    expect_semantic_success(tensor_loop_inference, "tensor_loop_inference");

    const std::string tensor_loop_inference_failure = R"(
        fn main() -> i32 {
            let inferred: Tensor = Tensor[T81Int, 2, 3];
            @bounded(infinite)
            loop {
                let value: Tensor[T81Int, 3, 3] = inferred;
                return 0;
            }
        }
    )";
    expect_semantic_failure(tensor_loop_inference_failure, "tensor_loop_inference_failure");

    std::cout << "Semantic analyzer generic tests passed!" << std::endl;
    return 0;
}
