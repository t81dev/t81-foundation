#pragma once

#include "t81/types/T81Int.hpp"

// Forward declaration of T81Float for from_float/to_float
namespace t81::v1 {
template <std::size_t M, std::size_t E>
class T81Float;
}

namespace t81::core::detail {

// Fixed-point type wrapping T81Int.
// Represents value = V * 3^(-F)
// N: Total number of trits in storage
// F: Number of fractional trits
template <std::size_t N, std::size_t F>
class Fixed {
public:
  using Storage = T81Int<N>;
  static constexpr std::size_t kFractionalTrits = F;
  Storage v;  // raw scaled value

  constexpr Fixed() = default;

  constexpr explicit Fixed(const Storage& raw) : v(raw) {}

  // Construct from integer
  constexpr explicit Fixed(int i) : v(Storage(i)) { v <<= F; }

  // Basic arithmetic
  constexpr Fixed operator+(const Fixed& other) const { return Fixed(v + other.v); }

  constexpr Fixed operator-(const Fixed& other) const { return Fixed(v - other.v); }

  constexpr Fixed operator-() const { return Fixed(-v); }

  constexpr Fixed operator*(const Fixed& other) const {
    using WideStorage = T81Int<2 * N>;
    WideStorage wa(v);
    WideStorage wb(other.v);
    WideStorage prod = wa * wb;
    prod >>= F;
    return Fixed(Storage(prod));
  }

  constexpr Fixed operator/(const Fixed& other) const {
    using WideStorage = T81Int<2 * N>;
    WideStorage num(v);
    num <<= F;
    WideStorage den(other.v);
    WideStorage res = num / den;
    return Fixed(Storage(res));
  }

  constexpr std::strong_ordering operator<=>(const Fixed& other) const = default;

  constexpr bool is_negative() const { return v.sign_trit() == t81::Trit::N; }
  constexpr bool is_zero() const { return v.is_zero(); }

  static constexpr Fixed zero() { return Fixed(Storage(0)); }
  static constexpr Fixed one() { return Fixed(1); }

  // Constants helper: construct from integer parts of decimal representation
  static constexpr Fixed from_decimal(std::int64_t int_part, std::uint64_t frac_part,
                                      std::uint32_t frac_digits) {
    Storage s(int_part);
    s <<= F;

    if (frac_part != 0) {
      using WideStorage = T81Int<2 * N>;
      WideStorage num(static_cast<std::int64_t>(frac_part));
      num <<= F;
      WideStorage den(1);
      WideStorage ten(10);
      for (std::uint32_t i = 0; i < frac_digits; ++i) den *= ten;
      WideStorage frac_val = num / den;
      Storage final_frac(frac_val);
      if (int_part >= 0)
        s += final_frac;
      else
        s -= final_frac;
    }
    return Fixed(s);
  }

  // Conversion from T81Float
  template <std::size_t M, std::size_t E>
  static Fixed from_float(const v1::T81Float<M, E>& f) {
    if (f.is_zero()) return zero();
    std::int64_t exp = f.exponent();
    auto mant = f.mantissa();
    std::int64_t shift = exp - static_cast<std::int64_t>(M) + 1 + static_cast<std::int64_t>(F);
    Storage s(mant);
    if (f.sign_trit() == t81::Trit::N) s = -s;
    if (shift > 0)
      s <<= static_cast<size_t>(shift);
    else if (shift < 0)
      s >>= static_cast<size_t>(-shift);
    return Fixed(s);
  }

  // Conversion to T81Float
  template <std::size_t M, std::size_t E>
  v1::T81Float<M, E> to_float() const {
    if (v.is_zero()) return v1::T81Float<M, E>::zero();
    Storage abs_v = v;
    t81::Trit sign = t81::Trit::P;
    if (v.sign_trit() == t81::Trit::N) {
      sign = t81::Trit::N;
      abs_v = -v;
    }
    size_t lead = 0;
    for (size_t i = N; i-- > 0;) {
      if (abs_v[i] != t81::Trit::Z) {
        lead = i;
        break;
      }
    }
    std::int64_t shift = static_cast<std::int64_t>(lead) - static_cast<std::int64_t>(M - 1);
    T81Int<M> mant;
    if (shift > 0) {
      Storage rounded = abs_v;
      rounded >>= static_cast<size_t>(shift);
      for (size_t i = 0; i < M; ++i) mant[i] = rounded[i];
    } else {
      Storage shifted = abs_v;
      shifted <<= static_cast<size_t>(-shift);
      for (size_t i = 0; i < M; ++i) mant[i] = shifted[i];
    }
    return v1::T81Float<M, E>::from_components(
        sign, static_cast<std::int64_t>(lead) - static_cast<std::int64_t>(F), mant);
  }

  std::int64_t to_int64() const { return (v >> F).to_int64(); }
};

// Use wider storage (192 trits) and more fractional trits (80)
using DFixed = Fixed<192, 80>;

}  // namespace t81::core::detail
