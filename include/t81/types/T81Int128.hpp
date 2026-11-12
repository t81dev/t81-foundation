#pragma once
#include <cstdint>

#if defined(_MSC_VER) && defined(_WIN64)
#include <intrin.h>

namespace t81::v1::detail {
struct int128_t {
  uint64_t lo;
  int64_t hi;

  int128_t() : lo(0), hi(0) {}
  int128_t(int64_t v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? -1 : 0) {}
  int128_t(uint64_t l, int64_t h) : lo(l), hi(h) {}

  bool is_neg() const { return hi < 0; }

  int128_t operator-() const {
    uint64_t l = ~lo + 1ULL;
    uint64_t h = ~static_cast<uint64_t>(hi) + (lo == 0 ? 1ULL : 0ULL);
    return {l, static_cast<int64_t>(h)};
  }

  int128_t operator+(const int128_t& o) const {
    uint64_t l = lo + o.lo;
    uint64_t h = static_cast<uint64_t>(hi) + static_cast<uint64_t>(o.hi) + (l < lo ? 1ULL : 0ULL);
    return {l, static_cast<int64_t>(h)};
  }

  int128_t operator-(const int128_t& o) const {
    uint64_t l = lo - o.lo;
    uint64_t h =
        static_cast<uint64_t>(hi) - static_cast<uint64_t>(o.hi) - (lo < o.lo ? 1ULL : 0ULL);
    return {l, static_cast<int64_t>(h)};
  }

  int128_t operator*(const int64_t& o) const {
    uint64_t h1 = 0;
    uint64_t o_abs = o < 0 ? (0ULL - static_cast<uint64_t>(o)) : static_cast<uint64_t>(o);
    int128_t lhs = is_neg() ? -(*this) : *this;
    uint64_t l1 = _umul128(lhs.lo, o_abs, &h1);
    uint64_t h2 = h1 + static_cast<uint64_t>(lhs.hi) * o_abs;
    int128_t res = {l1, static_cast<int64_t>(h2)};
    return (is_neg() != (o < 0)) ? -res : res;
  }

  int128_t operator*(const int128_t& o) const {
    uint64_t h1 = 0;
    int128_t lhs = is_neg() ? -(*this) : *this;
    int128_t rhs = o.is_neg() ? -o : o;
    uint64_t l1 = _umul128(lhs.lo, rhs.lo, &h1);
    uint64_t h2 =
        h1 + static_cast<uint64_t>(lhs.hi) * rhs.lo + lhs.lo * static_cast<uint64_t>(rhs.hi);
    int128_t res = {l1, static_cast<int64_t>(h2)};
    return (is_neg() != o.is_neg()) ? -res : res;
  }

  // WARNING: This is a partial implementation. Divisors are truncated to 64-bit.
  // This is safe ONLY because T81BigInt currently only divides by 64-bit Radix bounds.
  int128_t operator/(const int128_t& o) const {
    int128_t den = o.is_neg() ? -o : o;
    uint64_t d = den.lo;
    if (d == 0) {
      return {0, 0};
    }
    bool neg = (is_neg() != o.is_neg());
    int128_t num = is_neg() ? -(*this) : *this;
    uint64_t rem = 0;
    if (static_cast<uint64_t>(num.hi) >= d) {
      return {0, 0};
    }
    uint64_t q = _udiv128(static_cast<uint64_t>(num.hi), num.lo, d, &rem);
    int128_t res = {q, 0};
    return neg ? -res : res;
  }

  // WARNING: This is a partial implementation. Divisors are truncated to 64-bit.
  // This is safe ONLY because T81BigInt currently only divides by 64-bit Radix bounds.
  int128_t operator%(const int128_t& o) const {
    int128_t den = o.is_neg() ? -o : o;
    uint64_t d = den.lo;
    if (d == 0) {
      return {0, 0};
    }
    bool neg = is_neg();
    int128_t num = is_neg() ? -(*this) : *this;
    uint64_t rem = 0;
    if (static_cast<uint64_t>(num.hi) >= d) {
      return {0, 0};
    }
    _udiv128(static_cast<uint64_t>(num.hi), num.lo, d, &rem);
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
