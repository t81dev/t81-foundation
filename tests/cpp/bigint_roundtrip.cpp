#include <cassert>
#include <iostream>
#include <vector>
#include "t81/bigint.hpp"

using t81::T243BigInt;

static void assert_eq(const T243BigInt& a, const T243BigInt& b) {
  assert(T243BigInt::cmp(a,b) == 0);
}

int main() {
  // from_i64 / zero / one
  auto z = T243BigInt::from_i64(0);
  auto o = T243BigInt::from_i64(1);
  assert(T243BigInt::is_zero(z));
  assert(T243BigInt::is_one(o));

  // add/sub with small ints
  auto a = T243BigInt::from_i64(1234);
  auto b = T243BigInt::from_i64(-567);
  auto s = T243BigInt::add(a, b);                 // 1234 + (-567) = 667
  assert_eq(s, T243BigInt::from_i64(667));
  auto d = T243BigInt::sub(a, b);                 // 1234 - (-567) = 1801
  assert_eq(d, T243BigInt::from_i64(1801));

  // mul signs and zero
  auto m1 = T243BigInt::mul(a, b);                // 1234 * (-567) = -699,? -> compute with 64-bit then compare
  assert_eq(m1, T243BigInt::from_i64(1234ll * -567ll));
  auto m0 = T243BigInt::mul(a, z);                // x * 0 = 0
  assert(T243BigInt::is_zero(m0));

  // gcd (Euclidean, slow but correct for tiny values)
  assert_eq(T243BigInt::gcd(T243BigInt::from_i64(54), T243BigInt::from_i64(24)),
            T243BigInt::from_i64(6));
  assert_eq(T243BigInt::gcd(T243BigInt::from_i64(-54), T243BigInt::from_i64(24)),
            T243BigInt::from_i64(6));
  assert_eq(T243BigInt::gcd(T243BigInt::from_i64(0), T243BigInt::from_i64(7)),
            T243BigInt::from_i64(7));

  // exact division (no remainder) small cases
  auto n = T243BigInt::from_i64(84);
  auto q = T243BigInt::div(n, T243BigInt::from_i64(7)); // 84 / 7 = 12
  assert_eq(q, T243BigInt::from_i64(12));

  // mod sanity: a = b*q + r, with r < b (by magnitude)
  auto r = T243BigInt::mod(n, T243BigInt::from_i64(7));
  assert(T243BigInt::is_zero(r));
  auto r2 = T243BigInt::mod(T243BigInt::from_i64(100), T243BigInt::from_i64(9));
  assert_eq(r2, T243BigInt::from_i64(100 % 9));

  // cmp ordering
  assert(T243BigInt::cmp(T243BigInt::from_i64(-1), T243BigInt::from_i64(0)) < 0);
  assert(T243BigInt::cmp(T243BigInt::from_i64(0), T243BigInt::from_i64(0)) == 0);
  assert(T243BigInt::cmp(T243BigInt::from_i64(5), T243BigInt::from_i64(3)) > 0);

  std::cout << "bigint_roundtrip ok\n";
  return 0;
}
