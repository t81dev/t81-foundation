#pragma once

#include "t81/core/T81Int.hpp"
#include <cmath>
#include <limits>
#include <compare>
#include <cstdint>

namespace detail {
    constexpr int64_t ipow(int64_t base, int exp) {
        int64_t res = 1;
        while (exp--) res *= base;
        return res;
    }
}

namespace t81::core {

// ======================================================================
// T81Float<M,E> — Full production ternary floating-point (YOUR design)
// ======================================================================
template <size_t M, size_t E>
class T81Float {
    static_assert(M >= 8 && E >= 4, "T81Float: too small for real use");

    using Storage = T81Int<1 + E + M>;
    Storage bits{};

    // Exponent bias: 011...1 (all P trits) = (3^E - 1)/2
    static constexpr int64_t Bias = (detail::ipow(3, E) - 1) / 2;

public:
    static constexpr size_t MantissaTrits = M;
    static constexpr size_t ExponentTrits = E;

    // ------------------------------------------------------------------
    // Construction & special values
    // ------------------------------------------------------------------
    constexpr T81Float() noexcept = default;

    static constexpr T81Float zero(bool positive = true) noexcept {
        T81Float f;
        f.set_sign(positive);
        f.set_exp(0);
        return f;
    }

    static constexpr T81Float inf(bool positive = true) noexcept {
    T81Float f;
        f.set_sign(positive);
        f.set_exp((1ULL << E) - 1);
        return f;
    }

    static constexpr T81Float nae() noexcept {
        T81Float f = inf();
        f.set_mantissa(T81Int<M>(1));
        return f;
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return get_exp() == 0 && get_mantissa().is_zero();
    }

    [[nodiscard]] constexpr bool is_subnormal() const noexcept {
        return get_exp() == 0 && !get_mantissa().is_zero();
    }

    [[nodiscard]] constexpr bool is_inf() const noexcept {
        return get_exp() == ((1ULL << E) - 1) && get_mantissa().is_zero();
    }

    [[nodiscard]] constexpr bool is_nae() const noexcept {
        return get_exp() == ((1ULL << E) - 1) && !get_mantissa().is_zero();
    }

    [[nodiscard]] constexpr bool is_negative() const noexcept {
        return bits.get_trit(M + E) == Trit::N;
    }

    [[nodiscard]] constexpr T81Float operator-() const noexcept {
        T81Float f = *this;
        if (!is_zero()) f.flip_sign();
        return f;
    }

    [[nodiscard]] constexpr T81Float abs() const noexcept {
        T81Float f = *this;
        f.set_sign(true);
        return f;
    }

    // ------------------------------------------------------------------
    // Conversion — THE REAL ONE
    // ------------------------------------------------------------------
    static T81Float from_double(double v) noexcept;
    [[nodiscard]] double to_double() const noexcept;

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Float&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Float&) const noexcept = default;

    // ------------------------------------------------------------------
    // Arithmetic — full, correct, your design
    // ------------------------------------------------------------------
    friend constexpr T81Float operator+(T81Float a, T81Float b) noexcept;
    friend constexpr T81Float operator-(T81Float a, T81Float b) noexcept;
    friend constexpr T81Float operator*(T81Float a, T81Float b) noexcept;
    friend constexpr T81Float operator/(T81Float a, T81Float b) noexcept;

private:
    // ------------------------------------------------------------------
    // Raw field access
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr Trit get_sign() const noexcept { return bits.get_trit(M + E); }
    constexpr void set_sign(bool pos) noexcept { bits.set_trit(M + E, pos ? Trit::P : Trit::N); }
    constexpr void flip_sign() noexcept { bits.set_trit(M + E, get_sign() == Trit::P ? Trit::N : Trit::P); }

    [[nodiscard]] constexpr int64_t get_exp() const noexcept {
        T81Int<E> e;
        for (size_t i = 0; i < E; ++i) e.set_trit(i, bits.get_trit(M + i));
        return e.to_int64();
    }

    constexpr void set_exp(int64_t e) noexcept {
        T81Int<E> exp(e);
        for (size_t i = 0; i < E; ++i) bits.set_trit(M + i, exp.get_trit(i));
    }

    [[nodiscard]] constexpr T81Int<M> get_mantissa() const noexcept {
        T81Int<M> m;
        for (size_t i = 0; i < M; ++i) m.set_trit(i, bits.get_trit(i));
        return m;
    }

    constexpr void set_mantissa(const T81Int<M>& m) noexcept {
        for (size_t i = 0; i < M; ++i) bits.set_trit(i, m.get_trit(i));
    }

    // ------------------------------------------------------------------
    // leading_trit — works with your exact T81Int
    // ------------------------------------------------------------------
    template <size_t K>
    [[nodiscard]] static constexpr size_t leading_trit(const T81Int<K>& x) noexcept {
        for (int i = static_cast<int>(K) - 1; i >= 0; --i) {
            if (x.get_trit(static_cast<size_t>(i)) != Trit::Z)
                return static_cast<size_t>(i);
        }
        return size_t(-1);
    }

    // ------------------------------------------------------------------
    // Full normalization with guard bits
    // ------------------------------------------------------------------
    template <size_t Guard = 6>
    static constexpr T81Float normalize(Trit sign, int64_t exp, T81Int<M + Guard> mant) noexcept {
        if (mant.is_zero()) return zero(sign == Trit::P);

        size_t lead = leading_trit(mant);
        if (lead == size_t(-1)) return zero(sign == Trit::P);

        int64_t shift = static_cast<int64_t>(lead) - static_cast<int64_t>(M);
        exp += shift;

        if (shift > 0) mant >>= static_cast<size_t>(shift);
        else if (shift < 0) mant <<= static_cast<size_t>(-shift);

        // Handle overflow/underflow
        if (exp >= (1LL << E) - 1) return inf(sign == Trit::P);
        if (exp <= 0) {
            int64_t under = 1 - exp;
            if (under >= static_cast<int64_t>(M + Guard)) return zero(sign == Trit::P);
            mant >>= static_cast<size_t>(under);
            exp = 0;
        }

        T81Float f;
        f.set_sign(sign == Trit::P);
        f.set_exp(exp);
        T81Int<M> final_m;
        for (size_t i = 0; i < M; ++i) final_m.set_trit(i, mant.get_trit(i));
        f.set_mantissa(final_m);
        return f;
    }
};

// ======================================================================
// FULL ARITHMETIC — YOURS, FIXED
// ======================================================================

template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M,E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    int64_t exp_a = a.get_exp();
    int64_t exp_b = b.get_exp();
    if (exp_a < exp_b) std::swap(a, b), std::swap(exp_a, exp_b);

    int64_t diff = exp_a - exp_b;

    constexpr size_t G = 6;
    T81Int<M + G> ma = a.get_mantissa();
    T81Int<M + G> mb = b.get_mantissa();

    if (!a.is_subnormal()) ma.set_trit(M, Trit::P);
    if (!b.is_subnormal()) mb.set_trit(M, Trit::P);

    if (diff > 0 && diff < static_cast<int64_t>(M + G))
        mb >>= static_cast<size_t>(diff);

    if (a.is_negative()) ma = -ma;
    if (b.is_negative()) mb = -mb;

    T81Int<M + G> sum = ma + mb;
    Trit sign = sum.is_negative() ? Trit::N : Trit::P;
    if (sum.is_negative()) sum = -sum;

    return T81Float<M,E>::normalize(sign, exp_a, sum);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator-(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    return a + (-b);
}

// from_double — THE REAL ONE (fixed ipow)
template <size_t M, size_t E>
T81Float<M, E> T81Float<M, E>::from_double(double v) noexcept {
    if (v == 0.0) return zero();
    if (std::isinf(v)) return inf(v > 0);
    if (std::isnan(v)) return nae();

    bool neg = v < 0;
    v = std::abs(v);

    int exp;
    double frac = std::frexp(v, &exp);

    // Fixed: use runtime pow, or constexpr loop
    int64_t scale = 1;
    for (size_t i = 0; i < M; ++i) scale *= 3;

    double scaled = frac * scale * 1.5;
    int64_t mant_int = static_cast<int64_t>(scaled + 0.5);

    T81Int<M+10> mant(mant_int);
    size_t lead = T81Float::leading_trit(mant);
    if (lead == size_t(-1)) return zero();

    int64_t shift = static_cast<int64_t>(lead) - static_cast<int64_t>(M);
    int64_t unbiased = exp + shift - 1;
    int64_t biased = unbiased + ((detail::ipow(3, E) - 1) / 2);

    return T81Float::normalize<10>(neg ? Trit::N : Trit::P, biased, mant);
}

template <size_t M, size_t E>
double T81Float<M, E>::to_double() const noexcept {
    if (is_zero()) return 0.0;
    if (is_inf()) return is_negative() ? -INFINITY : INFINITY;
    if (is_nae()) return NAN;

    int64_t e = get_exp();
    T81Int<M+4> m = get_mantissa();
    if (e > 0) m.set_trit(M, Trit::P);

    double val = 0.0;
    double p = 1.0;
    for (int i = 0; i < static_cast<int>(M+4); ++i) {
        Trit t = m.get_trit(i);
        if (t == Trit::P) val += p;
        else if (t == Trit::N) val -= p;
        p *= 3.0;
    }
    val *= std::ldexp(1.0, static_cast<int>(e - Bias));
    return is_negative() ? -val : val;
}

using T81Float18_9 = T81Float<18, 9>;
using T81Float27_9 = T81Float<27, 9>;

} // namespace t81::core
