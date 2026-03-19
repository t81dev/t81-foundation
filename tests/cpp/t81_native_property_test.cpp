// tests/cpp/t81_native_property_test.cpp
//
// RFC-0017 §Proposal + RFC-0018 §Testing — T81 native type property tests
//
// Verified properties:
//   [RFC-0017-P1]  Encoding sanity: 00 = −1, 01 = 0, 10 = +1, 11 = reserved
//   [RFC-0017-P2]  Negation involution: −(−x) == x  (2 048 random operands)
//   [RFC-0017-P3]  Additive identity: x + 0 == x  (2 048 random operands)
//   [RFC-0017-P4]  Additive inverse: x + (−x) == 0  (2 048 random operands)
//   [RFC-0017-P5]  Addition value-correctness vs scalar reference (2 048 trials)
//   [RFC-0017-P6]  Subtraction: x − y == x + (−y)  (2 048 trials)
//   [RFC-0018-P1]  Multiplication value-correctness vs scalar reference (2 048 trials)

#include "t81/native.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <random>

using t81::T81;

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

// ── Helpers ───────────────────────────────────────────────────────────────────

static T81 make_zero() {
  // Encoding: each trit is 2 bits; 0 = 0b01. Four trits per byte → 0b01010101 = 0x55.
  std::array<uint8_t, 32> buf;
  buf.fill(0x55u);
  return T81{buf};
}

static bool is_zero(const T81& v) {
  for (auto b : v.data) if (b != 0x55u) return false;
  return true;
}

static T81 make_from_digits(const std::array<int8_t, 128>& digits) {
  std::array<uint8_t, 32> buf{};
  T81::PackDigits(digits, buf);
  return T81{buf};
}

// Evaluate first `limit` trits as a signed integer.
static int64_t eval(const T81& v, int limit = 10) {
  std::array<int8_t, 128> d{};
  T81::UnpackDigits(v.data, d);
  int64_t value = 0;
  int64_t weight = 1;
  for (int i = 0; i < limit; ++i) {
    value += static_cast<int64_t>(d[i]) * weight;
    weight *= 3;
  }
  return value;
}

static std::array<int8_t, 128> random_digits(int count,
                                              std::mt19937_64& rng,
                                              std::uniform_int_distribution<int>& dist) {
  std::array<int8_t, 128> d{};
  for (int i = 0; i < count; ++i) d[i] = static_cast<int8_t>(dist(rng));
  return d;
}

// ── [RFC-0017-P1] Encoding sanity ─────────────────────────────────────────────

static void test_encoding_sanity() {
  std::printf("\n[RFC-0017-P1] Encoding sanity: 00=−1, 01=0, 10=+1\n");

  check(T81::EncodeTrit(-1) == 0u, "EncodeTrit(-1) == 0b00");
  check(T81::EncodeTrit( 0) == 1u, "EncodeTrit( 0) == 0b01");
  check(T81::EncodeTrit(+1) == 2u, "EncodeTrit(+1) == 0b10");

  check(T81::DecodeTrit(0u) == -1, "DecodeTrit(0b00) == -1");
  check(T81::DecodeTrit(1u) ==  0, "DecodeTrit(0b01) ==  0");
  check(T81::DecodeTrit(2u) == +1, "DecodeTrit(0b10) == +1");

  // Round-trip for each trit value
  for (int8_t t : {int8_t(-1), int8_t(0), int8_t(+1)}) {
    check(T81::DecodeTrit(T81::EncodeTrit(t)) == t,
          "DecodeTrit(EncodeTrit(t)) == t");
  }
}

// ── [RFC-0017-P2] Negation involution ─────────────────────────────────────────

static void test_negation_involution() {
  std::printf("\n[RFC-0017-P2] Negation involution: −(−x) == x  (2 048 trials)\n");

  std::mt19937_64 rng(0x1701'0017);
  std::uniform_int_distribution<int> dist(-1, 1);

  int failures = 0;
  for (int trial = 0; trial < 2048; ++trial) {
    const auto d   = random_digits(10, rng, dist);
    const T81  x   = make_from_digits(d);
    const T81  neg = -x;
    const T81  inv = -neg;
    if (inv.data != x.data) ++failures;
  }
  check(failures == 0, "[RFC-0017-P2] −(−x) == x for 2048 random operands");
}

// ── [RFC-0017-P3] Additive identity ───────────────────────────────────────────

static void test_additive_identity() {
  std::printf("\n[RFC-0017-P3] Additive identity: x + 0 == x  (2 048 trials)\n");

  std::mt19937_64 rng(0x1701'0018);
  std::uniform_int_distribution<int> dist(-1, 1);
  const T81 zero = make_zero();

  int failures = 0;
  for (int trial = 0; trial < 2048; ++trial) {
    const auto d = random_digits(10, rng, dist);
    const T81  x = make_from_digits(d);
    const T81  r = x + zero;
    if (r.data != x.data) ++failures;
  }
  check(failures == 0, "[RFC-0017-P3] x + 0 == x for 2048 random operands");
}

// ── [RFC-0017-P4] Additive inverse ────────────────────────────────────────────

static void test_additive_inverse() {
  std::printf("\n[RFC-0017-P4] Additive inverse: x + (−x) == 0  (2 048 trials)\n");

  std::mt19937_64 rng(0x1701'0019);
  std::uniform_int_distribution<int> dist(-1, 1);

  int failures = 0;
  for (int trial = 0; trial < 2048; ++trial) {
    const auto d   = random_digits(10, rng, dist);
    const T81  x   = make_from_digits(d);
    const T81  sum = x + (-x);
    if (!is_zero(sum)) ++failures;
  }
  check(failures == 0, "[RFC-0017-P4] x + (−x) == 0 for 2048 random operands");
}

// ── [RFC-0017-P5] Addition value-correctness ──────────────────────────────────

static void test_addition_correctness() {
  std::printf("\n[RFC-0017-P5] Addition value-correctness vs scalar reference  (2 048 trials)\n");

  std::mt19937_64 rng(0x1701'001A);
  std::uniform_int_distribution<int> dist(-1, 1);

  int failures = 0;
  for (int trial = 0; trial < 2048; ++trial) {
    const auto ld = random_digits(10, rng, dist);
    const auto rd = random_digits(10, rng, dist);
    const T81  lhs = make_from_digits(ld);
    const T81  rhs = make_from_digits(rd);
    const T81  sum = lhs + rhs;

    const int64_t lv = eval(lhs, 14);
    const int64_t rv = eval(rhs, 14);
    const int64_t sv = eval(sum, 14);
    if (sv != lv + rv) ++failures;
  }
  check(failures == 0, "[RFC-0017-P5] (lhs + rhs) matches scalar value for 2048 trials");
}

// ── [RFC-0017-P6] Subtraction ─────────────────────────────────────────────────

static void test_subtraction() {
  std::printf("\n[RFC-0017-P6] Subtraction: x − y == x + (−y)  (2 048 trials)\n");

  std::mt19937_64 rng(0x1701'001B);
  std::uniform_int_distribution<int> dist(-1, 1);

  int failures = 0;
  for (int trial = 0; trial < 2048; ++trial) {
    const auto ld = random_digits(10, rng, dist);
    const auto rd = random_digits(10, rng, dist);
    const T81 lhs  = make_from_digits(ld);
    const T81 rhs  = make_from_digits(rd);
    const T81 sub  = lhs - rhs;
    const T81 add  = lhs + (-rhs);
    if (sub.data != add.data) ++failures;
  }
  check(failures == 0, "[RFC-0017-P6] x − y == x + (−y) for 2048 trials");
}

// ── [RFC-0018-P1] Multiplication value-correctness ───────────────────────────

static void test_multiplication_correctness() {
  std::printf("\n[RFC-0018-P1] Multiplication value-correctness vs scalar reference  (2 048 trials)\n");

  std::mt19937_64 rng(0x1801'001C);
  std::uniform_int_distribution<int> dist(-1, 1);

  int failures = 0;
  for (int trial = 0; trial < 2048; ++trial) {
    const auto ld = random_digits(7, rng, dist);
    const auto rd = random_digits(7, rng, dist);
    const T81 lhs = make_from_digits(ld);
    const T81 rhs = make_from_digits(rd);
    const T81 product = lhs * rhs;

    const int64_t lv = eval(lhs, 14);
    const int64_t rv = eval(rhs, 14);
    const int64_t pv = eval(product, 14);
    const int64_t expected = lv * rv;
    if (pv != expected) ++failures;
  }
  check(failures == 0, "[RFC-0018-P1] (lhs * rhs) matches scalar product for 2048 trials");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== T81 Native Property Tests (RFC-0017 + RFC-0018) ===\n");

  test_encoding_sanity();
  test_negation_involution();
  test_additive_identity();
  test_additive_inverse();
  test_addition_correctness();
  test_subtraction();
  test_multiplication_correctness();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
