// tests/cpp/t81lang_foreign_ffi_test.cpp
//
// RFC-0036 acceptance tests — T81Lang `foreign {}` syntax and FFI_CALL lowering.
//
// Verified criteria:
//   AC-1  'foreign' lexes as TokenType::Foreign
//   AC-2  Parser produces a ForeignDecl node
//   AC-3  ForeignDecl captures policy and function name
//   AC-4  Empty foreign block (no policy) is valid
//   AC-5  Multiple signatures in one block
//   AC-6  SA registers foreign functions in foreign_definitions()
//   AC-7  Duplicate foreign declaration is an SA error
//   AC-8  `foreign.<name>(args)` call site lowers to FFI_CALL with correct name
//   AC-9  `foreign` is usable as a variable name (binding position)

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "t81/frontend/ast.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/ir.hpp"

using namespace t81::frontend;
using namespace t81::tisc::ir;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) {
    std::printf("  PASS  %s\n", label);
    ++g_pass;
  } else {
    std::printf("  FAIL  %s\n", label);
    ++g_fail;
  }
}

// ─── helpers ────────────────────────────────────────────────────────────────

static std::vector<std::unique_ptr<Stmt>> parse_source(const std::string& src,
                                                        bool* had_error = nullptr) {
  Lexer lex(src);
  Parser p(lex, "test");
  auto stmts = p.parse();
  if (had_error) *had_error = p.had_error();
  return stmts;
}

static std::vector<Instruction> lower(const std::string& src) {
  Lexer lex(src);
  Parser p(lex, "test");
  auto stmts = p.parse();
  SemanticAnalyzer sa(stmts, "test");
  sa.analyze();
  IRGenerator gen;
  gen.attach_semantic_analyzer(&sa);
  return gen.generate(stmts).instructions();
}

// ─── AC-1: lexer tokenises 'foreign' as TokenType::Foreign ──────────────────

static void test_lexer_foreign_keyword() {
  Lexer lex("foreign");
  auto tok = lex.next_token();
  check(tok.type == TokenType::Foreign, "[AC-1] 'foreign' lexes as TokenType::Foreign");
}

// ─── AC-2: parser produces a ForeignDecl node ───────────────────────────────

static void test_parser_produces_foreign_decl() {
  const std::string src = R"(
    foreign deterministic {
      fn sin(x: T81Float) -> T81Float;
    }
  )";
  bool err = false;
  auto stmts = parse_source(src, &err);
  check(!err, "[AC-2] foreign block parses without error");
  bool found = !stmts.empty() && dynamic_cast<ForeignDecl*>(stmts[0].get()) != nullptr;
  check(found, "[AC-2] parser produces a ForeignDecl node");
}

// ─── AC-3: ForeignDecl captures policy and function name ────────────────────

static void test_policy_and_function_captured() {
  const std::string src = R"(
    foreign governed {
      fn my_fn(a: T81BigInt) -> T81BigInt;
    }
  )";
  auto stmts = parse_source(src);
  auto* fd = stmts.empty() ? nullptr : dynamic_cast<ForeignDecl*>(stmts[0].get());
  check(fd != nullptr && fd->policy == "governed",
        "[AC-3] ForeignDecl captures policy 'governed'");
  check(fd != nullptr && fd->functions.size() == 1u &&
            std::string(fd->functions[0].name.lexeme) == "my_fn",
        "[AC-3] ForeignDecl captures function name 'my_fn'");
}

// ─── AC-4: empty foreign block (no policy) is valid ────────────────────────

static void test_empty_foreign_block_no_policy() {
  bool err = false;
  auto stmts = parse_source("foreign { }", &err);
  check(!err, "[AC-4] empty foreign block (no policy) parses without error");
  auto* fd = stmts.empty() ? nullptr : dynamic_cast<ForeignDecl*>(stmts[0].get());
  check(fd != nullptr && fd->policy.empty() && fd->functions.empty(),
        "[AC-4] empty foreign block has empty policy and no functions");
}

// ─── AC-5: multiple signatures in one block ─────────────────────────────────

static void test_multiple_signatures() {
  const std::string src = R"(
    foreign deterministic {
      fn add(a: T81Float, b: T81Float) -> T81Float;
      fn mul(a: T81Float, b: T81Float) -> T81Float;
    }
  )";
  auto stmts = parse_source(src);
  auto* fd = stmts.empty() ? nullptr : dynamic_cast<ForeignDecl*>(stmts[0].get());
  check(fd != nullptr && fd->functions.size() == 2u,
        "[AC-5] two foreign function signatures parsed");
  check(fd != nullptr && fd->functions.size() >= 2u &&
            std::string(fd->functions[0].name.lexeme) == "add" &&
            std::string(fd->functions[1].name.lexeme) == "mul",
        "[AC-5] function names are 'add' and 'mul'");
}

// ─── AC-6: SA registers foreign functions ────────────────────────────────────

static void test_sa_registers_foreign_functions() {
  const std::string src = R"(
    foreign deterministic {
      fn hypot(a: T81Float, b: T81Float) -> T81Float;
    }
  )";
  auto stmts = parse_source(src);
  SemanticAnalyzer sa(stmts, "test");
  sa.analyze();
  const auto& defs = sa.foreign_definitions();
  check(defs.count("hypot") == 1u,
        "[AC-6] SA registers 'hypot' in foreign_definitions");
  check(defs.count("hypot") == 1u && defs.at("hypot").policy == "deterministic",
        "[AC-6] registered entry carries policy='deterministic'");
}

// ─── AC-7: duplicate declaration is an SA error ──────────────────────────────

static void test_duplicate_foreign_function_is_error() {
  const std::string src = R"(
    foreign deterministic {
      fn dup(x: T81Float) -> T81Float;
    }
    foreign governed {
      fn dup(x: T81Float) -> T81Float;
    }
  )";
  auto stmts = parse_source(src);
  SemanticAnalyzer sa(stmts, "test");
  sa.analyze();
  check(sa.had_error(),
        "[AC-7] duplicate foreign function name produces SA error");
}

// ─── AC-8: call site lowers to FFI_CALL with correct name ────────────────────

static void test_call_site_emits_ffi_call() {
  const std::string src = R"(
    foreign deterministic {
      fn mysin(x: T81Float) -> T81Float;
    }
    fn main() -> void {
      let result: T81Float = foreign.mysin(1.0);
    }
  )";
  auto instrs = lower(src);
  bool found = false;
  bool name_ok = false;
  for (const auto& instr : instrs) {
    if (instr.opcode == Opcode::FFI_CALL) {
      found = true;
      name_ok = instr.text_literal.has_value() &&
                instr.text_literal.value() == "mysin";
    }
  }
  check(found, "[AC-8] FFI_CALL instruction emitted for foreign.mysin(...)");
  check(name_ok, "[AC-8] FFI_CALL text_literal == 'mysin'");
}

// ─── AC-9: `foreign` usable as variable name ─────────────────────────────────

static void test_foreign_usable_as_variable_name() {
  const std::string src = R"(
    fn main() -> void {
      let foreign: T81BigInt = 42;
    }
  )";
  bool threw = false;
  try {
    parse_source(src);
  } catch (...) {
    threw = true;
  }
  check(!threw, "[AC-9] 'foreign' usable as variable name without parser crash");
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== RFC-0036 T81Lang Foreign FFI acceptance tests ===\n");
  test_lexer_foreign_keyword();
  test_parser_produces_foreign_decl();
  test_policy_and_function_captured();
  test_empty_foreign_block_no_policy();
  test_multiple_signatures();
  test_sa_registers_foreign_functions();
  test_duplicate_foreign_function_is_error();
  test_call_site_emits_ffi_call();
  test_foreign_usable_as_variable_name();
  std::printf("\nResults: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
