/**
 * @file T81Float.hpp
 * @brief Balanced ternary floating-point backed by T81Int storage.
 *
 * Model:
 *   • Storage: T81Int<1 + E + M> (sign + exponent + mantissa trits)
 *   • IEEE-like layout with a binary-style biased exponent:
 *       - Sign:  1 trit (P = +, N = −)
 *       - Exponent: E trits (stored as balanced integer, but using a 2^E-style bias)
 *       - Mantissa: M trits
 *   • Special values:
 *       - Zero     (exp = 0, mant = 0)
 *       - Subnormal (exp = 0, mant != 0)
 *       - Infinity (exp = max, mant = 0)
 *       - NaE      (Not-an-Entity, exp = max, mant != 0)
 *
 * Arithmetic operators currently delegate to double for correctness and
 * simplicity, while preserving special-value semantics.
 */

#pragma once

#include "t81/core/T81Int.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <compare>

namespace t81::core {

template <std::size_t M, std::size_t E>
class T81Float;

// Forward declarations for arithmetic
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
    static_assert(M >= 4, "T81Float: mantissa must be at least 4 trits");
    static_assert(E >= 4, "T81Float: exponent must be at least 4 trits");
    static_assert(M + E + 1 <= 2048,
                  "T81Float: total (1 + E + M) trits must fit in T81Int");

public:
    using size_type = std::size_t;
    using Storage   = T81Int<1 + E + M>;

    static constexpr size_type kMantissaTrits = M;
    static constexpr size_type kExponentTrits = E;

    // Binary-style bias used when mapping to/from IEEE doubles.
    static constexpr std::int64_t kExponentBias =
        (std::int64_t(1) << (E - 1)) - 1;

private:
    Storage bits_{}; // [0..M-1] mantissa, [M..M+E-1] exponent, [M+E] sign

    static constexpr std::int64_t kInfExponent =
        (std::int64_t(1) << E) - 1;

public:
    constexpr T81Float() noexcept = default;

    // Factories
    static constexpr T81Float zero(bool positive = true) noexcept {
        T81Float f;
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
        return get_exp() == 0 && !get_mantissa().is_zero();
    }

    [[nodiscard]] constexpr bool is_negative() const noexcept {
        return get_sign() == Trit::N;
    }

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

    [[nodiscard]] double to_double() const noexcept;
    static T81Float from_double(double v) noexcept;

    // Bitwise comparison (note: NaE is not IEEE-754 NaN; this is raw-encoding order)
    [[nodiscard]] constexpr auto operator<=>(const T81Float&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Float&) const noexcept = default;

    // Arithmetic friends
    template <std::size_t MM, std::size_t EE>
    friend T81Float<MM, EE> operator+(T81Float<MM, EE> a,
                                      T81Float<MM, EE> b) noexcept;

    template <std::size_t MM, std::size_t EE>
    friend T81Float<MM, EE> operator-(T81Float<MM, EE> a,
                                      T81Float<MM, EE> b) noexcept;

    template <std::size_t MM, std::size_t EE>
    friend T81Float<MM, EE> operator*(T81Float<MM, EE> a,
                                      T81Float<MM, EE> b) noexcept;

    template <std::size_t MM, std::size_t EE>
    friend T81Float<MM, EE> operator/(T81Float<MM, EE> a,
                                      T81Float<MM, EE> b) noexcept;

    template <std::size_t MM, std::size_t EE>
    friend T81Float<MM, EE> fma(T81Float<MM, EE> a,
                                T81Float<MM, EE> b,
                                T81Float<MM, EE> c) noexcept;

private:
    // --- Raw field accessors ---

    [[nodiscard]] constexpr Trit get_sign() const noexcept {
        return bits_.get_trit(M + E);
    }

    constexpr void set_sign(bool positive) noexcept {
        bits_.set_trit(M + E, positive ? Trit::P : Trit::N);
    }

    constexpr void flip_sign() noexcept {
        bits_.set_trit(M + E, get_sign() == Trit::P ? Trit::N : Trit::P);
    }

    [[nodiscard]] constexpr std::int64_t get_exp() const noexcept {
        T81Int<E> e;
        for (size_type i = 0; i < E; ++i) {
            e[i] = bits_.get_trit(M + i);
        }
        return e.to_int64();
    }

    constexpr void set_exp(std::int64_t e) noexcept {
        T81Int<E> exp(e);
        for (size_type i = 0; i < E; ++i) {
            bits_.set_trit(M + i, exp[i]);
        }
    }

    [[nodiscard]] constexpr T81Int<M> get_mantissa() const noexcept {
        T81Int<M> m;
        for (size_type i = 0; i < M; ++i) {
            m[i] = bits_.get_trit(i);
        }
        return m;
    }

    constexpr void set_mantissa(const T81Int<M>& m) noexcept {
        for (size_type i = 0; i < M; ++i) {
            bits_.set_trit(i, m[i]);
        }
    }

    // Index of most-significant non-zero trit, or max() if all zero.
    template <std::size_t K>
    [[nodiscard]] static constexpr size_type leading_trit(const T81Int<K>& x) noexcept {
        for (size_type i = K; i-- > 0;) {
            if (x[i] != Trit::Z) {
                return i;
            }
        }
        return std::numeric_limits<size_type>::max();
    }

    // Normalize mantissa+exponent with Guard extra trits of precision.
    template <std::size_t Guard = 4>
    static T81Float normalize(Trit sign, std::int64_t exp,
                              T81Int<M + Guard> mant) noexcept {
        if (mant.is_zero()) {
            return zero(sign == Trit::P);
        }

        const size_type lead = leading_trit(mant);
        if (lead == std::numeric_limits<size_type>::max()) {
            return zero(sign == Trit::P);
        }

        const std::int64_t shift =
            static_cast<std::int64_t>(lead) - static_cast<std::int64_t>(M);
        exp -= shift;

        if (shift > 0) {
            mant >>= static_cast<size_type>(shift);
        } else if (shift < 0) {
            mant <<= static_cast<size_type>(-shift);
        }

        // Guard-+-sticky rounding (simple nearest-like rule)
        bool round_up = false;
        if constexpr (Guard >= 1) {
            const Trit guard_trit = mant.get_trit(M);
            bool sticky = false;

            if constexpr (Guard > 1) {
                for (size_type i = M + 1; i < M + Guard; ++i) {
                    if (mant.get_trit(i) != Trit::Z) {
                        sticky = true;
                        break;
                    }
                }
            }

            if (guard_trit == Trit::P ||
                (guard_trit == Trit::Z && sticky)) {
                round_up = true;
            }
        }

        T81Int<M> final_m;
        for (size_type i = 0; i < M; ++i) {
            final_m[i] = mant.get_trit(i);
        }

        if (round_up) {
            final_m = final_m + T81Int<M>(1);
        }

        // Crude overflow handling: if we just filled the top trit,
        // push one trit into the exponent.
        const size_type lead_final = leading_trit(final_m);
        if (lead_final == M - 1) {
            final_m >>= 1;
            ++exp;
        }

        // Map exponent to special / subnormal / normal ranges.
        if (exp >= kInfExponent) {
            return inf(sign == Trit::P);
        }

        if (exp <= 0) {
            const std::int64_t under = 1 - exp;
            if (under >= static_cast<std::int64_t>(M + Guard)) {
                return zero(sign == Trit::P);
            }
            final_m >>= static_cast<size_type>(under);
            exp = 0;
        }

        T81Float f;
        f.set_sign(sign == Trit::P);
        f.set_exp(exp);
        f.set_mantissa(final_m);
        return f;
    }
};

// ======================================================================
// Arithmetic operators (double-backed, with special-value handling)
// ======================================================================

template <std::size_t M, std::size_t E>
T81Float<M, E> operator+(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    using F = T81Float<M, E>;

    if (a.is_nae() || b.is_nae()) {
        return F::nae();
    }

    if (a.is_inf() || b.is_inf()) {
        if (a.is_inf() && b.is_inf()) {
            if (a.is_negative() != b.is_negative()) {
                // +inf + -inf → NaE
                return F::nae();
            }
            return a;
        }
        return a.is_inf() ? a : b;
    }

    const double da = a.to_double();
    const double db = b.to_double();
    return F::from_double(da + db);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator-(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    return a + (-b);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator*(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    using F = T81Float<M, E>;

    if (a.is_nae() || b.is_nae()) {
        return F::nae();
    }

    const bool a_zero = a.is_zero();
    const bool b_zero = b.is_zero();
    const bool a_inf  = a.is_inf();
    const bool b_inf  = b.is_inf();

    if ((a_inf && b_zero) || (b_inf && a_zero)) {
        // 0 * inf → NaE
        return F::nae();
    }

    if (a_inf || b_inf) {
        const bool neg = a.is_negative() ^ b.is_negative();
        return F::inf(!neg);
    }

    if (a_zero || b_zero) {
        return F::zero();
    }

    const double da = a.to_double();
    const double db = b.to_double();
    return F::from_double(da * db);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator/(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    using F = T81Float<M, E>;

    if (a.is_nae() || b.is_nae()) {
        return F::nae();
    }

    const bool a_zero = a.is_zero();
    const bool b_zero = b.is_zero();
    const bool a_inf  = a.is_inf();
    const bool b_inf  = b.is_inf();

    if (b_zero) {
        if (a_zero || a_inf) {
            // 0/0 or inf/0 → NaE
            return F::nae();
        }
        const bool neg = a.is_negative() ^ b.is_negative();
        return F::inf(!neg);
    }

    if (a_inf) {
        if (b_inf) {
            // inf / inf → NaE
            return F::nae();
        }
        const bool neg = a.is_negative() ^ b.is_negative();
        return F::inf(!neg);
    }

    if (b_inf) {
        // finite / inf → +0 or -0 (we just return +0)
        return F::zero();
    }

    if (a_zero) {
        return F::zero();
    }

    const double da = a.to_double();
    const double db = b.to_double();
    return F::from_double(da / db);
}

template <std::size_t M, std::size_t E>
T81Float<M, E> fma(T81Float<M, E> a,
                   T81Float<M, E> b,
                   T81Float<M, E> c) noexcept {
    return a * b + c;
}

// ======================================================================
// Conversions: double ↔ T81Float
// ======================================================================

template <std::size_t M, std::size_t E>
T81Float<M, E> T81Float<M, E>::from_double(double v) noexcept {
    using F  = T81Float<M, E>;
    using iT = T81Int<M + 10>;

    if (v == 0.0) {
        return F::zero();
    }
    if (std::isinf(v)) {
        return F::inf(v > 0.0);
    }
    if (std::isnan(v)) {
        return F::nae();
    }

    const bool neg = v < 0.0;
    double mag     = std::fabs(v);

    int bin_exp = 0;
    const double frac = std::frexp(mag, &bin_exp); // mag = frac * 2^bin_exp, frac ∈ [0.5,1)

    std::int64_t scale = 1;
    for (std::size_t i = 0; i < M; ++i) {
        scale *= 3;
    }

    const double scaled   = frac * static_cast<double>(scale) * 1.5;
    const std::int64_t mi = static_cast<std::int64_t>(scaled + 0.5);

    iT mant(mi);
    const size_type lead = leading_trit(mant);
    if (lead == std::numeric_limits<size_type>::max()) {
        return F::zero();
    }

    const std::int64_t shift =
        static_cast<std::int64_t>(lead) - static_cast<std::int64_t>(M);
    const std::int64_t unbiased =
        static_cast<std::int64_t>(bin_exp) + shift - 1;
    const std::int64_t biased = unbiased + kExponentBias;

    return normalize<10>(neg ? Trit::N : Trit::P, biased, mant);
}

template <std::size_t M, std::size_t E>
double T81Float<M, E>::to_double() const noexcept {
    using std::numeric_limits;

    if (is_zero()) {
        return 0.0;
    }
    if (is_inf()) {
        return is_negative()
                   ? -numeric_limits<double>::infinity()
                   : numeric_limits<double>::infinity();
    }
    if (is_nae()) {
        return numeric_limits<double>::quiet_NaN();
    }

    std::int64_t exp = get_exp();

    // Rebuild an extended mantissa with an implicit leading trit when exp > 0.
    T81Int<M + 4> mant_ext;
    const T81Int<M> m = get_mantissa();
    for (size_type i = 0; i < M; ++i) {
        mant_ext[i] = m[i];
    }
    if (exp > 0) {
        mant_ext.set_trit(M, Trit::P);
    }

    double value = 0.0;
    double p     = 1.0;
    for (size_type i = 0; i < M + 4; ++i) {
        const Trit t = mant_ext.get_trit(i);
        if (t == Trit::P) {
            value += p;
        } else if (t == Trit::N) {
            value -= p;
        }
        p *= 3.0;
    }

    value = std::ldexp(value, static_cast<int>(exp - kExponentBias));
    return is_negative() ? -value : value;
}

// ======================================================================
// Common typedefs
// ======================================================================

using T81Float18_9 = T81Float<18, 9>;
using T81Float27_9 = T81Float<27, 9>;

} // namespace t81::core
