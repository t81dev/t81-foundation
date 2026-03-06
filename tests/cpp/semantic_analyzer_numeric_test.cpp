#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <iostream>
#include <string>

using namespace t81::frontend;

bool expect_semantic_success(const std::string& source, const char* label = "<success fixture>") {
  auto expect = [](bool cond, const char* msg) -> bool {
    if (!cond) {
      std::cerr << "semantic_analyzer_numeric_test failure: " << msg << "\n";
      return false;
    }
    return true;
  };

  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();
  if (parser.had_error()) {
    std::cerr << "[" << label << "] parser reported errors\n";
  }
  if (!expect(!parser.had_error(), "unexpected parser error in success fixture")) return false;

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  if (!expect(!analyzer.had_error(), "unexpected semantic error in success fixture")) return false;
  return true;
}

bool expect_semantic_failure(const std::string& source, const char* label = "<failure fixture>") {
  auto expect = [](bool cond, const char* msg) -> bool {
    if (!cond) {
      std::cerr << "semantic_analyzer_numeric_test failure: " << msg << "\n";
      return false;
    }
    return true;
  };

  Lexer lexer(source);
  Parser parser(lexer);
  [[maybe_unused]] auto stmts = parser.parse();
  if (parser.had_error()) {
    // Parsing already failed, acceptable for these fixtures.
    return true;
  }

  SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  if (!expect(analyzer.had_error(), "expected semantic failure did not occur")) return false;
  (void)label;
  return true;
}

int main() {
  const std::string float_fraction_failure = R"(
        fn main() -> T81Float {
            return 1.20t81 + 22/7t81;
        }
    )";
  if (!expect_semantic_failure(float_fraction_failure, "float_fraction_failure")) return 1;

  const std::string bigint_float_success = R"(
        fn main() -> T81Float {
            let big: T81BigInt = 123456;
            let result: T81Float = big + 1.20t81;
            return result;
        }
    )";
  if (!expect_semantic_success(bigint_float_success, "bigint_float_success")) return 1;

  const std::string t81_integer_literal_success = R"(
        fn main() -> T81BigInt {
            let small: T81BigInt = 42t81;
            let big: T81BigInt = 9223372036854775808t81;
            return big + small;
        }
    )";
  if (!expect_semantic_success(t81_integer_literal_success, "t81_integer_literal_success")) {
    return 1;
  }

  const std::string contextual_bigint_integer_success = R"(
        fn main() -> T81BigInt {
            let small: T81BigInt = 7;
            let via_call: T81BigInt = std.math.bigint.from_int(9);
            return small + via_call;
        }
    )";
  if (!expect_semantic_success(contextual_bigint_integer_success,
                               "contextual_bigint_integer_success")) {
    return 1;
  }

  const std::string qutrit_arith_success = R"(
        fn main() -> i32 {
            let q: T81Qutrit = 1;
            let r: T81Qutrit = q + 1;
            return 0;
        }
    )";
  if (!expect_semantic_success(qutrit_arith_success, "qutrit_arith_success")) return 1;

  const std::string uint_arith_success = R"(
        fn main() -> i32 {
            let u: T81Uint = 7;
            let v: T81Uint = u + 1;
            return 0;
        }
    )";
  if (!expect_semantic_success(uint_arith_success, "uint_arith_success")) return 1;

  const std::string uint_subtract_promotes_to_bigint = R"(
        fn main() -> i32 {
            let u: T81Uint = 7;
            let d: T81BigInt = u - 9;
            return 0;
        }
    )";
  if (!expect_semantic_success(uint_subtract_promotes_to_bigint,
                               "uint_subtract_promotes_to_bigint")) {
    return 1;
  }

  const std::string uint_subtract_uint_failure = R"(
        fn main() -> i32 {
            let u: T81Uint = 7;
            let d: T81Uint = u - 1;
            return 0;
        }
    )";
  if (!expect_semantic_failure(uint_subtract_uint_failure, "uint_subtract_uint_failure")) return 1;

  const std::string fixed_arith_success = R"(
        fn main() -> i32 {
            let x: T81Fixed[8, 4] = 1;
            let y: T81Fixed[8, 4] = x + 2;
            return 0;
        }
    )";
  if (!expect_semantic_success(fixed_arith_success, "fixed_arith_success")) return 1;

  const std::string complex_arith_success = R"(
        fn main() -> i32 {
            var c1: T81Complex[18];
            var c2: T81Complex[18];
            c2 = c1 + c1;
            return 0;
        }
    )";
  if (!expect_semantic_success(complex_arith_success, "complex_arith_success")) return 1;

  const std::string complex_int_failure = R"(
        fn main() -> i32 {
            var c: T81Complex[18];
            let bad = c + 1;
            return 0;
        }
    )";
  if (!expect_semantic_failure(complex_int_failure, "complex_int_failure")) return 1;

  const std::string uint_unary_minus_failure = R"(
        fn main() -> i32 {
            let u: T81Uint = 7;
            let bad = -u;
            return 0;
        }
    )";
  if (!expect_semantic_failure(uint_unary_minus_failure, "uint_unary_minus_failure")) return 1;

  const std::string uint_negative_constant_failure = R"(
        fn main() -> i32 {
            let u: T81Uint = -1;
            return 0;
        }
    )";
  if (!expect_semantic_failure(uint_negative_constant_failure, "uint_negative_constant_failure")) {
    return 1;
  }

  const std::string uint_negative_expr_failure = R"(
        fn main() -> i32 {
            let u: T81Uint = 1 - 2;
            return 0;
        }
    )";
  if (!expect_semantic_failure(uint_negative_expr_failure, "uint_negative_expr_failure")) {
    return 1;
  }

  const std::string qutrit_out_of_range_constant_failure = R"(
        fn main() -> i32 {
            let q: T81Qutrit = 2;
            return 0;
        }
    )";
  if (!expect_semantic_failure(qutrit_out_of_range_constant_failure,
                               "qutrit_out_of_range_constant_failure")) {
    return 1;
  }

  const std::string qutrit_assignment_out_of_range_failure = R"(
        fn main() -> i32 {
            var q: T81Qutrit = 1;
            q = 3;
            return 0;
        }
    )";
  if (!expect_semantic_failure(qutrit_assignment_out_of_range_failure,
                               "qutrit_assignment_out_of_range_failure")) {
    return 1;
  }

  const std::string qutrit_expr_out_of_range_failure = R"(
        fn main() -> i32 {
            let q: T81Qutrit = 1 + 1;
            return 0;
        }
    )";
  if (!expect_semantic_failure(qutrit_expr_out_of_range_failure,
                               "qutrit_expr_out_of_range_failure")) {
    return 1;
  }

  const std::string qutrit_conversion_success = R"(
        fn main() -> i32 {
            let q: T81Qutrit = T81Qutrit(1);
            return 0;
        }
    )";
  if (!expect_semantic_success(qutrit_conversion_success, "qutrit_conversion_success")) return 1;

  const std::string qutrit_conversion_out_of_range_failure = R"(
        fn main() -> i32 {
            let q: T81Qutrit = T81Qutrit(2);
            return 0;
        }
    )";
  if (!expect_semantic_failure(qutrit_conversion_out_of_range_failure,
                               "qutrit_conversion_out_of_range_failure")) {
    return 1;
  }

  const std::string uint_conversion_success = R"(
        fn main() -> i32 {
            let u: T81Uint = T81Uint(42);
            return 0;
        }
    )";
  if (!expect_semantic_success(uint_conversion_success, "uint_conversion_success")) return 1;

  const std::string uint_conversion_negative_failure = R"(
        fn main() -> i32 {
            let u: T81Uint = T81Uint(-2);
            return 0;
        }
    )";
  if (!expect_semantic_failure(uint_conversion_negative_failure,
                               "uint_conversion_negative_failure")) {
    return 1;
  }

  const std::string uint_conversion_negative_expr_failure = R"(
        fn main() -> i32 {
            let u: T81Uint = T81Uint(1 - 3);
            return 0;
        }
    )";
  if (!expect_semantic_failure(uint_conversion_negative_expr_failure,
                               "uint_conversion_negative_expr_failure")) {
    return 1;
  }

  const std::string uint_conversion_non_integer_failure = R"(
        fn main() -> i32 {
            let u: T81Uint = T81Uint(1.25);
            return 0;
        }
    )";
  if (!expect_semantic_failure(uint_conversion_non_integer_failure,
                               "uint_conversion_non_integer_failure")) {
    return 1;
  }

  const std::string fixed_constructor_success = R"(
        fn main() -> i32 {
            let f: T81Fixed[8, 4] = T81Fixed[8, 4](1);
            return 0;
        }
    )";
  if (!expect_semantic_success(fixed_constructor_success, "fixed_constructor_success")) return 1;

  const std::string fixed_constructor_arity_failure = R"(
        fn main() -> i32 {
            let f: T81Fixed[8, 4] = T81Fixed[8, 4](1, 2);
            return 0;
        }
    )";
  if (!expect_semantic_failure(fixed_constructor_arity_failure,
                               "fixed_constructor_arity_failure")) {
    return 1;
  }

  const std::string complex_constructor_success = R"(
        fn main() -> i32 {
            let c: T81Complex[18] = T81Complex[18](1, -1);
            return 0;
        }
    )";
  if (!expect_semantic_success(complex_constructor_success, "complex_constructor_success")) {
    return 1;
  }

  const std::string complex_constructor_non_numeric_failure = R"(
        fn main() -> i32 {
            let c: T81Complex[18] = T81Complex[18](1, "bad");
            return 0;
        }
    )";
  if (!expect_semantic_failure(complex_constructor_non_numeric_failure,
                               "complex_constructor_non_numeric_failure")) {
    return 1;
  }

  std::cout << "Semantic analyzer numeric rules tests passed!" << std::endl;
  return 0;
}
