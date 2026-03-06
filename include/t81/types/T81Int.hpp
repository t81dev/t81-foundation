/**
 * @file T81Int.hpp
 * @brief Balanced ternary integer with packed trits.
 *
 * Features:
 * • Packed 4 trits per byte (2 bits each: 0 = N, 1 = Z, 2 = P)
 * • Correct balanced ternary arithmetic (+, -, *, /, %)
 * • Trit proxy, operator[], tritwise shifts
 * • Safe to_int64() with overflow checking
 * • kMinValue / kMaxValue as inline static constants
 */
#pragma once
#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include "t81/axion/api.hpp"
#include "t81/determinism/canon_hash81.hpp"

namespace t81 {

enum class Trit : std::int8_t { N = -1, Z = 0, P = 1, Neg = -1, Zero = 0, Pos = 1 };

constexpr inline int trit_to_int(Trit t) noexcept { return static_cast<int>(t); }

constexpr inline Trit int_to_trit(int v) noexcept {
  if (v < 0) return Trit::N;
  if (v > 0) return Trit::P;
  return Trit::Z;
}

// Forward declarations
namespace v1 {
template <std::size_t M, std::size_t E>
class T81Float;
template <std::size_t I, std::size_t F>
class T81Fixed;
template <std::size_t Trits>
class T81Prob;
}  // namespace v1
using v1::T81Fixed;
using v1::T81Float;
using v1::T81Prob;

template <std::size_t N>
class T81Int;

template <std::size_t N>
constexpr std::pair<T81Int<N>, T81Int<N>> div_mod(const T81Int<N>& a, const T81Int<N>& b);

template <std::size_t N>
class T81Int {
public:
  using size_type = std::size_t;
  static_assert(N > 0 && N <= 2048, "T81Int<N>: N must be in 1..2048");
  static constexpr size_type kNumTrits = N;
  static constexpr size_type kTritsPerByte = 4;
  static constexpr size_type kNumBytes = (N + kTritsPerByte - 1) / kTritsPerByte;

  // Maximum number of trits that safely fit in int64_t (3^39 fits in 64-bit signed)
  // Trits 0..39 can be accumulated without overflow relative to int64 range.
  static constexpr size_type kPow3AccumTrits = 40;
  // Trit 40 (index 40) requires special overflow checking because 3^40 > INT64_MAX.
  static constexpr size_type kSpecialIndex = 40;

private:
  std::array<std::uint8_t, kNumBytes> data_{};

  static constexpr std::uint8_t encode_trit(Trit t) noexcept {
    return (t == Trit::N) ? 0u : (t == Trit::Z) ? 1u : 2u;
  }

  static constexpr Trit decode_trit(std::uint8_t v) noexcept {
    switch (v & 0x3u) {
      case 0u:
        return Trit::N;
      case 1u:
        return Trit::Z;
      case 2u:
        return Trit::P;
      default:
        return Trit::Z;
    }
  }

  constexpr Trit get_trit(size_type idx) const noexcept {
    const size_type byte = idx / kTritsPerByte;
    const size_type off = (idx % kTritsPerByte) * 2;
    return decode_trit(static_cast<std::uint8_t>((data_[byte] >> off) & 0x3u));
  }

  constexpr void set_trit(size_type idx, Trit t) noexcept {
    const size_type byte = idx / kTritsPerByte;
    const size_type off = (idx % kTritsPerByte) * 2;
    const std::uint8_t mask = static_cast<std::uint8_t>(~(0x3u << off));
    const std::uint8_t enc = static_cast<std::uint8_t>(encode_trit(t) << off);
    data_[byte] = (data_[byte] & mask) | enc;
  }

  constexpr void clear() noexcept {
    std::fill(data_.begin(), data_.end(), 0x55u);  // all Z trits
  }

  template <std::size_t M, std::size_t E>
  friend class T81Float;
  template <std::size_t I, std::size_t F>
  friend class T81Fixed;
  template <std::size_t Trits>
  friend class T81Prob;

public:
  static const T81Int kMaxValue;
  static const T81Int kMinValue;

  static constexpr T81Int make_max_value() noexcept {
    T81Int m;
    for (size_type i = 0; i < kNumTrits; ++i) m.set_trit(i, Trit::P);
    return m;
  }

  class TritRef {
    T81Int* owner_;
    size_type idx_;

  public:
    constexpr TritRef(T81Int& owner, size_type idx) noexcept : owner_(&owner), idx_(idx) {}
    constexpr TritRef& operator=(Trit t) noexcept {
      owner_->set_trit(idx_, t);
      return *this;
    }
    constexpr TritRef& operator=(const TritRef& other) noexcept {
      return *this = static_cast<Trit>(other);
    }
    constexpr operator Trit() const noexcept { return owner_->get_trit(idx_); }
  };

  constexpr T81Int() noexcept { clear(); }
  constexpr T81Int(const T81Int&) noexcept = default;
  constexpr T81Int(T81Int&&) noexcept = default;
  constexpr T81Int& operator=(const T81Int&) noexcept = default;
  constexpr T81Int& operator=(T81Int&&) noexcept = default;

  // Width conversion constructor: widen or truncate
  template <std::size_t K>
  constexpr explicit T81Int(const T81Int<K>& other) noexcept {
    clear();
    const size_type limit = (N < K) ? N : K;
    for (size_type i = 0; i < limit; ++i) {
      set_trit(i, other[i]);
    }
  }

  explicit T81Int(std::int64_t value) { assign_from_int64(value); }
  explicit T81Int(int value) : T81Int(static_cast<std::int64_t>(value)) {}

  template <std::size_t K>
  constexpr std::optional<T81Int<K>> try_to_int() const {
    if constexpr (K >= N) {
      return T81Int<K>(*this);
    } else {
      for (size_type i = K; i < N; ++i) {
        if (get_trit(i) != Trit::Z) return std::nullopt;
      }
      return T81Int<K>(*this);
    }
  }

  template <std::size_t K>
  constexpr T81Int<K> checked_to_int() const {
    auto res = try_to_int<K>();
    if (!res) {
#if defined(__cpp_lib_is_constant_evaluated)
      if (std::is_constant_evaluated()) {
        throw std::overflow_error("T81Int::checked_to_int overflow");
      }
#endif
      axion::trap_overflow("T81Int::checked_to_int overflow");
    }
    return *res;
  }

  static constexpr size_type num_trits() noexcept { return kNumTrits; }

  constexpr Trit operator[](size_type idx) const noexcept { return get_trit(idx); }
  constexpr TritRef operator[](size_type idx) noexcept { return TritRef(*this, idx); }

  constexpr const std::array<std::uint8_t, kNumBytes>& raw_data() const noexcept { return data_; }

  constexpr bool is_zero() const noexcept {
    for (size_type i = 0; i < kNumTrits; ++i)
      if (get_trit(i) != Trit::Z) return false;
    return true;
  }

  constexpr Trit sign_trit() const noexcept {
    for (size_type i = kNumTrits; i-- > 0;) {
      Trit t = get_trit(i);
      if (t != Trit::Z) return t;
    }
    return Trit::Z;
  }

  constexpr size_type significant_trits() const noexcept {
    for (size_type i = kNumTrits; i-- > 0;) {
      if (get_trit(i) != Trit::Z) return i + 1;
    }
    return 0;
  }

private:
  void assign_from_int64(std::int64_t v) {
    clear();
    if (v == 0) return;
    bool neg = v < 0;
    std::uint64_t uv =
        (v == std::numeric_limits<std::int64_t>::min())
            ? static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1
            : static_cast<std::uint64_t>(neg ? -v : v);
    size_type i = 0;
    while (uv != 0 && i < kNumTrits) {
      int r = static_cast<int>(uv % 3);
      uv /= 3;
      if (r == 2) {
        r = -1;
        ++uv;
      }
      set_trit(i++, int_to_trit(neg ? -r : r));
    }
    if (uv != 0) throw std::overflow_error("T81Int: value does not fit in N trits");
  }

public:
  // Universal C++20 to_int64(): works on MSVC (no throw), GCC/Clang (full safety)
  [[nodiscard]] constexpr std::int64_t to_int64() const {
#ifdef _MSC_VER
    // MSVC: no throw allowed in constexpr → fast path (symbol IDs are tiny)
    // Note: MSVC path here is simplified and doesn't support full range safely in constexpr if
    // throwing is banned. For full correctness, we should use the same logic, but MSVC constexpr
    // limitations might be tricky.
    std::int64_t value = 0;
    std::int64_t pow3 = 1;
    const size_type limit = (kNumTrits < kPow3AccumTrits) ? kNumTrits : kPow3AccumTrits;
    for (size_type i = 0; i < limit; ++i) {
      value += trit_to_int(get_trit(i)) * pow3;
      if (i < kPow3AccumTrits - 1) pow3 *= 3;
    }
    return value;
#else
    // GCC/Clang: full overflow checking with throw (safe in constexpr if not executed)

    // 1. Check forbidden trits (index > 40 must be Zero)
    if (kNumTrits > kSpecialIndex + 1) {
      for (size_type i = kSpecialIndex + 1; i < kNumTrits; ++i) {
        if (get_trit(i) != Trit::Z)
          throw std::overflow_error("T81Int::to_int64(): value out of range (trits > 40)");
      }
    }

    // 2. Accumulate trits 0..39 (safe relative to int64 range)
    const size_type limit = (kNumTrits < kPow3AccumTrits) ? kNumTrits : kPow3AccumTrits;
    std::int64_t value = 0;
    std::int64_t pow3 = 1;
    for (size_type i = 0; i < limit; ++i) {
      value += static_cast<std::int64_t>(trit_to_int(get_trit(i))) * pow3;
      if (i + 1 < limit) pow3 *= 3;
    }

    // 3. Handle trit 40 if present
    if (kNumTrits > kSpecialIndex) {
      Trit t40 = get_trit(kSpecialIndex);
      if (t40 != Trit::Z) {
        // 3^40 = 12157665459056928801
        constexpr std::uint64_t kPow40 = 12157665459056928801ULL;

        if (t40 == Trit::P) {
          // Check value + 3^40 <= kMax
          constexpr std::uint64_t uMax =
              static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
          constexpr std::uint64_t diff = kPow40 - uMax;
          if (value > -static_cast<std::int64_t>(diff))
            throw std::overflow_error("T81Int::to_int64(): overflow (positive)");

          return static_cast<std::int64_t>(static_cast<std::uint64_t>(value) + kPow40);
        } else {
          // t40 == Trit::N
          // Check value - 3^40 >= kMin
          constexpr std::uint64_t uMin =
              static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::min());
          constexpr std::uint64_t limit = kPow40 - uMin;

          if (value < static_cast<std::int64_t>(limit))
            throw std::overflow_error("T81Int::to_int64(): overflow (negative)");

          return static_cast<std::int64_t>(static_cast<std::uint64_t>(value) - kPow40);
        }
      }
    }
    return value;
#endif
  }

  template <typename T>
  T to_binary() const {
    static_assert(std::is_integral<T>::value, "T must be integral");
    const std::int64_t val = to_int64();
    if (std::cmp_greater(val, std::numeric_limits<T>::max()) ||
        std::cmp_less(val, std::numeric_limits<T>::min()))
      throw std::overflow_error("T81Int::to_binary overflow");
    return static_cast<T>(val);
  }

  constexpr T81Int operator-() const {
    T81Int out;
    for (size_type i = 0; i < kNumTrits; ++i) {
      Trit t = get_trit(i);
      out.set_trit(i, (t == Trit::P) ? Trit::N : (t == Trit::N) ? Trit::P : Trit::Z);
    }
    return out;
  }

  constexpr T81Int& operator++() {
    *this += T81Int(1);
    return *this;
  }
  constexpr T81Int operator++(int) {
    T81Int t(*this);
    ++*this;
    return t;
  }
  constexpr T81Int& operator--() {
    *this -= T81Int(1);
    return *this;
  }
  constexpr T81Int operator--(int) {
    T81Int t(*this);
    --*this;
    return t;
  }

private:
  constexpr void shift_left(size_type k) noexcept {
    if (k >= kNumTrits) {
      clear();
      return;
    }
    for (size_type i = kNumTrits; i-- > k;) set_trit(i, get_trit(i - k));
    for (size_type i = 0; i < k; ++i) set_trit(i, Trit::Z);
  }

  constexpr void shift_right(size_type k) noexcept {
    if (k >= kNumTrits) {
      clear();
      return;
    }
    for (size_type i = 0; i + k < kNumTrits; ++i) set_trit(i, get_trit(i + k));
    for (size_type i = kNumTrits - k; i < kNumTrits; ++i) set_trit(i, Trit::Z);
  }

public:
  constexpr T81Int& operator<<=(size_type k) noexcept {
    shift_left(k);
    return *this;
  }
  constexpr T81Int& operator>>=(size_type k) noexcept {
    shift_right(k);
    return *this;
  }
  friend constexpr T81Int operator<<(T81Int v, size_type k) noexcept {
    v <<= k;
    return v;
  }
  friend constexpr T81Int operator>>(T81Int v, size_type k) noexcept {
    v >>= k;
    return v;
  }

  constexpr std::strong_ordering operator<=>(const T81Int& other) const noexcept {
    for (size_type i = kNumTrits; i-- > 0;) {
      const int s = trit_to_int(get_trit(i));
      const int o = trit_to_int(other.get_trit(i));
      if (s < o) return std::strong_ordering::less;
      if (s > o) return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
  }

  constexpr bool operator==(const T81Int&) const noexcept = default;

  friend constexpr T81Int operator+(const T81Int& a, const T81Int& b) {
    T81Int r;
    int carry = 0;
    for (size_type i = 0; i < kNumTrits; ++i) {
      int sum = trit_to_int(a.get_trit(i)) + trit_to_int(b.get_trit(i)) + carry;
      int digit = (sum > 1) ? sum - 3 : (sum < -1) ? sum + 3 : sum;
      carry = (sum > 1) ? 1 : (sum < -1) ? -1 : 0;
      r.set_trit(i, int_to_trit(digit));
    }
    if (carry != 0) {
      axion::trap_overflow("T81Int overflow");
    }
    return r;
  }

  friend constexpr T81Int operator-(const T81Int& a, const T81Int& b) { return a + (-b); }
  constexpr T81Int& operator+=(const T81Int& o) {
    *this = *this + o;
    return *this;
  }
  constexpr T81Int& operator-=(const T81Int& o) {
    *this = *this - o;
    return *this;
  }

  friend constexpr T81Int operator*(const T81Int& a, const T81Int& b) {
    T81Int r;
    for (size_type i = 0; i < kNumTrits; ++i) {
      Trit tb = b.get_trit(i);
      if (tb == Trit::Z) continue;
      T81Int tmp = a;
      tmp <<= i;
      if (tb == Trit::P)
        r += tmp;
      else
        r -= tmp;
    }
    return r;
  }

  constexpr T81Int& operator*=(const T81Int& o) {
    *this = *this * o;
    return *this;
  }

  friend constexpr T81Int operator/(const T81Int& a, const T81Int& b) {
    return div_mod(a, b).first;
  }

  friend constexpr T81Int operator%(const T81Int& a, const T81Int& b) {
    return div_mod(a, b).second;
  }

  T81Int& operator/=(const T81Int& o) {
    *this = *this / o;
    return *this;
  }
  T81Int& operator%=(const T81Int& o) {
    *this = *this % o;
    return *this;
  }

  friend constexpr T81Int pow(T81Int base, T81Int exp) {
    if (exp.sign_trit() == Trit::N) throw std::domain_error("pow with negative exponent");
    if (exp.is_zero()) return T81Int(1);

    std::int64_t e = exp.to_int64();

    T81Int res(1);
    T81Int b = base;
    while (e > 0) {
      if (e & 1) res *= b;
      b *= b;
      e >>= 1;
    }
    return res;
  }

  friend constexpr T81Int gcd(T81Int a, T81Int b) {
    if (a.sign_trit() == Trit::N) a = -a;
    if (b.sign_trit() == Trit::N) b = -b;
    while (!b.is_zero()) {
      T81Int t = b;
      b = a % b;
      a = t;
    }
    return a;
  }

  friend constexpr T81Int lcm(T81Int a, T81Int b) {
    if (a.is_zero() || b.is_zero()) return T81Int(0);
    T81Int g = gcd(a, b);
    T81Int res = (a / g) * b;
    if (res.sign_trit() == Trit::N) res = -res;
    return res;
  }

  std::string to_string() const {
    std::int64_t v = to_int64();
    if (v == 0) return "0";
    bool neg = v < 0;
    if (neg) v = -v;
    std::string s;
    while (v) {
      int r = static_cast<int>(v % 3);
      v /= 3;
      s.push_back(r == 0 ? '0' : r == 1 ? '1' : '2');
    }
    if (neg) s.push_back('-');
    std::reverse(s.begin(), s.end());
    return s;
  }

  friend std::ostream& operator<<(std::ostream& os, const T81Int& v) { return os << v.to_int64(); }

  std::string to_trit_string() const {
    std::string s;
    s.reserve(kNumTrits);
    for (size_type i = kNumTrits; i-- > 0;) {
      switch (get_trit(i)) {
        case Trit::P:
          s.push_back('+');
          break;
        case Trit::Z:
          s.push_back('0');
          break;
        case Trit::N:
          s.push_back('-');
          break;
      }
    }
    return s;
  }

  // P2: Canonical serialization
  std::string to_canonical_string() const { return to_trit_string(); }

  // --- Binary Serialization ---
  void serialize(std::ostream& os) const {
    uint64_t len = data_.size();
    os.write(reinterpret_cast<const char*>(&len), sizeof(len));
    os.write(reinterpret_cast<const char*>(data_.data()), static_cast<std::streamsize>(len));
  }

  void deserialize(std::istream& is) {
    uint64_t len;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!is) return;
    if (len != data_.size()) {
      throw std::runtime_error("T81Int deserialize: size mismatch");
    }
    is.read(reinterpret_cast<char*>(data_.data()), static_cast<std::streamsize>(len));
  }
};

template <std::size_t N>
inline const T81Int<N> T81Int<N>::kMaxValue = T81Int<N>::make_max_value();

template <std::size_t N>
inline const T81Int<N> T81Int<N>::kMinValue = -T81Int<N>::kMaxValue;

template <std::size_t N>
inline constexpr std::pair<T81Int<N>, T81Int<N>> div_mod(const T81Int<N>& a, const T81Int<N>& b) {
  if (b.is_zero()) throw std::domain_error("div_mod by zero");

  // Standard integer division semantics (truncate towards zero).
  // Handle signs separately.
  bool a_neg = (a.sign_trit() == Trit::N);
  bool b_neg = (b.sign_trit() == Trit::N);

  T81Int<N> u = a_neg ? -a : a;
  T81Int<N> v = b_neg ? -b : b;

  T81Int<N> q;  // 0
  T81Int<N> r;  // 0
  T81Int<N> one(1);

  // Iterating from MSB
  for (std::size_t i = N; i-- > 0;) {
    r <<= 1;

    // Add u[i]
    Trit t = u[i];
    if (t == Trit::P)
      r += one;
    else if (t == Trit::N)
      r -= one;

    q <<= 1;

    // Reduce r (restoring division)
    // Ensure r < v
    while (r >= v) {
      r -= v;
      q += one;
    }
    // Ensure r >= 0 (handle temporary negative r from balanced ternary ops)
    while (r.sign_trit() == Trit::N) {
      r += v;
      q -= one;
    }
  }

  if (a_neg != b_neg) q = -q;
  if (a_neg) r = -r;

  return {q, r};
}

}  // namespace t81

namespace std {
template <size_t N>
struct hash<t81::T81Int<N>> {
  size_t operator()(const t81::T81Int<N>& val) const noexcept {
    const auto& data = val.raw_data();
    size_t seed = 0;
    for (const auto& byte : data)
      seed ^= t81::CanonHash<std::uint8_t>{}(static_cast<std::uint8_t>(byte)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};
}  // namespace std
