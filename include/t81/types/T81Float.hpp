/**
 * @file T81Float.hpp
 * @brief Balanced ternary floating-point backed by T81Int storage.
 *
 * Model:
 * • Storage: T81Int<1 + E + M> (sign + exponent + mantissa trits)
 * • Correct balanced-ternary biased exponent (bias = (3^E - 1)/2)
 * • Special values:
 *   - Zero    (exp = 0,            mant = 0)
 *   - Subnorm (exp = 0,            mant != 0)
 *   - Infinity(exp = all P trits,  mant = 0)
 *   - NaE     (Not-an-Entity,      exp = all P trits, mant != 0)
 */

#pragma once

#include "t81/types/T81Int.hpp"

// We try to avoid host math in deterministic mode, but legacy/conversion may
// still need it. Phase 1: Only dmath handles transcendentals.
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstdlib>  // fabsl, powl
#include <limits>
#include <numbers>
#include <string>
#include "t81/math/t81_soft_math/t81_soft_math.hpp"
#include "t81/types/detail/dmath.hpp"

namespace t81::v1 {

template <std::size_t M, std::size_t E>
class T81Float;

}  // namespace t81::v1

// Deterministic Math Backend
#include "t81/types/detail/dmath.hpp"

namespace t81::v1 {
template <std::size_t M, std::size_t E>
class T81Float;
}

namespace t81::core::math::t81_soft_math {
template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_sin(const t81::v1::T81Float<M, E>&);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_cos(const t81::v1::T81Float<M, E>&);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_exp(const t81::v1::T81Float<M, E>&);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_log(const t81::v1::T81Float<M, E>&);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_sqrt(const t81::v1::T81Float<M, E>&);
}  // namespace t81::core::math::t81_soft_math

namespace t81::v1 {

// Forward declarations (arithmetic surface)
template <std::size_t M, std::size_t E>
T81Float<M, E> operator+(T81Float<M, E> a, T81Float<M, E> b) noexcept;
template <std::size_t M, std::size_t E>
T81Float<M, E> operator-(T81Float<M, E> a, T81Float<M, E> b) noexcept;
template <std::size_t M, std::size_t E>
T81Float<M, E> operator*(T81Float<M, E> a, T81Float<M, E> b) noexcept;
template <std::size_t M, std::size_t E>
T81Float<M, E> operator/(T81Float<M, E> a, T81Float<M, E> b) noexcept;
template <std::size_t M, std::size_t E>
T81Float<M, E> fma(T81Float<M, E> a, T81Float<M, E> b, T81Float<M, E> c) noexcept;

template <std::size_t M, std::size_t E>
class T81Float {
  friend class T81BigInt;
  static_assert(M >= 4, "T81Float: mantissa must be at least 4 trits");
  static_assert(E >= 4, "T81Float: exponent must be at least 4 trits");
  static_assert(M + E + 1 <= 2048, "T81Float: total trits must fit in T81Int");

public:
  using size_type = std::size_t;
  using Storage = T81Int<1 + E + M>;

  static constexpr size_type kMantissaTrits = M;
  static constexpr size_type kExponentTrits = E;

  // Correct balanced ternary bias: (3^E - 1) / 2
  static constexpr std::int64_t kExponentBias = []() constexpr {
    std::int64_t pow3 = 1;
    for (size_type i = 0; i < E; ++i) pow3 *= 3;
    return (pow3 - 1) / 2;
  }();

  // Maximum exponent value (all P trits) = (3^E - 1)/2
  static constexpr std::int64_t kInfExponent = kExponentBias;

private:
  // Layout in bits_:
  //   [0 .. M-1]      : mantissa
  //   [M .. M+E-1]    : exponent (balanced ternary, biased)
  //   [M+E]           : sign trit (P = +, N = -)
  Storage bits_{};

public:
  // ---------------------------------------------------------------------
  // Constructors
  // ---------------------------------------------------------------------

  constexpr T81Float() noexcept = default;

  // Convenience scalar constructors for higher-level APIs (Quaternion, Vector,
  // Time)
  explicit T81Float(int v) noexcept { *this = from_double(static_cast<double>(v)); }

  explicit T81Float(long v) noexcept { *this = from_double(static_cast<double>(v)); }

  explicit T81Float(long long v) noexcept { *this = from_double(static_cast<double>(v)); }

  explicit T81Float(float v) noexcept { *this = from_double(static_cast<double>(v)); }

  explicit T81Float(double v) noexcept { *this = from_double(v); }

  // Widening constructor: T81Float<OtherM,E> → T81Float<M,E> when OtherM <= M
  template <std::size_t OtherM>
    requires(OtherM <= M)
  constexpr T81Float(const T81Float<OtherM, E>& other) noexcept {
    *this = from_double(other.to_double());
  }

  // ---------------------------------------------------------------------
  // Factories
  // ---------------------------------------------------------------------

  static constexpr T81Float zero(bool positive = true) noexcept {
    T81Float f;
    // Canonical zero has exponent -Bias (encoded as 0) or potentially minimal exponent.
    // Spec: "Zero (exp = 0, mant = 0)". Here 0 means unbiased 0?
    // Or encoded 0? The code uses get_exp() which decodes.
    // If encoded exp is 0, then unbiased is 0.
    // But bias is (3^E-1)/2. So encoded 0 is unbiased 0.
    // If bias is applied, usually 0 encoded means -Bias.
    // Let's check get_exp(). It decodes balanced ternary integer directly.
    // So get_exp() returns the stored value.
    // The "biased exponent" in T81Float seems to be... just the stored exponent?
    // "Correct balanced-ternary biased exponent (bias = (3^E - 1)/2)"
    // "Value ≈ sign * mantissa * 3^(exp_unbiased - (M - 1))"
    // from_double uses: const std::int64_t exp_unb = k + 1;
    // And normalization seems to assume exp is passed directly.

    // To match to_canonical_string logic: "Exponent (decimal integer is canonical)" -> get_exp().
    // If we want +0E0, get_exp() must be 0.

    f.set_sign(positive);
    f.set_exp(0);
    f.set_mantissa(T81Int<M>{});
    return f;
  }

  static constexpr T81Float inf(bool positive = true) noexcept {
    T81Float f;
    f.set_sign(positive);
    f.set_exp(kInfExponent);
    f.set_mantissa(T81Int<M>{});
    return f;
  }

  static constexpr T81Float nae() noexcept {
    T81Float f = inf(true);
    f.set_mantissa(T81Int<M>(1));
    return f;
  }

  // ---------------------------------------------------------------------
  // Classification
  // ---------------------------------------------------------------------

  [[nodiscard]] constexpr bool is_zero() const noexcept {
    return get_exp() == 0 && get_mantissa().is_zero();
  }

  [[nodiscard]] constexpr bool is_inf() const noexcept {
    return get_exp() == kInfExponent && get_mantissa().is_zero();
  }

  [[nodiscard]] constexpr bool is_nae() const noexcept {
    return get_exp() == kInfExponent && !get_mantissa().is_zero();
  }

  [[nodiscard]] constexpr bool is_subnormal() const noexcept {
    return get_exp() == -kInfExponent && !get_mantissa().is_zero();
  }

  [[nodiscard]] constexpr bool is_negative() const noexcept { return get_sign() == Trit::N; }

  // ---------------------------------------------------------------------
  // Basic operations
  // ---------------------------------------------------------------------

  [[nodiscard]] constexpr T81Float operator-() const noexcept {
    T81Float f = *this;
    if (!f.is_zero()) {
      f.flip_sign();
    }
    return f;
  }

  [[nodiscard]] constexpr T81Float abs() const noexcept {
    T81Float f = *this;
    f.set_sign(true);
    return f;
  }

  // Bridge to/from double
  [[nodiscard]] double to_double() const noexcept;
  static T81Float from_double(double v) noexcept;

  [[nodiscard]] constexpr bool operator==(const T81Float& other) const noexcept {
    if (is_nae() || other.is_nae()) return false;
    if (is_zero() && other.is_zero()) return true;
    return bits_ == other.bits_;
  }

  [[nodiscard]] constexpr std::partial_ordering operator<=>(const T81Float& other) const noexcept {
    if (is_nae() || other.is_nae()) return std::partial_ordering::unordered;
    if (is_zero() && other.is_zero()) return std::partial_ordering::equivalent;

    const Trit s1 = get_sign();
    const Trit s2 = other.get_sign();

    if (s1 != s2) {
      // N < Z < P
      return (static_cast<int>(s1) < static_cast<int>(s2)) ? std::partial_ordering::less
                                                           : std::partial_ordering::greater;
    }

    // Same sign
    const auto e1 = get_exp();
    const auto e2 = other.get_exp();
    if (e1 != e2) {
      if (s1 == Trit::N) {
        return (e1 > e2) ? std::partial_ordering::less : std::partial_ordering::greater;
      }
      return (e1 < e2) ? std::partial_ordering::less : std::partial_ordering::greater;
    }

    const auto m1 = get_mantissa();
    const auto m2 = other.get_mantissa();
    const auto cmp = (m1 <=> m2);

    if (cmp == std::strong_ordering::equal) return std::partial_ordering::equivalent;

    const bool less = (cmp == std::strong_ordering::less);
    if (s1 == Trit::N) {
      return less ? std::partial_ordering::greater : std::partial_ordering::less;
    }
    return less ? std::partial_ordering::less : std::partial_ordering::greater;
  }

  // ---------------------------------------------------------------------
  // High-level math helpers for geometry/time layers
  // ---------------------------------------------------------------------

  // Deterministic implementations via dmath (Phase 1)
  // Now routing to t81_soft_math explicitly for Phase 2 determinism.

  [[nodiscard]] T81Float sin() const noexcept {
#if defined(T81_DETERMINISTIC)
    return core::math::t81_soft_math::t81_sin(*this);
#else
    if (is_nae()) return *this;
    return from_double(std::sin(to_double()));
#endif
  }

  [[nodiscard]] T81Float cos() const noexcept {
#if defined(T81_DETERMINISTIC)
    return core::math::t81_soft_math::t81_cos(*this);
#else
    if (is_nae()) return *this;
    return from_double(std::cos(to_double()));
#endif
  }

  [[nodiscard]] T81Float tan() const noexcept {
#if defined(T81_DETERMINISTIC)
    return core::math::t81_soft_math::t81_sin(*this) / core::math::t81_soft_math::t81_cos(*this);
#else
    if (is_nae()) return *this;
    return from_double(std::tan(to_double()));
#endif
  }

  [[nodiscard]] T81Float exp() const {
#if defined(T81_DETERMINISTIC)
    return core::math::t81_soft_math::t81_exp(*this);
#else
    if (is_nae()) return *this;
    return from_double(std::exp(to_double()));
#endif
  }

  [[nodiscard]] T81Float log() const {
#if defined(T81_DETERMINISTIC)
    return core::math::t81_soft_math::t81_log(*this);
#else
    if (is_nae()) return *this;
    return from_double(std::log(to_double()));
#endif
  }

  [[nodiscard]] T81Float sqrt() const {
#if defined(T81_DETERMINISTIC)
    return core::math::t81_soft_math::t81_sqrt(*this);
#else
    if (is_nae()) return *this;
    return from_double(std::sqrt(to_double()));
#endif
  }

  // Non-deterministic / Non-canonical functions (Phase 2 candidates)
  // Marked explicitly as relying on host math for now where dmath fallbacks
  // aren't ready/efficient. Note: acos/asin/atan could be derived from dmath
  // functions but require careful handling of domains. For Phase 1, we leave
  // them as host-dependent or marked. Actually, we can implement them via host
  // math but label them.

  /**
   * @brief Arc cosine of the value.
   * @warning NON-DETERMINISTIC: Relies on host platform math.
   * @return T81Float in the range [0, pi], or NaE if input is out of range or
   * NaE.
   */
  [[nodiscard]] T81Float acos() const noexcept {
#if defined(T81_DETERMINISTIC)
    // Deterministic path: acos not available in current dmath implementation
    // Explicitly reject rather than silently fall back to host math
    return nae();
#else
    if (is_nae()) return *this;
    double x = to_double();
    if (x < -1.0) x = -1.0;
    if (x > 1.0) x = 1.0;
    return from_double(std::acos(x));
#endif
  }

  /**
   * @brief Arc sine of the value.
   * @warning NON-DETERMINISTIC: Relies on host platform math unless T81_DETERMINISTIC is defined.
   */
  [[nodiscard]] T81Float asin() const noexcept {
#if defined(T81_DETERMINISTIC)
    // Deterministic path: asin not available in current dmath implementation
    // Explicitly reject rather than silently fall back to host math
    return nae();
#else
    if (is_nae()) return *this;
    double x = to_double();
    if (x < -1.0 || x > 1.0) return nae();
    return from_double(std::asin(x));
#endif
  }

  /**
   * @brief Arc tangent of the value.
   * @warning NON-DETERMINISTIC: Relies on host platform math unless T81_DETERMINISTIC is defined.
   */
  [[nodiscard]] T81Float atan() const noexcept {
#if defined(T81_DETERMINISTIC)
    // Deterministic path: atan not available in current dmath implementation
    // Explicitly reject rather than silently fall back to host math
    return nae();
#else
    if (is_nae()) return *this;
    return from_double(std::atan(to_double()));
#endif
  }

  /**
   * @brief Hyperbolic sine.
   * @warning NON-DETERMINISTIC: Relies on host platform math unless T81_DETERMINISTIC is defined.
   */
  [[nodiscard]] T81Float sinh() const noexcept {
#if defined(T81_DETERMINISTIC)
    // Deterministic path: sinh not available in current dmath implementation
    // Explicitly reject rather than silently fall back to host math
    return nae();
#else
    if (is_nae()) return *this;
    return from_double(std::sinh(to_double()));
#endif
  }

  /**
   * @brief Hyperbolic cosine.
   * @warning NON-DETERMINISTIC: Relies on host platform math unless T81_DETERMINISTIC is defined.
   */
  [[nodiscard]] T81Float cosh() const noexcept {
#if defined(T81_DETERMINISTIC)
    // Deterministic path: cosh not available in current dmath implementation
    // Explicitly reject rather than silently fall back to host math
    return nae();
#else
    if (is_nae()) return *this;
    return from_double(std::cosh(to_double()));
#endif
  }

  /**
   * @brief Hyperbolic tangent.
   * @warning NON-DETERMINISTIC: Relies on host platform math unless T81_DETERMINISTIC is defined.
   */
  [[nodiscard]] T81Float tanh() const noexcept {
#if defined(T81_DETERMINISTIC)
    // Deterministic path: tanh not available in current dmath implementation
    // Explicitly reject rather than silently fall back to host math
    return nae();
#else
    if (is_nae()) return *this;
    return from_double(std::tanh(to_double()));
#endif
  }

  /**
   * @brief Power function x^exponent.
   * @warning NON-DETERMINISTIC: Relies on host platform math unless T81_DETERMINISTIC is defined.
   */
  [[nodiscard]] T81Float pow(T81Float exponent) const noexcept {
#if defined(T81_DETERMINISTIC)
    // In deterministic mode, use the deterministic math implementation
    return t81::core::math::t81_soft_math::t81_pow(*this, exponent);
#else
    if (is_nae() || exponent.is_nae()) return nae();
    return from_double(std::pow(to_double(), exponent.to_double()));
#endif
  }

  // ---------------------------------------------------------------------
  // Rounding
  // ---------------------------------------------------------------------

  [[nodiscard]] T81Float floor() const noexcept {
    if (is_nae()) return *this;
    return from_double(std::floor(to_double()));
  }

  [[nodiscard]] T81Float ceil() const noexcept {
    if (is_nae()) return *this;
    return from_double(std::ceil(to_double()));
  }

  [[nodiscard]] T81Float round() const noexcept {
    if (is_nae()) return *this;
    return from_double(std::round(to_double()));
  }

  [[nodiscard]] T81Float clamp(T81Float min, T81Float max) const noexcept {
    if (is_nae() || min.is_nae() || max.is_nae()) return nae();
    if (*this < min) return min;
    if (*this > max) return max;
    return *this;
  }

  // ---------------------------------------------------------------------
  // Debugging
  // ---------------------------------------------------------------------

  struct DebugFields {
    Trit sign;
    std::int64_t biased_exp;
    T81Int<M> mantissa;
  };

  [[nodiscard]] DebugFields debug_get_fields() const noexcept {
    return {get_sign(), get_exp(), get_mantissa()};
  }

  // ---------------------------------------------------------------------
  // Public Components Access & Factory
  // ---------------------------------------------------------------------

  // P2: Canonical serialization (no host float dependency)
  [[nodiscard]] std::string to_canonical_string() const {
    if (is_nae()) return "NaE";
    if (is_inf()) return is_negative() ? "-Inf" : "+Inf";

    // Zero Handling:
    // T81Float::zero() sets exp=0, mant=0.
    // However, is_zero() checks mant==0 and exp==0.
    // We want "+0E0" or "-0E0".
    if (is_zero()) return is_negative() ? "-0E0" : "+0E0";

    std::string s;
    // Sign
    s.push_back(get_sign() == Trit::N ? '-' : '+');

    // Mantissa trits
    s += get_mantissa().to_canonical_string();

    s.push_back('E');

    // Exponent (decimal integer is canonical)
    s += std::to_string(get_exp());

    return s;
  }

  [[nodiscard]] constexpr Trit sign_trit() const noexcept { return get_sign(); }
  [[nodiscard]] constexpr std::int64_t exponent() const noexcept { return get_exp(); }
  [[nodiscard]] constexpr T81Int<M> mantissa() const noexcept { return get_mantissa(); }

  static T81Float from_components(Trit sign, std::int64_t exp, const T81Int<M>& mant) noexcept {
    T81Float f;
    f.set_sign(sign != Trit::N);
    f.set_exp(exp);
    f.set_mantissa(mant);
    return f;
  }

  // ---------------------------------------------------------------------
  // Native Arithmetic (Deterministic)
  // ---------------------------------------------------------------------

  static T81Float add(T81Float a, T81Float b) noexcept {
    if (a.is_nae() || b.is_nae()) return nae();
    if (a.is_inf() || b.is_inf()) {
      if (a.is_inf() && b.is_inf() && a.get_sign() != b.get_sign()) return nae();
      return a.is_inf() ? a : b;
    }
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    if (a.get_exp() < b.get_exp()) {
      T81Float t = a;
      a = b;
      b = t;
    }

    std::int64_t ea = a.get_exp();
    std::int64_t eb = b.get_exp();
    std::int64_t diff = ea - eb;

    constexpr size_t WideN = 2 * M + 4;
    static_assert(WideN <= 2048, "M too large for native arithmetic");
    using Wide = T81Int<WideN>;

    Wide wa(a.get_mantissa());
    Wide wb(b.get_mantissa());

    // Align wa to have guard bits
    wa <<= M;

    if (diff > static_cast<std::int64_t>(M + 2)) {
      wb = Wide(0);
    } else {
      std::int64_t shift = static_cast<std::int64_t>(M) - diff;
      if (shift >= 0)
        wb <<= static_cast<size_t>(shift);
      else
        wb >>= static_cast<size_t>(-shift);
    }

    if (a.is_negative()) wa = -wa;
    if (b.is_negative()) wb = -wb;

    Wide sum = wa + wb;

    Trit s = sum.sign_trit();
    if (s == Trit::Z) return zero();

    if (s == Trit::N) sum = -sum;

    return normalize<WideN - M>(s == Trit::P ? Trit::P : Trit::N, ea - M, sum);
  }

  static T81Float mul(T81Float a, T81Float b) noexcept {
    if (a.is_nae() || b.is_nae()) return nae();
    if (a.is_inf() || b.is_inf()) {
      if (a.is_zero() || b.is_zero()) return nae();
      return inf(a.get_sign() == b.get_sign());
    }
    if (a.is_zero() || b.is_zero()) return zero();

    constexpr size_t WideN = 2 * M;
    static_assert(WideN <= 2048, "M too large");
    using Wide = T81Int<WideN>;

    Wide wa(a.get_mantissa());
    Wide wb(b.get_mantissa());

    Wide prod = wa * wb;
    // Value is prod * 3^(ea - (M-1) + eb - (M-1))
    // = prod * 3^(ea + eb - 2M + 2)
    // normalize expects exp such that value = prod * 3^exp
    // So passed exp = ea + eb - M + 1?
    // Wait, normalize produces mant * 3^(out_exp - (M-1)).
    // We want mant * 3^(out_exp - M + 1) == prod * 3^(ea + eb - 2M + 2).
    // If normalize preserves prod * 3^exp_in, then exp_in should be ea + eb -
    // 2M + 2. But normalize produces T81Float. normalize invariant: result
    // value == mant * 3^exp. So we must pass exp = ea + eb - 2*M + 2. Wait,
    // normalize interprets input as m * 3^(exp - (M-1)). We want m_prod * 3^(ea
    // + eb - 2M + 2). m_prod * 3^(E - M + 1) = m_prod * 3^(ea + eb - 2M + 2).
    // E - M + 1 = ea + eb - 2M + 2 + M - 1 = ea + eb - M + 1.

    std::int64_t exp = a.get_exp() + b.get_exp() - static_cast<std::int64_t>(M) + 1;
    bool pos = (a.get_sign() == b.get_sign());

    // normalize expects T81Int<M + Guard>
    // Here M + Guard = 2M, so Guard = M.
    return normalize<M>(pos ? Trit::P : Trit::N, exp, prod);
  }

  static T81Float div(T81Float a, T81Float b) noexcept {
    if (a.is_nae() || b.is_nae()) return nae();
    if (b.is_zero()) {
      if (a.is_zero() || a.is_inf()) return nae();
      return inf(a.get_sign() == b.get_sign());
    }
    if (a.is_inf()) {
      if (b.is_inf()) return nae();
      return inf(a.get_sign() == b.get_sign());
    }
    if (b.is_inf()) return zero();
    if (a.is_zero()) return zero();

#if defined(T81_DETERMINISTIC)
    // In deterministic mode, use the deterministic math implementation
    return t81::core::math::t81_soft_math::t81_div(a, b);
#else
    // Non-deterministic mode: fall back to double conversion
    return from_double(a.to_double() / b.to_double());
#endif
  }

  // Arithmetic friends
  template <std::size_t MM, std::size_t EE>
  friend T81Float<MM, EE> operator+(T81Float<MM, EE> a, T81Float<MM, EE> b) noexcept;
  template <std::size_t MM, std::size_t EE>
  friend T81Float<MM, EE> operator-(T81Float<MM, EE> a, T81Float<MM, EE> b) noexcept;
  template <std::size_t MM, std::size_t EE>
  friend T81Float<MM, EE> operator*(T81Float<MM, EE> a, T81Float<MM, EE> b) noexcept;
  template <std::size_t MM, std::size_t EE>
  friend T81Float<MM, EE> operator/(T81Float<MM, EE> a, T81Float<MM, EE> b) noexcept;
  template <std::size_t MM, std::size_t EE>
  friend T81Float<MM, EE> fma(T81Float<MM, EE> a, T81Float<MM, EE> b, T81Float<MM, EE> c) noexcept;

  // --- Binary Serialization ---
  void serialize(std::ostream& os) const { bits_.serialize(os); }

  void deserialize(std::istream& is) { bits_.deserialize(is); }

private:
  // ---------------------------------------------------------------------
  // Raw field accessors
  // ---------------------------------------------------------------------

  [[nodiscard]] constexpr Trit get_sign() const noexcept { return bits_.operator[](M + E); }

  constexpr void set_sign(bool positive) noexcept { bits_[M + E] = positive ? Trit::P : Trit::N; }

  constexpr void flip_sign() noexcept {
    bits_[M + E] = (get_sign() == Trit::P ? Trit::N : Trit::P);
  }

  [[nodiscard]] constexpr std::int64_t get_exp() const noexcept {
    std::int64_t e = 0;
    std::int64_t p = 1;
    for (size_type i = 0; i < E; ++i) {
      e += static_cast<std::int64_t>(trit_to_int(bits_[M + i])) * p;
      p *= 3;
    }
    return e;
  }

  constexpr void set_exp(std::int64_t e) noexcept {
    for (size_type i = 0; i < E; ++i) {
      int digit = static_cast<int>(e % 3);
      e /= 3;
      if (digit > 1) {
        digit -= 3;
        ++e;
      }
      if (digit < -1) {
        digit += 3;
        --e;
      }
      bits_[M + i] = int_to_trit(digit);
    }
  }

  [[nodiscard]] constexpr T81Int<M> get_mantissa() const noexcept {
    T81Int<M> m;
    for (size_type i = 0; i < M; ++i) {
      m[i] = bits_[i];
    }
    return m;
  }

  constexpr void set_mantissa(const T81Int<M>& m) noexcept {
    for (size_type i = 0; i < M; ++i) {
      bits_[i] = m[i];
    }
  }

  // Leading trit position (MSNZ index or max if all zero)
  template <std::size_t K>
  [[nodiscard]] static constexpr size_type leading_trit(const T81Int<K>& x) noexcept {
    for (size_type i = K; i-- > 0;) {
      if (x[i] != Trit::Z) {
        return i;
      }
    }
    return std::numeric_limits<size_type>::max();
  }

  // Normalization helper used by future pure-ternary arithmetic
  template <std::size_t Guard = 4>
  static constexpr T81Float normalize(Trit sign, std::int64_t exp,
                                      T81Int<M + Guard> mant) noexcept {
    if (mant.is_zero()) {
      return zero(sign == Trit::P);
    }
    const size_type lead = leading_trit(mant);
    if (lead == std::numeric_limits<size_type>::max()) {
      return zero(sign == Trit::P);
    }

    // Target index is M-1 (MSB of M trits)
    const std::int64_t shift = static_cast<std::int64_t>(lead) - static_cast<std::int64_t>(M - 1);

    // Adjust exponent
    exp += shift;

    // We copy the relevant trits to final_m.
    // Ideally we'd do rounding here (balanced ternary truncation is
    //   effectively rounding to
    // nearest, except for tie-breaking on .5). For now, we perform simple
    // truncation (copying) to fix the immediate bug. Note: To support proper
    // rounding of 0.5 cases, we'd need to inspect mant[lead - M] etc.

    T81Int<M> final_m;
    if (shift >= 0) {
      const size_type s = static_cast<size_type>(shift);
      for (size_type i = 0; i < M; ++i) {
        if (i + s < M + Guard) {
          final_m[i] = mant[i + s];
        } else {
          final_m[i] = Trit::Z;
        }
      }
    } else {
      const size_type s = static_cast<size_type>(-shift);
      for (size_type i = 0; i < M; ++i) {
        if (i >= s && i - s < M + Guard) {
          final_m[i] = mant[i - s];
        } else {
          final_m[i] = Trit::Z;
        }
      }
    }

    // Overflow/underflow handling
    if (exp >= kInfExponent) {
      return inf(sign == Trit::P);
    }
    if (exp <= -kInfExponent) {
      const std::int64_t under = -kInfExponent - exp;
      if (under >= static_cast<std::int64_t>(M)) {
        return zero(sign == Trit::P);
      }
      final_m >>= static_cast<size_type>(under);
      exp = -kInfExponent;
    }

    T81Float f;
    f.set_sign(sign == Trit::P);
    f.set_exp(exp);
    f.set_mantissa(final_m);
    return f;
  }
};

// ======================================================================
// Arithmetic operators (double-backed, NaE-aware)
// ======================================================================

template <std::size_t M, std::size_t E>
T81Float<M, E> operator+(T81Float<M, E> a, T81Float<M, E> b) noexcept {
  return T81Float<M, E>::add(a, b);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator-(T81Float<M, E> a, T81Float<M, E> b) noexcept {
  return T81Float<M, E>::add(a, -b);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator*(T81Float<M, E> a, T81Float<M, E> b) noexcept {
  return T81Float<M, E>::mul(a, b);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator/(T81Float<M, E> a, T81Float<M, E> b) noexcept {
  return T81Float<M, E>::div(a, b);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> fma(T81Float<M, E> a, T81Float<M, E> b, T81Float<M, E> c) noexcept {
  return T81Float<M, E>::add(T81Float<M, E>::mul(a, b), c);
}

// ======================================================================
// Double conversion (base-3, symmetric mapping)
// value ≈ sign * mantissa * 3^(exp_unbiased - (M - 1))
// ======================================================================

template <std::size_t M, std::size_t E>
T81Float<M, E> T81Float<M, E>::from_double(double v) noexcept {
  using F = T81Float<M, E>;
  using size_type = typename F::size_type;

  // Special cases
  if (v == 0.0) {
    return F::zero();
  }
  if (std::isinf(v)) {
    return F::inf(v > 0.0);
  }
  if (std::isnan(v)) {
    return F::nae();
  }

  const bool neg = (v < 0.0);
  long double mag = fabsl(static_cast<long double>(v));

  // log_3(|v|)
  static const long double kLn3 = std::log(3.0L);
  const long double log3_v = std::log(mag) / kLn3;

  // Choose exponent so that mantissa is roughly in [3^(M-2), 3^(M-1))
  const std::int64_t k = static_cast<std::int64_t>(std::floor(log3_v));
  const std::int64_t exp_unb = k + 1;

  // We can only safely extract into int64_t if we limit the trits.
  // 63 bits ~ 39.7 trits. Let's start with safe limit 39.
  constexpr size_type kSafeTrits = 39;
  constexpr size_type EffectiveM = (M > kSafeTrits) ? kSafeTrits : M;
  constexpr size_type Shift = M - EffectiveM;

  const long double scale_exp = static_cast<long double>(EffectiveM - 1 - exp_unb);
  const long double mant_real = mag * powl(3.0L, scale_exp);  // ≈ |v| * 3^(EffectiveM-1-exp_unb)

  if (!std::isfinite(mant_real) || mant_real == 0.0L) {
    return F::zero();
  }

  // Integer mantissa magnitude
  const long long mant_ll = static_cast<long long>(std::llround(mant_real));

  // Encode mant_ll >= 0 into balanced ternary
  T81Int<M> mantissa;  // zero by default
  std::int64_t tmp = mant_ll;

  // Fill upper trits [Shift ... M-1]
  for (size_type i = 0; i < EffectiveM && tmp != 0; ++i) {
    int digit = static_cast<int>(tmp % 3);
    tmp /= 3;

    size_type idx = i + Shift;
    if (digit == 2) {
      mantissa[idx] = Trit::N;  // 2 → -1 with carry
      ++tmp;
    } else {
      mantissa[idx] = int_to_trit(digit);  // 0 or 1
    }
  }

  // Use normalize to ensure canonical representation (MSB at M-1)
  // We extend to Guard size to allow normalize to handle it
  // But here we constructed mantissa to be within M trits (MSB at M-2 usually).
  // So a simple normalize call should shift it left by 1 if needed.
  using Wide = T81Int<M + 4>;
  // Explicitly initialize to zero to ensure upper trits are clean (avoid
  // garbage).
  Wide w(0);
  for (size_type i = 0; i < M; ++i) w[i] = mantissa[i];

  return normalize<4>(!neg ? Trit::P : Trit::N, exp_unb, w);
}

template <std::size_t M, std::size_t E>
double T81Float<M, E>::to_double() const noexcept {
  using F = T81Float<M, E>;
  using size_type = typename F::size_type;

  if (is_zero()) {
    return 0.0;
  }
  if (is_inf()) {
    return is_negative() ? -std::numeric_limits<double>::infinity()
                         : std::numeric_limits<double>::infinity();
  }
  if (is_nae()) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const std::int64_t exp_unb = get_exp();
  const T81Int<M> m = get_mantissa();

  long double mant_val = 0.0L;
  long double p = 1.0L;

  for (size_type i = 0; i < M; ++i) {
    mant_val += static_cast<long double>(trit_to_int(m[i])) * p;
    p *= 3.0L;
  }

  if (mant_val == 0.0L) {
    return 0.0;
  }

  const long double pow_factor =
      powl(3.0L, static_cast<long double>(exp_unb) - static_cast<long double>(M - 1));

  long double mag_ld = mant_val * pow_factor;
  if (!std::isfinite(mag_ld)) {
    return is_negative() ? -std::numeric_limits<double>::infinity()
                         : std::numeric_limits<double>::infinity();
  }

  double result = static_cast<double>(mag_ld);
  if (is_negative()) {
    result = -result;
  }
  return result;
}

// Common typedefs for canonical sizes
using T81Float18_9 = T81Float<18, 9>;
using T81Float27_9 = T81Float<27, 9>;
using T81Float72_9 = T81Float<72, 9>;  // default for Vec3f, etc.

}  // namespace t81::v1

namespace t81 {
using v1::T81Float;
using v1::T81Float18_9;
using v1::T81Float27_9;
using v1::T81Float72_9;
}  // namespace t81
