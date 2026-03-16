// tests/cpp/tnn_stdlib_test.cpp
//
// RFC-0037 acceptance tests — T81Lang TNN stdlib builtins.
//
// Verifies that the six std.tnn.* functions are registered in the builtin
// table, parse + type-check without error, and lower to the correct
// RFC-0034 TISC opcodes (TWMATMUL, TQUANT, TATTN, TWEMBED, TERNACCUM, TACT).
//
// Acceptance criteria:
//   AC-1  std.tnn.matmul  registered; lowers to TWMATMUL
//   AC-2  std.tnn.quant   registered; lowers to TQUANT
//   AC-3  std.tnn.attn    registered; lowers to TATTN
//   AC-4  std.tnn.embed   registered; lowers to TWEMBED
//   AC-5  std.tnn.accum   registered; lowers to TERNACCUM
//   AC-6  std.tnn.act     registered; lowers to TACT
//   AC-7  Wrong arity is a SA error
//   AC-8  Forward-pass composition (quant → matmul → act) emits all three ops

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "t81/frontend/ast.hpp"
#include "t81/frontend/builtin_registry.hpp"
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
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
}

// ─── helpers ─────────────────────────────────────────────────────────────────

static std::vector<std::unique_ptr<Stmt>> parse_source(const std::string& src,
                                                        bool* had_error = nullptr) {
  Lexer lex(src);
  Parser p(lex, "test");
  auto stmts = p.parse();
  if (had_error) *had_error = p.had_error();
  return stmts;
}

static std::vector<Instruction> lower(const std::string& src, bool* sa_error = nullptr) {
  Lexer lex(src);
  Parser p(lex, "test");
  auto stmts = p.parse();
  SemanticAnalyzer sa(stmts, "test");
  sa.analyze();
  if (sa_error) *sa_error = sa.had_error();
  if (sa.had_error()) return {};
  IRGenerator gen;
  gen.attach_semantic_analyzer(&sa);
  return gen.generate(stmts).instructions();
}

static bool has_opcode(const std::vector<Instruction>& instrs, Opcode op) {
  for (const auto& i : instrs)
    if (i.opcode == op) return true;
  return false;
}

// ─── AC-1: std.tnn.matmul → TWMATMUL ────────────────────────────────────────

static void test_tnn_matmul() {
  // Verify registry entry
  const auto* def = t81::frontend::lookup_builtin_by_canonical("tnn_matmul");
  check(def != nullptr, "[AC-1] tnn_matmul in builtin registry");
  check(def != nullptr && def->return_kind == Type::Kind::Tensor,
        "[AC-1] tnn_matmul return type is Tensor");

  // Verify IR lowering
  const std::string src = R"(
    fn run(act: Tensor, wt: Tensor) -> Tensor {
      return std.tnn.matmul(act, wt);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TWMATMUL),
        "[AC-1] std.tnn.matmul lowers to TWMATMUL");
}

// ─── AC-2: std.tnn.quant → TQUANT ───────────────────────────────────────────

static void test_tnn_quant() {
  const auto* def = t81::frontend::lookup_builtin_by_canonical("tnn_quant");
  check(def != nullptr, "[AC-2] tnn_quant in builtin registry");

  const std::string src = R"(
    fn quantize(x: T81Float, thr: T81Float) -> Tensor {
      return std.tnn.quant(x, thr);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TQUANT),
        "[AC-2] std.tnn.quant lowers to TQUANT");
}

// ─── AC-3: std.tnn.attn → TATTN ─────────────────────────────────────────────

static void test_tnn_attn() {
  const auto* def = t81::frontend::lookup_builtin_by_canonical("tnn_attn");
  check(def != nullptr, "[AC-3] tnn_attn in builtin registry");

  const std::string src = R"(
    fn attend(q: Tensor, k: Tensor, v: Tensor) -> Tensor {
      return std.tnn.attn(q, k, v);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TATTN),
        "[AC-3] std.tnn.attn lowers to TATTN");
}

// ─── AC-4: std.tnn.embed → TWEMBED ──────────────────────────────────────────

static void test_tnn_embed() {
  const auto* def = t81::frontend::lookup_builtin_by_canonical("tnn_embed");
  check(def != nullptr, "[AC-4] tnn_embed in builtin registry");

  const std::string src = R"(
    fn embed(table: Tensor, idx: T81BigInt) -> Tensor {
      return std.tnn.embed(table, idx);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TWEMBED),
        "[AC-4] std.tnn.embed lowers to TWEMBED");
}

// ─── AC-5: std.tnn.accum → TERNACCUM ────────────────────────────────────────

static void test_tnn_accum() {
  const auto* def = t81::frontend::lookup_builtin_by_canonical("tnn_accum");
  check(def != nullptr, "[AC-5] tnn_accum in builtin registry");
  check(def != nullptr && def->return_kind == Type::Kind::Float,
        "[AC-5] tnn_accum return type is Float");

  const std::string src = R"(
    fn dot(wt: Tensor, act: Tensor) -> T81Float {
      return std.tnn.accum(wt, act);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TERNACCUM),
        "[AC-5] std.tnn.accum lowers to TERNACCUM");
}

// ─── AC-6: std.tnn.act → TACT ───────────────────────────────────────────────

static void test_tnn_act() {
  const auto* def = t81::frontend::lookup_builtin_by_canonical("tnn_act");
  check(def != nullptr, "[AC-6] tnn_act in builtin registry");
  check(def != nullptr && def->return_kind == Type::Kind::BigInt,
        "[AC-6] tnn_act return type is BigInt");

  const std::string src = R"(
    fn activate(x: T81Float, mode: T81BigInt) -> T81BigInt {
      return std.tnn.act(x, mode);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TACT),
        "[AC-6] std.tnn.act lowers to TACT");
}

// ─── AC-7: wrong arity is a SA error ────────────────────────────────────────

static void test_wrong_arity_is_error() {
  // tnn.matmul expects 2 args; passing 1 should error
  const std::string src = R"(
    fn bad(act: Tensor) -> Tensor {
      return std.tnn.matmul(act);
    }
  )";
  bool sa_err = false;
  lower(src, &sa_err);
  check(sa_err, "[AC-7] wrong arity on tnn.matmul is a SA error");
}

// ─── AC-8: forward-pass composition emits all three ops ─────────────────────

static void test_forward_pass_composition() {
  // Minimal TNN layer: quant → matmul → act
  const std::string src = R"(
    fn forward(input: T81Float, thr: T81Float, wt: Tensor, mode: T81BigInt) -> T81BigInt {
      let q: Tensor = std.tnn.quant(input, thr);
      let z: Tensor = std.tnn.matmul(q, wt);
      let a: T81Float = std.tnn.accum(z, q);
      return std.tnn.act(a, mode);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TQUANT),   "[AC-8] forward pass emits TQUANT");
  check(has_opcode(instrs, Opcode::TWMATMUL), "[AC-8] forward pass emits TWMATMUL");
  check(has_opcode(instrs, Opcode::TERNACCUM),"[AC-8] forward pass emits TERNACCUM");
  check(has_opcode(instrs, Opcode::TACT),     "[AC-8] forward pass emits TACT");
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== RFC-0037 TNN stdlib acceptance tests ===\n");
  test_tnn_matmul();
  test_tnn_quant();
  test_tnn_attn();
  test_tnn_embed();
  test_tnn_accum();
  test_tnn_act();
  test_wrong_arity_is_error();
  test_forward_pass_composition();
  std::printf("\nResults: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
