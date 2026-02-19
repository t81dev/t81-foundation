#include <iostream>
#include <string>
#include <vector>

#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

using namespace t81::frontend;

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "semantic_analyzer_diagnostic_precision_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

std::vector<Diagnostic> analyze_source(const std::string& source, const char* label) {
  Lexer lexer(source);
  Parser parser(lexer, label);
  auto stmts = parser.parse();
  if (parser.had_error()) {
    std::cerr << "unexpected parser error in " << label << "\n";
    return {};
  }

  SemanticAnalyzer analyzer(stmts, label);
  analyzer.analyze();
  return analyzer.diagnostics();
}

bool has_message(const std::vector<Diagnostic>& diagnostics, std::string_view needle) {
  for (const auto& diag : diagnostics) {
    if (diag.message.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  const std::string field_access_source = R"(
fn main() -> i32 {
  let x: i32 = 1;
  let y: i32 = x.value;
  return y;
}
)";

  const auto field_diags = analyze_source(field_access_source, "field_precision");
  if (!expect(has_message(field_diags, "Field access requires a record value, found 'i32'."),
              "missing precise field-access type diagnostic")) {
    return 1;
  }

  const std::string enum_ctor_source = R"(
enum Wrap {
  Item(i32);
};

fn main() -> i32 {
  let w: Wrap = Wrap.Item(true);
  return 0;
}
)";

  const auto enum_diags = analyze_source(enum_ctor_source, "enum_ctor_precision");
  if (!expect(has_message(
                  enum_diags,
                  "Argument mismatch for enum constructor 'Item': expected 'i32' but got 'bool'."),
              "missing precise enum-constructor mismatch diagnostic")) {
    return 1;
  }

  const std::string match_type_source = R"(
fn main() -> i32 {
  let m: Option[i32] = Some(1);
  return match (m) {
    Some(v) => v;
    None => true;
  };
}
)";

  const auto match_diags = analyze_source(match_type_source, "match_precision");
  if (!expect(has_message(match_diags, "All match arms must produce the same type: expected 'i32' "
                                       "but got 'bool' for arm 'None'."),
              "missing precise match-arm type mismatch diagnostic")) {
    return 1;
  }

  const std::string some_ctor_source = R"(
fn main() -> i32 {
  let maybe: Option[i32] = Some(true);
  return 0;
}
)";

  const auto some_diags = analyze_source(some_ctor_source, "some_ctor_precision");
  if (!expect(has_message(some_diags, "The 'Some' constructor argument must match the contextual "
                                      "Option payload: expected 'i32' but got 'bool'."),
              "missing precise Some-constructor mismatch diagnostic")) {
    return 1;
  }

  const std::string ok_ctor_source = R"(
fn main() -> i32 {
  let res: Result[i32, bool] = Ok(true);
  return 0;
}
)";

  const auto ok_ctor_diags = analyze_source(ok_ctor_source, "ok_ctor_precision");
  if (!expect(
          has_message(ok_ctor_diags, "The 'Ok' constructor argument must match the success type of "
                                     "the contextual Result: expected 'i32' but got 'bool'."),
          "missing precise Ok-constructor mismatch diagnostic")) {
    return 1;
  }

  const std::string err_ctor_source = R"(
fn main() -> i32 {
  let res: Result[i32, bool] = Err(1);
  return 0;
}
)";

  const auto err_ctor_diags = analyze_source(err_ctor_source, "err_ctor_precision");
  if (!expect(
          has_message(err_ctor_diags, "The 'Err' constructor argument must match the error type of "
                                      "the contextual Result: expected 'bool' but got 'i32'."),
          "missing precise Err-constructor mismatch diagnostic")) {
    return 1;
  }

  const std::string logical_not_source = R"(
fn main() -> i32 {
  let x: i32 = 1;
  if (!x) {
    return 1;
  }
  return 0;
}
)";

  const auto logical_not_diags = analyze_source(logical_not_source, "logical_not_precision");
  if (!expect(has_message(logical_not_diags, "Logical not requires a boolean operand, got 'i32'."),
              "missing precise logical-not operand diagnostic")) {
    return 1;
  }

  const std::string immutable_index_source = R"(
fn main() -> i32 {
  let xs: Vector[i32] = [1, 2, 3];
  xs[0] = 9;
  return 0;
}
)";

  const auto immutable_index_diags =
      analyze_source(immutable_index_source, "immutable_index_precision");
  if (!expect(has_message(immutable_index_diags,
                          "Cannot assign to immutable index expression 'xs[0]'."),
              "missing precise immutable-index target diagnostic")) {
    return 1;
  }

  const std::string index_type_source = R"(
fn main() -> i32 {
  let xs: Vector[i32] = [1, 2, 3];
  let flag: bool = true;
  let y: i32 = xs[flag];
  return y;
}
)";

  const auto index_type_diags = analyze_source(index_type_source, "index_type_precision");
  if (!expect(has_message(
                  index_type_diags,
                  "Index expression 'flag' for target 'xs' must be an integer type, got 'bool'."),
              "missing precise index-type diagnostic with expression context")) {
    return 1;
  }

  const std::string assign_mismatch_source = R"(
fn main() -> i32 {
  var x: i32 = 0;
  x = true;
  return 0;
}
)";

  const auto assign_mismatch_diags =
      analyze_source(assign_mismatch_source, "assign_mismatch_precision");
  if (!expect(has_message(
                  assign_mismatch_diags,
                  "Cannot assign expression 'true' of type 'bool' to target 'x' of type 'i32'."),
              "missing precise assignment mismatch diagnostic with source/target context")) {
    return 1;
  }

  const std::string index_unsupported_source = R"(
fn main() -> i32 {
  let x: i32 = 5;
  let y: i32 = x[0];
  return y;
}
)";

  const auto index_unsupported_diags =
      analyze_source(index_unsupported_source, "index_unsupported_precision");
  if (!expect(has_message(index_unsupported_diags,
                          "Expression 'x' of type 'i32' does not support indexing."),
              "missing precise unsupported-indexing diagnostic with expression context")) {
    return 1;
  }

  const std::string if_condition_source = R"(
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

  const auto if_condition_diags = analyze_source(if_condition_source, "if_condition_precision");
  if (!expect(has_message(if_condition_diags,
                          "Condition expression 'u.active' must be bool, found 'i32'."),
              "missing precise if-condition diagnostic with expression context")) {
    return 1;
  }

  const std::string while_condition_source = R"(
fn main() -> i32 {
  var x: i32 = 1;
  while (x) {
    break;
  }
  return 0;
}
)";

  const auto while_condition_diags =
      analyze_source(while_condition_source, "while_condition_precision");
  if (!expect(
          has_message(while_condition_diags, "Condition expression 'x' must be bool, found 'i32'."),
          "missing precise while-condition diagnostic with expression context")) {
    return 1;
  }

  const std::string match_guard_source = R"(
fn main() -> i32 {
  let m: Option[i32] = Some(1);
  return match (m) {
    Some(v) if v => v;
    None => 0;
  };
}
)";

  const auto match_guard_diags = analyze_source(match_guard_source, "match_guard_precision");
  if (!expect(has_message(match_guard_diags, "Condition expression 'v' must be bool, found 'i32'."),
              "missing precise match-guard condition diagnostic with expression context")) {
    return 1;
  }

  return 0;
}
