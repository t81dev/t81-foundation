#include <iostream>
#include <string>

#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

using namespace t81::frontend;

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "semantic_analyzer_diagnostic_location_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  const std::string source = R"(
record User {
  active: i32;
};

fn main() -> i32 {
  let u: User = User { active: 1 };
  if (u.active) {
    return 1;
  }
  return 0;
}
)";

  Lexer lexer(source);
  Parser parser(lexer, "diag-location");
  auto stmts = parser.parse();
  if (!expect(!parser.had_error(), "unexpected parser error")) return 1;

  SemanticAnalyzer analyzer(stmts, "diag-location");
  analyzer.analyze();
  if (!expect(analyzer.had_error(), "expected semantic failure did not occur")) return 1;

  const auto& diagnostics = analyzer.diagnostics();
  if (!expect(!diagnostics.empty(), "expected at least one diagnostic")) return 1;

  bool found_condition_error = false;
  for (const auto& diag : diagnostics) {
    if (diag.message.find("Condition expression '") != std::string::npos &&
        diag.message.find("must be bool") != std::string::npos) {
      found_condition_error = true;
      if (!expect(diag.line == 8, "condition diagnostic should report the correct source line")) {
        return 1;
      }
      if (!expect(diag.column == 9, "condition diagnostic should point at field token column")) {
        return 1;
      }
    }
  }

  if (!expect(found_condition_error, "missing bool-condition diagnostic")) return 1;
  return 0;
}
