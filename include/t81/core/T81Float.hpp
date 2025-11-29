/**
 * @file T81Float.hpp
 * @brief Balanced ternary floating-point backed by T81Int storage.
 *
 * Model:
 * • Storage: T81Int<1 + E + M> (sign + exponent + mantissa trits)
 * • Correct balanced-ternary biased exponent (bias = (3^E - 1)/2)
 * • Special values:
 *   - Zero (exp = 0, mant = 0)
 *   - Subnormal (exp = 0, mant != 0)
 *   - Infinity (exp = all P trits, mant = 0)
 *   - NaE (Not-an-Entity, exp = all P trits, mant != 0)
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

// Forward declarations
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
    static_assert(M + E + 1 <= 2048, "T81Float: total trits must fit in T81Int");

public:
    using size_type = std::size_t;
    using Storage = T81Int<1 + E + M>;

    static constexpr size_type kMantissaTrits = M;
    static constexpr size_type kExponentTrits = E;

    // Correct balanced ternary bias: (3^E - 1)/2
    static constexpr std::int64_t kExponentBias = []() constexpr {
        std::int64_t pow3 = 1;
        for (size_type i = 0; i < E; ++i) pow3 *= 3;
        return (pow3 - 1) / 2;
    }();

    // Maximum exponent value (all P trits) = (3^E - 1)/2
    static constexpr std::int64_t kInfExponent = kExponentBias;

private:
    Storage bits_{}; // [0..M-1] mantissa, [M..M+E-1] exponent, [M+E] sign

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

    [[nodiscard]] constexpr auto operator<=>(const T81Float&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Float&) const noexcept = default;

    // Arithmetic friends (unchanged)
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

private:
    // --- Raw field accessors (now constexpr and safe) ---
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
        std::int64_t e = 0;
        std::int64_t p = 1;
        for (size_type i = 0; i < E; ++i) {
            e += trit_to_int(bits_.get_trit(M + i)) * p;
            p *= 3;
        }
        return e;
    }

    constexpr void set_exp(std::int64_t e) noexcept {
        for (size_type i = 0; i < E; ++i) {
            const int digit = static_cast<int>(e % 3);
            e /= 3;
            Trit t = int_to_trit((digit == 2) ? -1 : digit);
            if (digit == 2) ++e;  // carry adjustment
            bits_.set_trit(M + i, t);
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

    // Leading trit position
    template <std::size_t K>
    [[nodiscard]] static constexpr size_type leading_trit(const T81Int<K>& x) noexcept {
        for (size_type i = K; i-- > 0;) {
            if (x[i] != Trit::Z) {
                return i;
            }
        }
        return std::numeric_limits<size_type>::max();
    }

    // Your original normalize — unchanged except for correct shift semantics
    template <std::size_t Guard = 4>
    static constexpr T81Float normalize(Trit sign, std::int64_t exp, T81Int<M + Guard> mant) noexcept {
        if (mant.is_zero()) {
            return zero(sign == Trit::P);
        }
        const size_type lead = leading_trit(mant);
        if (lead == std::numeric_limits<size_type>::max()) {
            return zero(sign == Trit::P);
        }
        const std::int64_t shift = static_cast<std::int64_t>(lead) - static_cast<std::int64_t>(M);
        exp -= shift;

        // Manual trit shifts (no <<=/>>= on temporary)
        if (shift > 0) {
            for (size_type i = 0; i < M; ++i) {
                mant[i] = (i + shift < M + Guard) ? mant[i + shift] : Trit::Z;
            }
            for (size_type i = M; i < M + Guard; ++i) mant[i] = Trit::Z;
        } else if (shift < 0) {
            const size_type ashift = static_cast<size_type>(-shift);
            for (size_type i = M + Guard; i-- > ashift;) {
                mant[i] = mant[i - ashift];
            }
            for (size_type i = 0; i < ashift; ++i) mant[i] = Trit::Z;
        }

        // Guard + sticky rounding
        bool round_up = false;
        if constexpr (Guard >= 1) {
            const Trit guard = mant.get_trit(M);
            bool sticky = false;
            if constexpr (Guard > 1) {
                for (size_type i = M + 1; i < M + Guard; ++i) {
                    if (mant.get_trit(i) != Trit::Z) { sticky = true; break; }
                }
            }
            round_up = (guard == Trit::P) || (guard == Trit::Z && sticky);
        }

        T81Int<M> final_m;
        for (size_type i = 0; i < M; ++i) final_m[i] = mant.get_trit(i);
        if (round_up) final_m = final_m + T81Int<M>(1);

        // Overflow from rounding
        if (leading_trit(final_m) == M - 1) {
            final_m >>= 1;
            ++exp;
        }

        // Special values and subnormals
        if (exp >= kInfExponent) {
            return inf(sign == Trit::P);
        }
        if (exp <= 0) {
            const std::int64_t under = 1 - exp;
            if (under >= static_cast<std::int64_t>(M)) {
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
// Arithmetic operators — unchanged (still double-backed for speed)
// ======================================================================
template <std::size_t M, std::size_t E>
T81Float<M, E> operator+(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    using F = T81Float<M, E>;
    if (a.is_nae() || b.is_nae()) return F::nae();
    if (a.is_inf() || b.is_inf()) {
        if (a.is_inf() && b.is_inf() && a.is_negative() != b.is_negative()) return F::nae();
        return a.is_inf() ? a : b;
    }
    return F::from_double(a.to_double() + b.to_double());
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator-(T81Float<M, E> a, T81Float<M, E> b) noexcept { return a + (-b); }

template <std::size_t M, std::size_t E>
T81Float<M, E> operator*(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    using F = T81Float<M, E>;
    if (a.is_nae() || b.is_nae()) return F::nae();
    const bool a_zero = a.is_zero(), b_zero = b.is_zero();
    const bool a_inf = a.is_inf(), b_inf = b.is_inf();
    if ((a_inf && b_zero) || (b_inf && a_zero)) return F::nae();
    if (a_inf || b_inf) return F::inf(a.is_negative() ^ b.is_negative());
    if (a_zero || b_zero) return F::zero();
    return F::from_double(a.to_double() * b.to_double());
}

template <std::size_t M, std::size_t E>
T81Float<M, E> operator/(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    using F = T81Float<M, E>;
    if (a.is_nae() || b.is_nae()) return F::nae();
    const bool a_zero = a.is_zero(), b_zero = b.is_zero();
    const bool a_inf = a.is_inf(), b_inf = b.is_inf();
    if (b_zero) {
        if (a_zero || a_inf) return F::nae();
        return F::inf(a.is_negative() ^ b.is_negative());
    }
    if (a_inf || b_inf) {
        if (a_inf && b_inf) return F::nae();
        return (a_inf) ? F::inf(a.is_negative() ^ b.is_negative()) : F::zero();
    }
    if (a_zero) return F::zero();
    return F::from_double(a.to_double() / b.to_double());
}

template <std::size_t M, std::size_t E>
T81Float<M, E> fma(T81Float<M, E> a, T81Float<M, E> b, T81Float<M, E> c) noexcept {
    return a * b + c;
}

// ======================================================================
// Correct double conversions (powers of 3)
// ======================================================================
template <std::size_t M, std::size_t E>
T81Float<M, E> T81Float<M, E>::from_double(double v) noexcept {
    using F = T81Float<M, E>;
    if (v == 0.0) return F::zero();
    if (std::isinf(v)) return F::inf(v > 0.0);
    if (std::isnan(v)) return F::nae();

    const bool neg = v < 0.0;
    double mag = std::fabs(v);
    int bin_exp = 0;
    double frac = std::frexp(mag, &bin_exp);  // [0.5, 1)

    // Convert binary exponent to ternary scale
    std::int64_t ternary_exp = 0;
    double mant = frac;
    while (mant >= 1.0) { mant /= 3.0; ternary_exp++; }
    while (mant < 1.0/3.0) { mant *= 3.0; ternary_exp--; }

    // Build mantissa with M trits
    T81Int<M + 10> imant;
    double acc = mant;
    for (size_type i = M + 9; i-- > 0 && acc > 0.0;) {
        acc *= 3.0;
        int digit = static_cast<int>(acc);
        if (digit > 1) digit = -1, acc -= 2.0;
        else if (digit < -1) digit = 1, acc += 2.0;
        imant[i] = int_to_trit(digit);
        acc -= digit;
    }

    return normalize<10>(neg ? Trit::N : Trit::P,
                         ternary_exp + kExponentBias,
                         imant);
}

template <std::size_t M, std::size_t E>
double T81Float<M, E>::to_double() const noexcept {
    if (is_zero()) return 0.0;
    if (is_inf()) return is_negative() ? -INFINITY : INFINITY;
    if (is_nae()) return std::numeric_limits<double>::quiet_NaN();

    std::int64_t exp = get_exp() - kExponentBias;
    double value = 0.0;
    double p = 1.0;

    const T81Int<M> m = get_mantissa();
    // Implicit leading P for normalized numbers
    if (exp > 0) {
        value += 1.0;
        p = 3.0;
        for (size_type i = 0; i < M; ++i) {
            value += trit_to_int(m[i]) * p;
            p *= 3.0;
        }
    } else {
        // Subnormal or zero
        for (size_type i = 0; i < M; ++i) {
            value += trit_to_int(m[i]) * p;
            p *= 3.0;
        }
    }

    // Apply exponent (ternary)
    if (exp >= 0) {
        for (std::int64_t i = 0; i < exp; ++i) value *= 3.0;
    } else {
        for (std::int64_t i = 0; i < -exp; ++i) value /= 3.0;
    }

    return is_negative() ? -value : value;
}

// Common typedefs
using T81Float18_9 = T81Float<18, 9>;
using T81Float27_9 = T81Float<27, 9>;

} // namespace t81::core
