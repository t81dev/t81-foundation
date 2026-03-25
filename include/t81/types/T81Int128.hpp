#pragma once
#include <cstdint>

#if defined(_MSC_VER) && defined(_WIN64)
#include <intrin.h>

#if defined(__clang__)
inline uint64_t clang_udiv128(uint64_t high, uint64_t low, uint64_t divisor, uint64_t *remainder) {
    if (divisor == 0) {
        if (remainder) *remainder = 0;
        return 0;
    }
    if (high >= divisor) {
        if (remainder) *remainder = 0;
        return 0; // Overflow condition not fully handled as per _udiv128 spec, but we mimic MSVC's constraint
    }

    uint64_t q = 0;
    uint64_t r = high;
    for (int i = 0; i < 64; ++i) {
        uint64_t next_bit = (low >> 63) & 1;
        low <<= 1;
        r = (r << 1) | next_bit;
        if (r >= divisor) {
            r -= divisor;
            q = (q << 1) | 1;
        } else {
            q <<= 1;
        }
    }
    if (remainder) *remainder = r;
    return q;
}
#define T81_UDIV128 clang_udiv128
#else
#define T81_UDIV128 _udiv128
#endif

namespace t81::v1::detail {
struct int128_t {
  uint64_t lo;
  int64_t hi;

  int128_t() : lo(0), hi(0) {}
  int128_t(int64_t v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? -1 : 0) {}
  int128_t(uint64_t l, int64_t h) : lo(l), hi(h) {}

  bool is_neg() const { return hi < 0; }

  int128_t operator-() const {
    uint64_t l = ~lo + 1;
    int64_t h = ~hi + (lo == 0 ? 1 : 0);
    return {l, h};
  }

  int128_t operator+(const int128_t& o) const {
    uint64_t l = lo + o.lo;
    int64_t h = hi + o.hi + (l < lo ? 1 : 0);
    return {l, h};
  }

  int128_t operator-(const int128_t& o) const {
    uint64_t l = lo - o.lo;
    int64_t h = hi - o.hi - (lo < o.lo ? 1 : 0);
    return {l, h};
  }

  int128_t operator*(const int64_t& o) const {
    uint64_t h1;
    uint64_t o_abs = o < 0 ? -o : o;
    int128_t lhs = is_neg() ? -(*this) : *this;
    uint64_t l1 = _umul128(lhs.lo, o_abs, &h1);
    int64_t h2 = h1 + (lhs.hi * o_abs);
    int128_t res = {l1, h2};
    return ((is_neg() && o > 0) || (!is_neg() && o < 0)) ? -res : res;
  }

  int128_t operator*(const int128_t& o) const {
    uint64_t h1;
    int128_t lhs = is_neg() ? -(*this) : *this;
    int128_t rhs = o.is_neg() ? -o : o;
    uint64_t l1 = _umul128(lhs.lo, rhs.lo, &h1);
    int64_t h2 = h1 + (lhs.hi * rhs.lo) + (lhs.lo * rhs.hi);
    int128_t res = {l1, h2};
    return (is_neg() != o.is_neg()) ? -res : res;
  }

  // WARNING: This is a partial implementation. Divisors are truncated to 64-bit.
  // This is safe ONLY because T81BigInt currently only divides by 64-bit Radix bounds.
  int128_t operator/(const int128_t& o) const {
    int64_t d = (int64_t)o.lo;
    if (o.hi < 0) d = -d;
    bool neg = (is_neg() != o.is_neg());
    int128_t num = is_neg() ? -(*this) : *this;
    uint64_t rem;
    if (num.hi >= (uint64_t)d) {
      return {0, 0};
    }
    uint64_t q = T81_UDIV128(num.hi, num.lo, (uint64_t)d, &rem);
    int128_t res = {q, 0};
    return neg ? -res : res;
  }

  // WARNING: This is a partial implementation. Divisors are truncated to 64-bit.
  // This is safe ONLY because T81BigInt currently only divides by 64-bit Radix bounds.
  int128_t operator%(const int128_t& o) const {
    int64_t d = (int64_t)o.lo;
    if (o.hi < 0) d = -d;
    bool neg = is_neg();
    int128_t num = is_neg() ? -(*this) : *this;
    uint64_t rem;
    if (num.hi >= (uint64_t)d) {
      return {0, 0};
    }
    T81_UDIV128(num.hi, num.lo, (uint64_t)d, &rem);
    int128_t res = {rem, 0};
    return neg ? -res : res;
  }

  int128_t& operator+=(const int128_t& o) {
    *this = *this + o;
    return *this;
  }
  int128_t& operator-=(const int128_t& o) {
    *this = *this - o;
    return *this;
  }

  bool operator>=(int64_t v) const {
    int128_t o(v);
    if (hi != o.hi) return hi > o.hi;
    return lo >= o.lo;
  }

  bool operator>(const int128_t& o) const {
    if (hi != o.hi) return hi > o.hi;
    return lo > o.lo;
  }

  bool operator==(const int128_t& o) const { return hi == o.hi && lo == o.lo; }

  bool operator!=(const int128_t& o) const { return !(*this == o); }

  explicit operator int64_t() const { return (int64_t)lo; }
};
}  // namespace t81::v1::detail

#elif defined(_MSC_VER)

namespace t81::v1::detail {
// For 32-bit Windows MSVC, just use int64_t.
// NOTE: This breaks big int operations requiring 128-bit results.
typedef std::int64_t int128_t;
}  // namespace t81::v1::detail

#else

namespace t81::v1::detail {
// GCC, Clang on Linux/macOS/Windows, or MinGW which has __int128
__extension__ typedef __int128 int128_t;
}  // namespace t81::v1::detail

#endif
