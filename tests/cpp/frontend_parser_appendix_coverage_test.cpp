#include <iostream>
#include <string>
#include <string_view>

#include "t81/frontend/parser.hpp"

using namespace t81::frontend;

namespace {

bool expect_parse_ok(std::string_view source, const char* label) {
  std::string text(source);
  Lexer lexer(text);
  Parser parser(lexer, label);
  auto stmts = parser.parse();
  if (parser.had_error()) {
    std::cerr << "Parse should succeed but failed: " << label << "\n";
    return false;
  }
  if (stmts.empty()) {
    std::cerr << "Parse produced no statements: " << label << "\n";
    return false;
  }
  return true;
}

bool test_top_level_declarations() {
  constexpr const char* type_and_fn_source = R"(
type Vec3[T] = Tensor[T, 3];
fn id[T](x: T) -> T {
  return x;
}
)";
  constexpr const char* record_source = R"(
record Point {
  x: i32;
  y: i32;
};
)";
  constexpr const char* enum_source = R"(
enum MaybeInt {
  Some(i32);
  None;
};
)";
  return expect_parse_ok(type_and_fn_source, "appendix_top_level_type_and_fn") &&
         expect_parse_ok(record_source, "appendix_top_level_record") &&
         expect_parse_ok(enum_source, "appendix_top_level_enum");
}

bool test_statement_forms() {
  constexpr const char* source = R"(
fn statements() -> i32 {
  let x: i32 = 1;
  var y: i32;
  y = x + 1;
  if (y > 0) {
    y = y + 1;
  } else {
    y = y - 1;
  }
  @bounded(4)
  loop {
    y = y + 1;
    if (y > 6) {
      return y;
    }
  }
  return y;
}
)";
  return expect_parse_ok(source, "appendix_statement_forms");
}

bool test_expression_forms() {
  constexpr const char* source = R"(
fn expressions() -> i32 {
  let v: Vector[T81Int] = [1, 2, 3];
  let x: i32 = 1;
  let y: i32 = 7;
  let if_expr: i32 = if (x > 0) { 10 } else { 20 };
  let if_chain: i32 = if (x > 1) { 30 } else if (x == 1) { 40 } else { 50 };
  let m: Option[i32] = Some(3);
  let out: i32 = match (m) {
    Some(n) => n,
    None => 0,
  };
  return ((out + x + y + if_expr + if_chain) << 1) >>> 1;
}
)";
  return expect_parse_ok(source, "appendix_expression_forms");
}

}  // namespace

int main() {
  if (!test_top_level_declarations()) return 1;
  if (!test_statement_forms()) return 1;
  if (!test_expression_forms()) return 1;
  std::cout << "frontend_parser_appendix_coverage_test passed!\n";
  return 0;
}
