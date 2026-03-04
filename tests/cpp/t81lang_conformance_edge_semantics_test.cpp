#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

using namespace t81::frontend;

namespace {

bool analyzes(std::string_view source, const char* label) {
  std::string source_text(source);
  Lexer lexer{source_text};
  Parser parser(lexer, label);
  auto stmts = parser.parse();
  if (parser.had_error()) return false;
  SemanticAnalyzer analyzer(stmts, label);
  analyzer.analyze();
  return !analyzer.had_error();
}

bool fails_parse_or_semantic(std::string_view source, const char* label) {
  std::string source_text(source);
  Lexer lexer{source_text};
  Parser parser(lexer, label);
  auto stmts = parser.parse();
  if (parser.had_error()) return true;
  SemanticAnalyzer analyzer(stmts, label);
  analyzer.analyze();
  return analyzer.had_error();
}

void require_true(bool condition, const char* label) {
  if (!condition) {
    std::cerr << "Conformance edge assertion failed: " << label << "\n";
    std::abort();
  }
}

void test_match_guard_non_bool_fails() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let maybe: Option[i32] = Some(5);
      let out: i32 = match (maybe) {
        Some(v) if Some(v) => v,
        None => 0
      };
      return out;
    }
  )";
  require_true(fails_parse_or_semantic(source, "t81lang_edge_match_guard_non_bool"),
               "t81lang_edge_match_guard_non_bool");
}

void test_result_match_guard_and_payload_success() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let r: Result[i32, T81String] = Ok(9);
      let out: i32 = match (r) {
        Ok(v) if v > 5 => v,
        Ok(_) => 0,
        Err(_) => -1
      };
      return out;
    }
  )";
  require_true(analyzes(source, "t81lang_edge_result_match_guard_success"),
               "t81lang_edge_result_match_guard_success");
}

void test_match_guard_requires_unguarded_fallback() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      let maybe: Option[i32] = Some(5);
      let out: i32 = match (maybe) {
        Some(v) if v > 0 => v,
        None => 0
      };
      return out;
    }
  )";
  require_true(fails_parse_or_semantic(source, "t81lang_edge_match_guard_requires_fallback"),
               "t81lang_edge_match_guard_requires_fallback");
}

void test_loop_missing_bounded_annotation_fails() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      loop {
        return 0;
      }
    }
  )";
  require_true(fails_parse_or_semantic(source, "t81lang_edge_loop_missing_bounded"),
               "t81lang_edge_loop_missing_bounded");
}

void test_guarded_loop_non_bool_guard_fails() {
  constexpr const char* source = R"(
    fn main() -> i32 {
      var value: i32 = 0;
      @bounded(loop(value))
      loop {
        return value;
      }
    }
  )";
  require_true(fails_parse_or_semantic(source, "t81lang_edge_guarded_loop_non_bool"),
               "t81lang_edge_guarded_loop_non_bool");
}

void test_nested_loop_match_with_annotations_success() {
  constexpr const char* source = R"(
    @tier(2)
    fn main() -> i32 {
      var counter: i32 = 0;
      @bounded(infinite)
      loop {
        @bounded(loop(counter < 3))
        loop {
          counter = counter + 1;
          let maybe: Option[i32] = Some(counter);
          return match (maybe) {
            Some(v) => v,
            None => 0
          };
        }
      }
      return 0;
    }
  )";
  require_true(analyzes(source, "t81lang_edge_nested_loop_match_success"),
               "t81lang_edge_nested_loop_match_success");
}

void test_tier1_reflect_behavior_fails() {
  constexpr const char* source = R"(
    @tier(1)
    fn main() -> i32 {
      reflect {
        return;
      }
      return 0;
    }
  )";
  require_true(fails_parse_or_semantic(source, "t81lang_edge_tier1_reflect_forbidden"),
               "t81lang_edge_tier1_reflect_forbidden");
}

void test_tier3_distributed_behavior_fails() {
  constexpr const char* source = R"(
    @tier(3)
    fn main() -> i32 {
      distributed {
        return 0;
      }
      return 0;
    }
  )";
  require_true(fails_parse_or_semantic(source, "t81lang_edge_tier3_distributed_forbidden"),
               "t81lang_edge_tier3_distributed_forbidden");
}

void test_tier1_effect_surface_call_fails() {
  constexpr const char* source = R"(
    @tier(1)
    fn main() -> i32 {
      std.io.println("x");
      return 0;
    }
  )";
  require_true(fails_parse_or_semantic(source, "t81lang_edge_tier1_effect_surface_forbidden"),
               "t81lang_edge_tier1_effect_surface_forbidden");
}

void test_tier1_calling_tier3_function_succeeds() {
  constexpr const char* source = R"(
    @tier(3)
    fn deep() -> i32 { return 7; }

    @tier(1)
    fn main() -> i32 {
      return deep();
    }
  )";
  require_true(analyzes(source, "t81lang_edge_tier1_calls_tier3_succeeds"),
               "t81lang_edge_tier1_calls_tier3_succeeds");
}

}  // namespace

int main() {
  test_match_guard_non_bool_fails();
  test_result_match_guard_and_payload_success();
  test_match_guard_requires_unguarded_fallback();
  test_loop_missing_bounded_annotation_fails();
  test_guarded_loop_non_bool_guard_fails();
  test_nested_loop_match_with_annotations_success();
  test_tier1_reflect_behavior_fails();
  test_tier3_distributed_behavior_fails();
  test_tier1_effect_surface_call_fails();
  test_tier1_calling_tier3_function_succeeds();
  std::cout << "t81lang conformance edge semantics test passed!\n";
  return 0;
}
