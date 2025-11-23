// src/bigint/gcd.cpp
#include "t81/bigint/gcd.hpp"

using namespace t81::bigint;

static inline bool even(const BigInt& x){ return x.limb(0) % 2 == 0; }

BigInt gcd(BigInt a, BigInt b) {
  if (a.is_zero()) return abs(b);
  if (b.is_zero()) return abs(a);
  unsigned k = 0;
  while (even(a) && even(b)) { a >>= 1; b >>= 1; ++k; }
  while (!a.is_zero()) {
    while (even(a)) a >>= 1;
    while (even(b)) b >>= 1;
    if (a >= b) a = (a - b) >> 1; else b = (b - a) >> 1;
  }
  return b << k;
}
