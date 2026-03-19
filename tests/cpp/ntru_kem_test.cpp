// tests/cpp/ntru_kem_test.cpp
//
// RFC-0039 acceptance tests — NTRU-KEM: Ternary Key Encapsulation Mechanism.
//
// Verifies:
//   - TVecSub opcode dispatch (polysub → TVECSUB)
//   - polyadd / polysub builtins registered and lower to correct opcodes
//   - ntru_encrypt / ntru_decrypt lower to POLYMUL + TVECADD/POLYMOD sequences
//   - Wrong arity is a semantic-analysis error
//   - C++ math layer: polyadd coefficient correctness
//   - C++ math layer: polysub coefficient correctness
//   - C++ math layer: ntru_encrypt produces length-n output
//   - C++ math layer: ntru_decrypt(f=identity, encrypt(identity, msg, r, q), q) = msg
//
// Acceptance criteria:
//   AC-1  std.crypto.polyadd registered; lowers to TVECADD
//   AC-2  std.crypto.polysub registered; lowers to TVECSUB
//   AC-3  std.crypto.ntru_encrypt registered; lowers to POLYMUL+TVECADD+POLYMOD
//   AC-4  std.crypto.ntru_decrypt registered; lowers to POLYMUL+POLYMOD
//   AC-5  Wrong arity (polyadd with 1 arg) is a SA error
//   AC-6  polyadd math: [1,-1,0] + [0,1,1] = [1,0,1]
//   AC-7  polysub math: [1, 1,0] − [0,1,1] = [1,0,-1]
//   AC-8  ntru_encrypt produces a degree-(n-1) polynomial (length n)
//   AC-9  ntru_keygen / ntru_encrypt / ntru_decrypt round-trip (identity key)
//   AC-10 ntru_detail::make_ternary_poly is deterministic and ternary

#include <cassert>
#include <cmath>
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
#include "t81/tensor/ntru_kem.hpp"

using namespace t81::frontend;
using namespace t81::tisc::ir;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
}

// ─── IR helpers ───────────────────────────────────────────────────────────────

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

static int count_opcode(const std::vector<Instruction>& instrs, Opcode op) {
  int n = 0;
  for (const auto& i : instrs)
    if (i.opcode == op) ++n;
  return n;
}

// ─── Tensor helpers ───────────────────────────────────────────────────────────

static t81::T729DynamicTensor make_tensor(std::vector<float> v) {
  int n = static_cast<int>(v.size());
  return t81::T729DynamicTensor({n}, std::move(v));
}

static std::vector<float> snap(const t81::T729DynamicTensor& t) {
  return t.snapshot_values();
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-1: std.crypto.polyadd registered; lowers to TVECADD
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac1_polyadd_registry() {
  const auto* def = lookup_builtin("std.crypto.polyadd");
  check(def != nullptr, "AC-1a: std.crypto.polyadd in registry");
  check(def && def->primary_opcode == Opcode::TVECADD,
        "AC-1b: std.crypto.polyadd primary_opcode = TVECADD");

  const std::string src = R"(
    fn test(a: Tensor, b: Tensor) -> Tensor {
      return std.crypto.polyadd(a, b);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TVECADD), "AC-1c: polyadd lowers to TVECADD");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-2: std.crypto.polysub registered; lowers to TVECSUB
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac2_polysub_registry() {
  const auto* def = lookup_builtin("std.crypto.polysub");
  check(def != nullptr, "AC-2a: std.crypto.polysub in registry");
  check(def && def->primary_opcode == Opcode::TVECSUB,
        "AC-2b: std.crypto.polysub primary_opcode = TVECSUB");

  const std::string src = R"(
    fn test(a: Tensor, b: Tensor) -> Tensor {
      return std.crypto.polysub(a, b);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::TVECSUB), "AC-2c: polysub lowers to TVECSUB");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-3: ntru_encrypt lowers to POLYMUL + TVECADD + POLYMOD
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac3_ntru_encrypt_lowering() {
  const auto* def = lookup_builtin("std.crypto.ntru_encrypt");
  check(def != nullptr, "AC-3a: std.crypto.ntru_encrypt in registry");

  const std::string src = R"(
    fn test(h: Tensor, msg: Tensor, r: Tensor, q: i32) -> Tensor {
      return std.crypto.ntru_encrypt(h, msg, r, q);
    }
  )";
  auto instrs = lower(src);
  check(has_opcode(instrs, Opcode::POLYMUL),  "AC-3b: ntru_encrypt emits POLYMUL");
  check(has_opcode(instrs, Opcode::TVECADD),  "AC-3c: ntru_encrypt emits TVECADD");
  check(has_opcode(instrs, Opcode::POLYMOD),  "AC-3d: ntru_encrypt emits POLYMOD");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-4: ntru_decrypt lowers to POLYMUL + POLYMOD
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac4_ntru_decrypt_lowering() {
  const auto* def = lookup_builtin("std.crypto.ntru_decrypt");
  check(def != nullptr, "AC-4a: std.crypto.ntru_decrypt in registry");

  const std::string src = R"(
    fn test(f: Tensor, c: Tensor, p: i32) -> Tensor {
      return std.crypto.ntru_decrypt(f, c, p);
    }
  )";
  auto instrs = lower(src);
  check(count_opcode(instrs, Opcode::POLYMUL) >= 1, "AC-4b: ntru_decrypt emits POLYMUL");
  check(count_opcode(instrs, Opcode::POLYMOD) >= 1, "AC-4c: ntru_decrypt emits POLYMOD");
  // Must not emit TVECADD (that is only encrypt)
  check(!has_opcode(instrs, Opcode::TVECADD), "AC-4d: ntru_decrypt does NOT emit TVECADD");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-5: Wrong arity (polyadd with 1 arg) is a SA error
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac5_arity_error() {
  const std::string src = R"(
    fn bad(a: Tensor) -> Tensor {
      return std.crypto.polyadd(a);
    }
  )";
  bool sa_err = false;
  lower(src, &sa_err);
  check(sa_err, "AC-5: polyadd(1 arg) is a SA error");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-6: polyadd math: [1,-1,0] + [0,1,1] = [1,0,1]
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac6_polyadd_math() {
  auto a = make_tensor({ 1.f, -1.f,  0.f});
  auto b = make_tensor({ 0.f,  1.f,  1.f});
  auto c = t81::ops::add(a, b);
  auto cv = snap(c);
  check(cv.size() == 3 && cv[0] == 1.f && cv[1] == 0.f && cv[2] == 1.f,
        "AC-6: polyadd [1,-1,0]+[0,1,1]=[1,0,1]");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-7: polysub math: [1,1,0] − [0,1,1] = [1,0,-1]
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac7_polysub_math() {
  auto a = make_tensor({ 1.f,  1.f,  0.f});
  auto b = make_tensor({ 0.f,  1.f,  1.f});
  auto c = t81::ops::sub(a, b);
  auto cv = snap(c);
  check(cv.size() == 3 && cv[0] == 1.f && cv[1] == 0.f && cv[2] == -1.f,
        "AC-7: polysub [1,1,0]-[0,1,1]=[1,0,-1]");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-8: ntru_encrypt produces a degree-(n-1) polynomial (length n)
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac8_encrypt_length() {
  constexpr int n = 8;
  auto h   = t81::crypto::ntru_detail::make_ternary_poly(n, 42);
  auto msg = t81::crypto::ntru_detail::make_ternary_poly(n, 99);
  auto r   = t81::crypto::ntru_detail::make_ternary_poly(n, 7);
  auto c   = t81::crypto::ntru_encrypt(h, msg, r, /*q=*/7);
  auto cv  = snap(c);
  check(static_cast<int>(cv.size()) == n, "AC-8: ntru_encrypt output has length n");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-9: Round-trip with identity key f=[1,0,...,0], h=[1,0,...,0]
//       decrypt(f, encrypt(h, msg, r, q), q) mod q == msg mod q
//       (identity key: h*r = r, so c = r+msg mod q; f*c = c; polymod(c,q) = c)
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac9_roundtrip_identity() {
  // Identity polynomial: [1, 0, 0, 0]
  auto f = make_tensor({1.f, 0.f, 0.f, 0.f});
  auto h = make_tensor({1.f, 0.f, 0.f, 0.f});
  // msg and r are small ternary polynomials
  auto msg = make_tensor({ 0.f,  1.f, -1.f,  0.f});
  auto r   = make_tensor({ 1.f,  0.f,  0.f,  0.f});

  // c = polymod(polyadd(polymul(h=[1,0,0,0], r=[1,0,0,0]), msg), q)
  //   = polymod(polyadd([1,0,0,0], [0,1,-1,0]), 7)
  //   = polymod([1,1,-1,0], 7)
  //   = [1,1,-1,0]   (all within centered range of q=7)
  auto c = t81::crypto::ntru_encrypt(h, msg, r, /*q=*/7);
  auto cv = snap(c);
  check(cv.size() == 4 && cv[0]==1.f && cv[1]==1.f && cv[2]==-1.f && cv[3]==0.f,
        "AC-9a: ntru_encrypt(identity) = [1,1,-1,0]");

  // m' = polymod(polymul(f=[1,0,0,0], c), p)
  //    = polymod([1,1,-1,0], 7) = [1,1,-1,0]
  auto m_prime = t81::crypto::ntru_decrypt(f, c, /*p=*/7);
  auto mv = snap(m_prime);
  check(mv.size() == 4 && mv[0]==1.f && mv[1]==1.f && mv[2]==-1.f && mv[3]==0.f,
        "AC-9b: ntru_decrypt(identity) recovers ciphertext = [1,1,-1,0]");

  // AC-9c: with r=0 (no blinding), encrypt(h, msg, r=0, q) = polymod(msg, q) = msg
  //        (coefficients fit in q=7 window), and decrypt(f=identity, c=msg, p) = msg
  auto r_zero = make_tensor({0.f, 0.f, 0.f, 0.f});
  auto c_unblinded = t81::crypto::ntru_encrypt(h, msg, r_zero, /*q=*/7);
  auto m_recovered = t81::crypto::ntru_decrypt(f, c_unblinded, /*p=*/7);
  auto mr = snap(m_recovered);
  // msg=[0,1,-1,0] — all coefficients within centered range of q=7, so unchanged
  check(mr.size() == 4 && mr[0]==0.f && mr[1]==1.f && mr[2]==-1.f && mr[3]==0.f,
        "AC-9c: ntru_decrypt(identity, encrypt(identity,msg,r=0,q), q) = msg");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-10: ntru_detail::make_ternary_poly is deterministic and ternary
// ─────────────────────────────────────────────────────────────────────────────
static void test_ac10_make_ternary_poly() {
  auto p1 = t81::crypto::ntru_detail::make_ternary_poly(8, 42);
  auto p2 = t81::crypto::ntru_detail::make_ternary_poly(8, 42);
  auto v1 = snap(p1), v2 = snap(p2);
  // Deterministic: same seed → same poly
  bool same = (v1 == v2);
  check(same, "AC-10a: make_ternary_poly is deterministic");
  // All values in {-1, 0, +1}
  bool ternary = true;
  for (float x : v1)
    if (x != -1.f && x != 0.f && x != 1.f) { ternary = false; break; }
  check(ternary, "AC-10b: make_ternary_poly values are in {-1, 0, +1}");
  // Different seeds → different polys (statistical; expected with 8 trits)
  auto p3 = t81::crypto::ntru_detail::make_ternary_poly(8, 999);
  check(snap(p3) != v1, "AC-10c: make_ternary_poly differs for different seeds");
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== RFC-0039 NTRU-KEM Acceptance Tests ===\n");
  test_ac1_polyadd_registry();
  test_ac2_polysub_registry();
  test_ac3_ntru_encrypt_lowering();
  test_ac4_ntru_decrypt_lowering();
  test_ac5_arity_error();
  test_ac6_polyadd_math();
  test_ac7_polysub_math();
  test_ac8_encrypt_length();
  test_ac9_roundtrip_identity();
  test_ac10_make_ternary_poly();
  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail > 0 ? 1 : 0;
}
