#pragma once
#include <cstdint>

// CMake probes compilation AND linking: parsing __int128 alone does not prove
// that Windows supplies the signed division/remainder runtime helpers.
// Standalone Windows consumers conservatively use the portable implementation.
#ifndef T81_HAS_NATIVE_INT128
#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER)
#define T81_HAS_NATIVE_INT128 1
#else
#define T81_HAS_NATIVE_INT128 0
#endif
#endif

#if T81_HAS_NATIVE_INT128 && !defined(T81_FORCE_PORTABLE_INT128)
#define T81_INT128_BACKEND_PORTABLE 0
namespace t81::v1::detail {
__extension__ typedef __int128 int128_t;
}  // namespace t81::v1::detail
#else
#define T81_INT128_BACKEND_PORTABLE 1
#include <limits>
#include <stdexcept>

namespace t81::v1::detail {
// Two's-complement 128-bit storage. Add/subtract/multiply retain the low 128
// bits using unsigned arithmetic; callers must keep signed results representable.
// Division is full-width, truncates toward zero, and gives the remainder the
// dividend's sign. Zero divisors and MIN/-1 fail explicitly rather than returning 0.
struct int128_t {
  uint64_t lo;
  int64_t hi;

  int128_t() : lo(0), hi(0) {}
  int128_t(int64_t v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? -1 : 0) {}
  int128_t(uint64_t l, int64_t h) : lo(l), hi(h) {}

  bool is_neg() const { return hi < 0; }

  int128_t operator-() const {
    return {~lo + 1ULL, static_cast<int64_t>(~static_cast<uint64_t>(hi) + (lo == 0 ? 1ULL : 0ULL))};
  }

  int128_t operator+(const int128_t& o) const {
    uint64_t l = lo + o.lo;
    return {l, static_cast<int64_t>(static_cast<uint64_t>(hi) + static_cast<uint64_t>(o.hi) +
                                    (l < lo ? 1ULL : 0ULL))};
  }

  int128_t operator-(const int128_t& o) const {
    return {lo - o.lo,
            static_cast<int64_t>(static_cast<uint64_t>(hi) - static_cast<uint64_t>(o.hi) -
                                 (lo < o.lo ? 1ULL : 0ULL))};
  }

  int128_t operator*(const int64_t& o) const { return *this * int128_t(o); }

  int128_t operator*(const int128_t& o) const {
    constexpr uint64_t mask = 0xffffffffULL;
    const uint64_t a0 = lo & mask, a1 = lo >> 32;
    const uint64_t b0 = o.lo & mask, b1 = o.lo >> 32;
    const uint64_t w0 = a0 * b0;
    const uint64_t t = a1 * b0 + (w0 >> 32);
    const uint64_t w1 = (t & mask) + a0 * b1;
    const uint64_t high = a1 * b1 + (t >> 32) + (w1 >> 32) + static_cast<uint64_t>(hi) * o.lo +
                          lo * static_cast<uint64_t>(o.hi);
    return {(w1 << 32) | (w0 & mask), static_cast<int64_t>(high)};
  }

  int128_t operator/(const int128_t& o) const {
    int128_t q, r;
    divide(*this, o, q, r);
    return q;
  }

  int128_t operator%(const int128_t& o) const {
    int128_t q, r;
    divide(*this, o, q, r);
    return r;
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
    return *this > o || *this == o;
  }
  bool operator>(const int128_t& o) const { return hi != o.hi ? hi > o.hi : lo > o.lo; }
  bool operator==(const int128_t& o) const { return hi == o.hi && lo == o.lo; }
  bool operator!=(const int128_t& o) const { return !(*this == o); }
  explicit operator int64_t() const { return static_cast<int64_t>(lo); }

private:
  static bool unsigned_ge(const int128_t& a, const int128_t& b) {
    const uint64_t ah = static_cast<uint64_t>(a.hi), bh = static_cast<uint64_t>(b.hi);
    return ah != bh ? ah > bh : a.lo >= b.lo;
  }

  static void divide(const int128_t& a, const int128_t& b, int128_t& q, int128_t& r) {
    if (b == int128_t(0)) throw std::domain_error("int128 division by zero");
    if (a.lo == 0 && a.hi == std::numeric_limits<int64_t>::min() && b == int128_t(-1)) {
      throw std::overflow_error("int128 division overflow");
    }
    const int128_t numerator = a.is_neg() ? -a : a;
    const int128_t denominator = b.is_neg() ? -b : b;
    q = int128_t(0);
    r = int128_t(0);
    for (unsigned i = 128; i-- > 0;) {
      const uint64_t bit = i < 64 ? (numerator.lo >> i) & 1ULL
                                  : (static_cast<uint64_t>(numerator.hi) >> (i - 64)) & 1ULL;
      r.hi = static_cast<int64_t>((static_cast<uint64_t>(r.hi) << 1) | (r.lo >> 63));
      r.lo = (r.lo << 1) | bit;
      if (unsigned_ge(r, denominator)) {
        r -= denominator;
        if (i < 64) {
          q.lo |= uint64_t{1} << i;
        } else {
          q.hi = static_cast<int64_t>(static_cast<uint64_t>(q.hi) | (uint64_t{1} << (i - 64)));
        }
      }
    }
    if (a.is_neg() != b.is_neg()) q = -q;
    if (a.is_neg()) r = -r;
  }
};
}  // namespace t81::v1::detail
#endif

static_assert(sizeof(t81::v1::detail::int128_t) == 16, "128-bit intermediates are required");
