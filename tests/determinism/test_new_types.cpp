#undef NDEBUG
#include <cassert>
#include <iostream>
#include <string>

#include "t81/types/T81Prob.hpp"
#include "t81/types/T81Qutrit.hpp"
#include "t81/types/T81Quaternion.hpp"
#include "t81/types/T81Uint.hpp"

// ---------------------------------------------------------------------------
// T81Qutrit (= T81Int<2>) — construction and round-trip via to_int64
// ---------------------------------------------------------------------------
void test_qutrit_determinism() {
  using t81::T81Qutrit;
  using namespace t81::qutrit;

  // Construction determinism: same literal → identical value
  T81Qutrit a(0), b(0);
  assert(a == b);

  T81Qutrit p = ONE, q = ONE;
  assert(p == q);

  T81Qutrit m = TWO, n = TWO;
  assert(m == n);

  // Round-trip via to_int64: stable across two evaluations
  const auto v0a = ZERO.to_int64();
  const auto v0b = T81Qutrit(0).to_int64();
  assert(v0a == v0b);

  const auto v1a = ONE.to_int64();
  const auto v1b = T81Qutrit(1).to_int64();
  assert(v1a == v1b);

  // Arithmetic determinism
  T81Qutrit sum1 = ONE + ZERO;
  T81Qutrit sum2 = T81Qutrit(1) + T81Qutrit(0);
  assert(sum1 == sum2);

  std::cout << "test_qutrit_determinism PASSED\n";
}

// ---------------------------------------------------------------------------
// T81Prob — construction and round-trip via raw().to_int64()
// ---------------------------------------------------------------------------
void test_prob_determinism() {
  using t81::T81Prob27;

  // Construction determinism: identical probability → identical storage
  const T81Prob27 a = T81Prob27::from_prob(0.75);
  const T81Prob27 b = T81Prob27::from_prob(0.75);
  assert(a == b);

  // Zero (p=0.5) is always the same
  const T81Prob27 za = T81Prob27::zero();
  const T81Prob27 zb = T81Prob27::zero();
  assert(za == zb);

  // Round-trip: raw int storage is stable
  const auto raw_a = a.raw().to_int64();
  const auto raw_b = b.raw().to_int64();
  assert(raw_a == raw_b);

  const auto raw_za = za.raw().to_int64();
  const auto raw_zb = zb.raw().to_int64();
  assert(raw_za == raw_zb);

  // Arithmetic determinism
  const T81Prob27 sum1 = a + za;
  const T81Prob27 sum2 = b + zb;
  assert(sum1 == sum2);

  std::cout << "test_prob_determinism PASSED\n";
}

// ---------------------------------------------------------------------------
// T81Quaternion — construction, operator==, and serialize_canonical
// ---------------------------------------------------------------------------
void test_quaternion_determinism() {
  using t81::T81Quaternion;

  // Construction determinism: identity == identity
  const T81Quaternion id1 = T81Quaternion::identity();
  const T81Quaternion id2 = T81Quaternion::identity();
  assert(id1 == id2);

  // Canonical serialization is stable
  const std::string s1 = id1.serialize_canonical();
  const std::string s2 = id2.serialize_canonical();
  assert(s1 == s2);
  assert(!s1.empty());

  // Arithmetic determinism: Hamilton product of identity with itself
  const T81Quaternion prod1 = id1 * id1;
  const T81Quaternion prod2 = id2 * id2;
  assert(prod1 == prod2);

  const std::string ps1 = prod1.serialize_canonical();
  const std::string ps2 = prod2.serialize_canonical();
  assert(ps1 == ps2);

  std::cout << "test_quaternion_determinism PASSED\n";
}

// ---------------------------------------------------------------------------
// T81UInt — construction, arithmetic, and serialize_canonical
// ---------------------------------------------------------------------------
void test_uint_determinism() {
  using t81::T81UInt;
  using U81 = T81UInt<28>;  // 28 trits — multiple of 4, matches test_T81Uint.cpp

  // Construction determinism: same value → identical
  const U81 a(42);
  const U81 b(42);
  assert(a == b);

  // Canonical serialization is stable
  const std::string sa = a.serialize_canonical();
  const std::string sb = b.serialize_canonical();
  assert(sa == sb);
  assert(!sa.empty());

  // Zero construction is stable
  const U81 za(0);
  const U81 zb(0);
  assert(za == zb);
  assert(za.serialize_canonical() == zb.serialize_canonical());

  // Arithmetic determinism
  const U81 sum1 = a + za;
  const U81 sum2 = b + zb;
  assert(sum1 == sum2);
  assert(sum1.serialize_canonical() == sum2.serialize_canonical());

  // Negative input clamped to zero (unsigned semantics)
  const U81 neg(-5);
  const U81 neg2(-5);
  assert(neg == neg2);
  assert(neg.serialize_canonical() == neg2.serialize_canonical());

  std::cout << "test_uint_determinism PASSED\n";
}

int main() {
  test_qutrit_determinism();
  test_prob_determinism();
  test_quaternion_determinism();
  test_uint_determinism();
  std::cout << "All new-type determinism tests PASSED!\n";
  return 0;
}
