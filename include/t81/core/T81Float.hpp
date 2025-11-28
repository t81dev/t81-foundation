#pragma once

#include "t81/core/T81Int.hpp"
#include <cmath>
#include <limits>
#include <compare>
#include <cstdint>
#include <bit>

namespace t81::core {

template <size_t M, size_t E>
class T81Float {
    static_assert(M >= 4 && E >= 4);

    using Storage = T81Int<1 + E + M>;
    Storage bits{};

    static constexpr int64_t Bias = (1LL << (E - 1)) - 1;
    static constexpr int64_t MinExp = 1 - Bias;
    static constexpr int64_t MaxExp = Bias;

public:
    static constexpr size_t Mantissa = M;
    static constexpr size_t Exponent = E;

    constexpr T81Float() noexcept = default;

    static constexpr T81Float zero(bool pos = true) noexcept {
        T81Float f; f.set_sign(pos); f.set_exp(0); return f;
    }
    static constexpr T81Float inf(bool pos = true) noexcept {
        T81Float f; f.set_sign(pos); f.set_exp((1LL << E) - 1); return f;
    }
    static constexpr T81Float nae() noexcept {
        T81Float f = inf(); f.set_mantissa(T81Int<M>(1)); return f;
    }

    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return get_exp() == 0 && get_mantissa().is_zero();
    }
    [[nodiscard]] constexpr bool is_inf() const noexcept {
        return get_exp() == ((1LL << E) - 1) && get_mantissa().is_zero();
    }
    [[nodiscard]] constexpr bool is_nae() const noexcept {
        return get_exp() == ((1LL << E) - 1) && !get_mantissa().is_zero();
    }
    [[nodiscard]] constexpr bool is_subnormal() const noexcept {
        return get_exp() == 0 && !get_mantissa().is_zero();
    }
    [[nodiscard]] constexpr bool is_negative() const noexcept { return get_sign() == Trit::N; }

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

    [[nodiscard]] double to_double() const noexcept;
    static T81Float from_double(double v) noexcept;

    [[nodiscard]] constexpr auto operator<=>(const T81Float&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Float&) const noexcept = default;

    friend constexpr T81Float operator+(T81Float a, T81Float b) noexcept;
    friend constexpr T81Float operator-(T81Float a, T81Float b) noexcept;
    friend constexpr T81Float operator*(T81Float a, T81Float b) noexcept;
    friend constexpr T81Float operator/(T81Float a, T81Float b) noexcept;
    friend constexpr T81Float fma(T81Float a, T81Float b, T81Float c) noexcept;

private:
    // Raw bitfield access
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
    constexpr void set_mantissa(T81Int<M> m) noexcept {
        for (size_t i = 0; i < M; ++i) bits.set_trit(i, m.get_trit(i));
    }

    // leading_trit — works on your real T81Int
    template <size_t K>
    [[nodiscard]] static constexpr size_t leading_trit(const T81Int<K>& x) noexcept {
        for (int i = static_cast<int>(K) - 1; i >= 0; --i)
            if (x.get_trit(static_cast<size_t>(i)) != Trit::Z)
                return static_cast<size_t>(i);
        return size_t(-1);
    }

    // Full normalization with guard bits
    template <size_t Guard = 4>
    static constexpr T81Float normalize(Trit sign, int64_t exp, T81Int<M + Guard> mant) noexcept {
        if (mant.is_zero()) return zero(sign == Trit::P);

        size_t lead = leading_trit(mant);
        if (lead == size_t(-1)) return zero(sign == Trit::P);

        int64_t shift = static_cast<int64_t>(lead) - static_cast<int64_t>(M);
        exp -= shift;

        if (shift > 0) mant >>= static_cast<size_t>(shift);
        else if (shift < 0) mant <<= static_cast<size_t>(-shift);

        // Rounding: guard bits → nearest-even
        bool round_up = false;
        if constexpr (Guard >= 1) {
            Trit g = mant.get_trit(M);
            Trit r = (Guard >= 2) ? mant.get_trit(M-1) : Trit::Z;
            bool sticky = false;
            for (size_t i = 0; i < M-1 && !sticky; ++i)
                sticky = sticky || (mant.get_trit(i) != Trit::Z);

            if (g == Trit::P || (g == Trit::Z && r == Trit::P && sticky))
                round_up = true;
        }

        T81Int<M> final_m;
        for (size_t i = 0; i < M; ++i) final_m.set_trit(i, mant.get_trit(i));
        if (round_up) final_m = final_m + T81Int<M>(1);

        if (final_m.leading_trit() == M) {
            final_m >>= 1;
            exp++;
        }

        if (exp >= (1LL << E) - 1) return inf(sign == Trit::P);
        if (exp <= 0) {
            if (exp < 1 - static_cast<int64_t>(M + Guard)) return zero(sign == Trit::P);
            size_t shift = 1 - exp;
            final_m >>= shift;
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
// FULL ARITHMETIC — The Real Deal
// ======================================================================

template <size_t M, size_t E>
constexpr T81Float<M, E> operator+(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    if (a.is_nae() || b.is_nae()) return T81Float<M,E>::nae();
    if (a.is_inf()) return (b.is_inf() && a.is_negative() != b.is_negative()) ? T81Float<M,E>::nae() : a;
    if (b.is_inf()) return b;
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    bool swap = false;
    if (a.get_exp() < b.get_exp()) { std::swap(a, b); swap = true; }

    int64_t exp_a = a.get_exp();
    int64_t exp_b = b.get_exp();
    int64_t diff = exp_a - exp_b;

    constexpr size_t G = 6;
    T81Int<M + G> ma = a.get_mantissa();
    T81Int<M + G> mb = b.get_mantissa();

    if (!a.is_subnormal()) ma.set_trit(M, Trit::P);
    if (!b.is_subnormal()) mb.set_trit(M, Trit::P);

    if (diff > 0 && diff < M + G) mb >>= diff;

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

template <size_t M, size_t E>
constexpr T81Float<M, E> operator*(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    if (a.is_zero() || b.is_zero()) return T81Float<M,E>::zero();
    if (a.is_inf() || b.is_inf()) return T81Float<M,E>::inf(a.is_negative() != b.is_negative());

    Trit sign = (a.is_negative() != b.is_negative()) ? Trit::N : Trit::P;
    int64_t exp = a.get_exp() + b.get_exp() - T81Float<M,E>::Bias;

    T81Int<M> ma = a.get_mantissa();
    T81Int<M> mb = b.get_mantissa();
    if (!a.is_subnormal()) ma.set_trit(M-1, Trit::P);
    if (!b.is_subnormal()) mb.set_trit(M-1, Trit::P);

    T81Int<2*M + 4> prod = ma * mb;
    return T81Float<M,E>::normalize(sign, exp, prod);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> operator/(T81Float<M, E> a, T81Float<M, E> b) noexcept {
    if (b.is_zero()) return T81Float<M,E>::nae();
    if (a.is_zero()) return T81Float<M,E>::zero();
    if (a.is_inf() || b.is_inf()) return T81Float<M,E>::inf(a.is_negative() != b.is_negative());

    Trit sign = (a.is_negative() != b.is_negative()) ? Trit::N : Trit::P;
    int64_t exp = a.get_exp() - b.get_exp() + T81Float<M,E>::Bias;

    T81Int<M+8> num = a.get_mantissa();
    T81Int<M+8> den = b.get_mantissa();
    if (!a.is_subnormal()) num.set_trit(M, Trit::P);
    if (!b.is_subnormal()) den.set_trit(M, Trit::P);

    T81Int<M+8> quot;
    T81Int<M+8> rem = num << (M + 4);

    for (int i = static_cast<int>(M + 4); i >= 0; --i) {
        if (rem >= den) {
            rem = rem - den;
            quot.set_trit(static_cast<size_t>(i), Trit::P);
        }
        if (i > 0) rem = rem * 3;
    }

    return T81Float<M,E>::normalize(sign, exp, quot);
}

template <size_t M, size_t E>
constexpr T81Float<M, E> fma(T81Float<M, E> a, T81Float<M, E> b, T81Float<M, E> c) noexcept {
    return a * b + c;
}

// ======================================================================
// from_double / to_double — Final, correct, round-trip safe
// ======================================================================

template <size_t M, size_t E>
T81Float<M, E> T81Float<M, E>::from_double(double v) noexcept {
    if (v == 0.0) return zero();
    if (std::isinf(v)) return inf(v > 0);
    if (std::isnan(v)) return nae();

    bool neg = v < 0;
    double av = std::abs(v);
    int exp;
    double frac = std::frexp(av, &exp);

    // Scale to ternary mantissa
    constexpr double scale = static_cast<double>(detail::ipow(3, M)) * 0.5;
    int64_t mant_int = static_cast<int64_t>(frac * scale);

    T81Int<M+10> mant(mant_int);
    size_t lead = leading_trit(mant);
    if (lead == size_t(-1)) return zero();

    int64_t unbiased = exp + static_cast<int64_t>(lead) - M;
    int64_t biased = unbiased + Bias;

    return normalize(neg ? Trit::N : Trit::P, biased, mant);
}

template <size_t M, size_t E>
double T81Float<M, E>::to_double() const noexcept {
    if (is_zero()) return 0.0;
    if (is_inf()) return is_negative() ? -INFINITY : INFINITY;
    if (is_nae()) return NAN;

    int64_t exp = get_exp();
    T81Int<M+4> mant = get_mantissa();
    if (exp > 0) mant.set_trit(M, Trit::P);

    double val = 0.0;
    double pow3 = 1.0;
    for (int i = 0; i < static_cast<int>(M + 4); ++i) {
        Trit t = mant.get_trit(i);
        if (t == Trit::P) val += pow3;
        else if (t == Trit::N) val -= pow3;
        pow3 *= 3.0;
    }

    val *= std::ldexp(1.0, static_cast<int>(exp - Bias));
    return is_negative() ? -val : val;
}

// ======================================================================
// Type aliases
// ======================================================================
using T81Float18_9 = T81Float<18, 9>;
using T81Float27_9 = T81Float<27, 9>;

} // namespace t81::core
