#include <cassert>
#include <iostream>
#include "t81/fraction.hpp"
#include "t81/bigint.hpp"

int main() {
  using namespace t81;

  // Basic constructors
  auto a = T81Fraction::from_int(2);
  auto b = T81Fraction::from_int(3);

  // 2/1 + 3/1 = 5/1
  auto s = T81Fraction::add(a, b);
  assert(s.to_string() == "5/1");

  // 2/1 * 3/1 = 6/1
  auto m = T81Fraction::mul(a, b);
  assert(m.to_string() == "6/1");

  // Subtraction: 2/1 - 3/1 = -1/1
  auto d = T81Fraction::sub(a, b);
  assert(d.to_string() == "-1/1");

  // Division: (2/1) / (3/1) = 2/3 (reduced)
  auto q = T81Fraction::div(a, b);
  // String format depends on BigInt::to_string(); expect "2/3"
  assert(q.to_string() == "2/3");

  // Sign normalization: -1/(-2) == 1/2
  T81Fraction x(T81BigInt::from_i64(-1), T81BigInt::from_i64(-2));
  assert(x.to_string() == "1/2");

  // Zero canonicalization: 0/anything -> 0/1
  T81Fraction z(T81BigInt::from_i64(0), T81BigInt::from_i64(42));
  assert(z.to_string() == "0/1");

  std::cout << "fraction_roundtrip ok\n";
  return 0;
}
