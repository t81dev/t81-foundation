// src/bigint/divmod.cpp (skeleton)
#include "t81/bigint/divmod.hpp"
#include <cassert>

using namespace t81::bigint;

uint64_t mod_small(const BigInt& a, uint64_t d) {
  // branchless 128-accumulate over limbs
  __uint128_t rem = 0;
  for (ssize_t i = a.size()-1; i >= 0; --i) {
    rem = (rem << 64) | a.limb(i);
    rem %= d;
  }
  return (uint64_t)rem;
}

uint64_t div_small(const BigInt& a, uint64_t d, BigInt& q) {
  q = BigInt(a.size());
  __uint128_t carry = 0;
  for (ssize_t i = a.size()-1; i >= 0; --i) {
    __uint128_t cur = (carry << 64) | a.limb(i);
    q.limb_mut(i) = (uint64_t)(cur / d);
    carry = cur % d;
  }
  q.normalize();
  return (uint64_t)carry;
}

// Knuth’s Algorithm D (normalized) for medium sizes.
// NOTE: implement D1..D7; handle sign, leading zeros, normalization factor.
void divmod(const BigInt& a, const BigInt& b, BigInt& q, BigInt& r) {
  // dispatch small-divisor fast path
  if (b.is_small_u64()) {
    uint64_t rem = div_small(a, b.small_u64(), q);
    r = BigInt(rem);
    q.fix_sign(a.sign() ^ b.sign());
    r.fix_sign(a.sign());
    return;
  }
  // ... normalized long division skeleton here (placeholders) ...
  // normalize(a,b) -> an,bn, shift s; run trial quotient with 128-bit wide test;
  // unnormalize remainder by shifting back.
}
