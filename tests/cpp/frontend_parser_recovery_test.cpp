#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "t81/frontend/ast.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"

using namespace t81::frontend;

namespace {

struct CerrRedirect {
  CerrRedirect() : old_buf(std::cerr.rdbuf(buffer.rdbuf())) {}
  ~CerrRedirect() { std::cerr.rdbuf(old_buf); }

  std::string str() const { return buffer.str(); }

  std::ostringstream buffer;
  std::streambuf* old_buf = nullptr;
};

int count_substring(std::string_view haystack, std::string_view needle) {
  if (needle.empty()) return 0;
  int count = 0;
  std::size_t pos = 0;
  while ((pos = haystack.find(needle, pos)) != std::string_view::npos) {
    ++count;
    pos += needle.size();
  }
  return count;
}

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "frontend_parser_recovery_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

bool test_missing_paren_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let a: i32 = (1 + 2;
  let b: i32 = 3;
  return b;
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should flag malformed input")) return false;
  if (!expect(diagnostics.find("Expect ')' after expression.") != std::string::npos,
              "missing primary parse error")) {
    return false;
  }
  if (!expect(diagnostics.find("expected ')', found ';'") != std::string::npos,
              "missing expected-vs-found token detail")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect ';' after constant declaration.") == std::string::npos,
              "unexpected cascaded semicolon error")) {
    return false;
  }
  if (!expect(count_substring(diagnostics, "error:") == 1,
              "expected a single parse error after recovery")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level statement")) return false;
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "top-level statement should be a function")) return false;
  if (!expect(fn->body.size() == 2, "expected recovery to keep later statements")) return false;
  if (!expect(dynamic_cast<LetStmt*>(fn->body[0].get()) != nullptr,
              "first recovered statement should be let b")) {
    return false;
  }
  auto* let_stmt = dynamic_cast<LetStmt*>(fn->body[0].get());
  if (!expect(std::string(let_stmt->name.lexeme) == "b", "recovered let name should be 'b'")) {
    return false;
  }
  if (!expect(dynamic_cast<ReturnStmt*>(fn->body[1].get()) != nullptr,
              "second recovered statement should be return")) {
    return false;
  }

  return true;
}

bool test_match_arm_separator_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let maybe: Option[i32] = Some(1);
  return match (maybe) {
    Some(v) => v
    None => 0;
  };
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "match-recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should report missing match-arm separator")) return false;
  if (!expect(diagnostics.find("Expect ',' or ';' between match arms.") != std::string::npos,
              "missing match-arm separator diagnostic")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect '}' after match arms.") == std::string::npos,
              "unexpected closing-brace cascade in match")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level function in match recovery")) return false;
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "match recovery top-level should be a function")) return false;
  if (!expect(!fn->body.empty(), "match recovery function body should be non-empty")) return false;

  return true;
}

bool test_generic_trailing_comma_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let bad: Tensor[i32, 4, ] = 0;
  let good: i32 = 1;
  return good;
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "generic-recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should report trailing generic comma")) return false;
  if (!expect(diagnostics.find("Trailing comma in generic parameter list is not allowed.") !=
                  std::string::npos,
              "missing trailing-comma generic diagnostic")) {
    return false;
  }
  if (!expect(diagnostics.find("expected ']', found ']'") == std::string::npos,
              "unexpected noisy expected-vs-found self-report")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect expression.") == std::string::npos,
              "unexpected expression cascade from trailing generic comma")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level function in generic recovery"))
    return false;
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "generic recovery top-level should be a function")) return false;
  if (!expect(fn->body.size() == 3, "expected later statements to survive generic recovery")) {
    return false;
  }

  return true;
}

bool test_match_pattern_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let maybe: Option[i32] = Some(1);
  return match (maybe) {
    Some(,) => 1;
    None => 0;
  };
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "match-pattern-recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should report malformed match pattern")) return false;
  if (!expect(diagnostics.find("Expect pattern binding.") != std::string::npos,
              "missing malformed pattern diagnostic")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect '=>' after match arm pattern.") == std::string::npos,
              "unexpected follow-on arm syntax cascade")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level function in pattern recovery")) {
    return false;
  }
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "pattern recovery top-level should be a function")) return false;
  if (!expect(!fn->body.empty(), "pattern recovery function body should be non-empty")) {
    return false;
  }

  return true;
}

bool test_match_record_pattern_field_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let maybe: Option[i32] = Some(1);
  return match (maybe) {
    Some({value:}) => 1;
    None => 0;
  };
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "match-record-pattern-recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should report malformed record pattern binding")) {
    return false;
  }
  if (!expect(
          diagnostics.find("Expect binding name after ':' in record pattern.") != std::string::npos,
          "missing record-pattern binding diagnostic")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect ')' after match binding.") == std::string::npos,
              "unexpected closing-paren cascade after record-pattern recovery")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level function in record-pattern recovery")) {
    return false;
  }
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "record-pattern recovery top-level should be a function")) {
    return false;
  }
  if (!expect(!fn->body.empty(), "record-pattern recovery function body should be non-empty")) {
    return false;
  }

  return true;
}

bool test_match_tuple_pattern_tail_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let maybe: Option[i32] = Some(1);
  return match (maybe) {
    Some(a, ) => a;
    None => 0;
  };
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "match-tuple-pattern-recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should report malformed tuple pattern tail")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect binding identifier in tuple pattern.") != std::string::npos,
              "missing tuple-pattern binding diagnostic")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect '=>' after match arm pattern.") == std::string::npos,
              "unexpected fat-arrow cascade after tuple-pattern recovery")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level function in tuple-pattern recovery")) {
    return false;
  }
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "tuple-pattern recovery top-level should be a function")) {
    return false;
  }
  if (!expect(!fn->body.empty(), "tuple-pattern recovery function body should be non-empty")) {
    return false;
  }

  return true;
}

bool test_match_missing_binding_rparen_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let maybe: Option[i32] = Some(1);
  return match (maybe) {
    Some(v => v;
    None => 0;
  };
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "match-missing-rparen-recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should report missing ')' in match binding")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect ')' after match binding.") != std::string::npos,
              "missing match-binding ')' diagnostic")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect '=>' after match arm pattern.") == std::string::npos,
              "unexpected fat-arrow cascade after ')' recovery")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level function in missing-rparen recovery")) {
    return false;
  }
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "missing-rparen recovery top-level should be a function")) {
    return false;
  }
  if (!expect(!fn->body.empty(), "missing-rparen recovery function body should be non-empty")) {
    return false;
  }

  return true;
}

bool test_match_missing_guard_expression_recovery() {
  const std::string source = R"(
fn main() -> i32 {
  let maybe: Option[i32] = Some(1);
  return match (maybe) {
    Some(v) if => v;
    None => 0;
  };
}
)";

  CerrRedirect redirect;
  Lexer lexer(source);
  Parser parser(lexer, "match-missing-guard-recovery");
  auto stmts = parser.parse();
  const std::string diagnostics = redirect.str();

  if (!expect(parser.had_error(), "parser should report missing guard expression")) {
    return false;
  }
  if (!expect(
          diagnostics.find("Expect guard expression after 'if' in match arm.") != std::string::npos,
          "missing guard-expression diagnostic")) {
    return false;
  }
  if (!expect(diagnostics.find("Expect expression.") == std::string::npos,
              "unexpected expression cascade for missing guard")) {
    return false;
  }

  if (!expect(stmts.size() == 1, "expected one top-level function in guard recovery")) {
    return false;
  }
  auto* fn = dynamic_cast<FunctionStmt*>(stmts.front().get());
  if (!expect(fn != nullptr, "guard recovery top-level should be a function")) {
    return false;
  }
  if (!expect(!fn->body.empty(), "guard recovery function body should be non-empty")) {
    return false;
  }

  return true;
}

}  // namespace

int main() {
  if (!test_missing_paren_recovery()) return 1;
  if (!test_match_arm_separator_recovery()) return 1;
  if (!test_generic_trailing_comma_recovery()) return 1;
  if (!test_match_pattern_recovery()) return 1;
  if (!test_match_record_pattern_field_recovery()) return 1;
  if (!test_match_tuple_pattern_tail_recovery()) return 1;
  if (!test_match_missing_binding_rparen_recovery()) return 1;
  if (!test_match_missing_guard_expression_recovery()) return 1;
  return 0;
}
