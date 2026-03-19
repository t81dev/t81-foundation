// tests/cpp/lattice_crypto_test.cpp
//
// RFC-0038 acceptance tests — Ternary Lattice Cryptography primitives.
//
// Verifies that std.crypto.polymul and std.crypto.polymod are registered,
// parse + type-check without error, and lower to the correct TISC opcodes.
// Also verifies the math layer directly via t81::ops::polymul / polymod.
//
// Acceptance criteria:
//   AC-1  std.crypto.polymul registered; lowers to POLYMUL
//   AC-2  std.crypto.polymod registered; lowers to POLYMOD
//   AC-3  Wrong arity is a SA error
//   AC-4  polymul math: degree-4 negacyclic identity (A * 1 = A)
//   AC-5  polymul math: x * x = x^2 in Z[x]/(x^4+1) wraps with sign flip
//   AC-6  polymod math: centered reduction maps coefficients to (−q/2, q/2]
//   AC-7  polymod math: q=3 reduces {-1,0,1,2,3,4} correctly
//   AC-8  Composition: polymul + polymod emits both opcodes

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
#include "t81/tensor/lattice_crypto.hpp"

using namespace t81::frontend;
using namespace t81::tisc::ir;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
}

// ─── IR helpers ──────────────────────────────────────────────────────────────

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

// ─── Math helpers ────────────────────────────────────────────────────────────

static t81::T729DynamicTensor poly(std::vector<float> coeffs) {
  int n = static_cast<int>(coeffs.size());
  return t81::T729DynamicTensor({n}, std::move(coeffs));
}

// ─── AC-1: std.crypto.polymul → POLYMUL ──────────────────────────────────────

static void test_polymul_registry_and_lowering() {
  const auto* def = t81::frontend::lookup_builtin_by_canonical("crypto_polymul");
  check(def != nullptr, "[AC-1] crypto_polymul in builtin registry");
  check(def != nullptr && def->return_kind == Type::Kind::Tensor,
        "[AC-1] crypto_polymul return type is Tensor");

  const std::string src = R"(
    fn run(a: Tensor, b: Tensor) -> Tensor {
      return std.crypto.polymul(a, b);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::POLYMUL),
        "[AC-1] std.crypto.polymul lowers to POLYMUL");
}

// ─── AC-2: std.crypto.polymod → POLYMOD ──────────────────────────────────────

static void test_polymod_registry_and_lowering() {
  const auto* def = t81::frontend::lookup_builtin_by_canonical("crypto_polymod");
  check(def != nullptr, "[AC-2] crypto_polymod in builtin registry");
  check(def != nullptr && def->return_kind == Type::Kind::Tensor,
        "[AC-2] crypto_polymod return type is Tensor");

  const std::string src = R"(
    fn run(a: Tensor, q: T81BigInt) -> Tensor {
      return std.crypto.polymod(a, q);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::POLYMOD),
        "[AC-2] std.crypto.polymod lowers to POLYMOD");
}

// ─── AC-3: wrong arity is a SA error ─────────────────────────────────────────

static void test_wrong_arity_is_error() {
  const std::string src = R"(
    fn bad(a: Tensor) -> Tensor {
      return std.crypto.polymul(a);
    }
  )";
  bool sa_err = false;
  lower(src, &sa_err);
  check(sa_err, "[AC-3] wrong arity on polymul is a SA error");
}

// ─── AC-4: polymul identity (A * 1 = A) in degree-4 ring ────────────────────

static void test_polymul_identity() {
  // Identity polynomial in Z[x]/(x^4+1): [1, 0, 0, 0]
  auto a = poly({1.0f, -1.0f, 0.0f, 1.0f});
  auto one = poly({1.0f, 0.0f, 0.0f, 0.0f});
  auto result = t81::ops::polymul(a, one);
  auto rv = result.snapshot_values();
  // A * 1 = A: result should equal A's coefficients
  auto av = a.snapshot_values();
  bool ok = (rv.size() == av.size());
  for (std::size_t i = 0; i < rv.size() && ok; ++i)
    ok = (rv[i] == av[i]);
  check(ok, "[AC-4] polymul identity: A * [1,0,0,0] = A in Z[x]/(x^4+1)");
}

// ─── AC-5: negacyclic wrap — x * x in Z[x]/(x^4+1) ──────────────────────────

static void test_polymul_negacyclic_wrap() {
  // x = [0, 1, 0, 0]
  // x * x = x^2 = [0, 0, 1, 0]  (no wrap)
  // x^2 * x = x^3 = [0, 0, 0, 1]
  // x^3 * x = x^4 ≡ -1 in Z[x]/(x^4+1) → [-1, 0, 0, 0]
  auto x = poly({0.0f, 1.0f, 0.0f, 0.0f});
  auto x3 = poly({0.0f, 0.0f, 0.0f, 1.0f});
  auto result = t81::ops::polymul(x3, x);
  auto rv = result.snapshot_values();
  // x^4 ≡ -1 → result = [-1, 0, 0, 0]
  check(rv.size() == 4 && rv[0] == -1.0f && rv[1] == 0.0f &&
        rv[2] == 0.0f && rv[3] == 0.0f,
        "[AC-5] x^3 * x = x^4 ≡ -1 (negacyclic wrap)");
}

// ─── AC-6: polymod centered reduction ────────────────────────────────────────

static void test_polymod_centered_reduction() {
  // q=5: centered range (-2, 2] i.e. {-2,-1,0,1,2}
  // Input coefficients: 0, 1, 2, 3, 4, 5, -1, -2, -3
  auto a = poly({0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, -1.0f, -2.0f, -3.0f});
  auto result = t81::ops::polymod(a, 5);
  auto rv = result.snapshot_values();
  // Expected:  0→0, 1→1, 2→2, 3→-2, 4→-1, 5→0, -1→-1, -2→-2, -3→2
  std::vector<float> expected = {0.0f, 1.0f, 2.0f, -2.0f, -1.0f, 0.0f, -1.0f, -2.0f, 2.0f};
  bool ok = (rv.size() == expected.size());
  for (std::size_t i = 0; i < rv.size() && ok; ++i)
    ok = (rv[i] == expected[i]);
  check(ok, "[AC-6] polymod q=5 centered reduction");
}

// ─── AC-7: polymod q=3 ───────────────────────────────────────────────────────

static void test_polymod_q3() {
  // q=3: centered range {-1, 0, 1}
  // Input: -1→-1, 0→0, 1→1, 2→-1, 3→0, 4→1
  auto a = poly({-1.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f});
  auto result = t81::ops::polymod(a, 3);
  auto rv = result.snapshot_values();
  std::vector<float> expected = {-1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f};
  bool ok = (rv.size() == expected.size());
  for (std::size_t i = 0; i < rv.size() && ok; ++i)
    ok = (rv[i] == expected[i]);
  check(ok, "[AC-7] polymod q=3 reduces {-1,0,1,2,3,4} → {-1,0,1,-1,0,1}");
}

// ─── AC-8: composition emits both opcodes ────────────────────────────────────

static void test_composition_emits_both_ops() {
  const std::string src = R"(
    fn keymul(a: Tensor, b: Tensor, q: T81BigInt) -> Tensor {
      let c: Tensor = std.crypto.polymul(a, b);
      return std.crypto.polymod(c, q);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::POLYMUL), "[AC-8] composition emits POLYMUL");
  check(has_opcode(instrs, Opcode::POLYMOD), "[AC-8] composition emits POLYMOD");
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== RFC-0038 Lattice Crypto acceptance tests ===\n");
  test_polymul_registry_and_lowering();
  test_polymod_registry_and_lowering();
  test_wrong_arity_is_error();
  test_polymul_identity();
  test_polymul_negacyclic_wrap();
  test_polymod_centered_reduction();
  test_polymod_q3();
  test_composition_emits_both_ops();
  std::printf("\nResults: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
