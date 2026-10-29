#pragma once
#include <bit>
#include <cstdint>
#include <stdexcept>
using u64 = std::uint64_t;
// PCG XSL-RR 128/64, Section 6 seed and stream. Two limbs avoid
// implementation-specific 128-bit types (including on MSVC).
struct PCG64 {
  u64 hi = 0, lo = 0;
  static u64 multiply_high(u64 a, u64 b) {
    const u64 a0 = a & 0xffffffffULL, a1 = a >> 32;
    const u64 b0 = b & 0xffffffffULL, b1 = b >> 32;
    const u64 low = a0 * b0;
    const u64 middle = a1 * b0 + (low >> 32);
    const u64 carry = (middle & 0xffffffffULL) + a0 * b1;
    return a1 * b1 + (middle >> 32) + (carry >> 32);
  }
  void advance() {
    constexpr u64 mh = 0x2360ED051FC65DA4ULL, ml = 0x4385C511560B53A9ULL;
    const u64 product = lo * ml;
    hi = multiply_high(lo, ml) + hi * ml + lo * mh + (product > UINT64_MAX - 3 ? 1 : 0);
    lo = product + 3;
  }
  PCG64() {
    advance();
    const u64 old = lo;
    lo += 0x5F53454544303131ULL;
    hi += 0x5438315F434D5033ULL + (lo < old ? 1 : 0);
    advance();
  }
  u64 next() {
    const u64 x = hi ^ lo;
    const int rotation = static_cast<int>(hi >> 58);
    advance();
    return std::rotr(x, rotation);
  }
  u64 bounded(u64 n) {
    if (n == 0) throw std::invalid_argument("ZERO_BOUND");
    const u64 threshold = -n % n;
    for (;;) {
      const u64 x = next();
      if (x >= threshold) return x % n;
    }
  }
  double prob() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
};
