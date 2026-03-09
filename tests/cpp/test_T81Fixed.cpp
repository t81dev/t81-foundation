#include <cassert>
#include <cmath>
#include <iostream>
#include "t81/types/T81Fixed.hpp"

using namespace t81;

int main() {
  std::cout << "Running T81Fixed tests...\n";

  using Fixed = T81Fixed<18, 9>;  // 18 integer trits, 9 fractional trits

  // Construction
  [[maybe_unused]] Fixed zero;  // Default constructor creates zero
  [[maybe_unused]] Fixed one = Fixed::from_double(1.0);
  [[maybe_unused]] Fixed half = Fixed::from_double(0.5);

  assert(zero.is_zero());
  assert(!one.is_zero());
  assert(one.to_double() > 0.9 && one.to_double() < 1.1);
  assert(half.to_double() > 0.4 && half.to_double() < 0.6);

  // Arithmetic
  [[maybe_unused]] Fixed sum = one + half;
  assert(sum.to_double() > 1.4 && sum.to_double() < 1.6);

  [[maybe_unused]] Fixed diff = one - half;
  assert(diff.to_double() > 0.4 && diff.to_double() < 0.6);

  [[maybe_unused]] Fixed prod = half * Fixed::from_double(2.0);
  assert(prod.to_double() > 0.9 && prod.to_double() < 1.1);

  // Comparison
  assert(one > half);
  assert(half < one);
  assert(one == one);

  // Negation
  [[maybe_unused]] Fixed neg = -one;
  assert(neg.to_double() < -0.9 && neg.to_double() > -1.1);

  std::cout << "All T81Fixed tests PASSED!\n";
  return 0;
}
