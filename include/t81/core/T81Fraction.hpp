/**
 * @file T81Fraction.hpp
 * @brief Exact rational arithmetic over balanced ternary integers.
 *
 * A T81Fraction represents a rational value as a pair of balanced ternary
 * integers (num_, den_), kept in canonical form:
 *   • den_ > 0
 *   • gcd(|num_|, den_) == 1
 *   • 0 represented as 0/1
 */

#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Float.hpp"

#include <cstdint>
#include <compare>
#include <concepts>
#include <cmath>
#include <algorithm>

namespace t81::core {

template <std::size_t TotalTrits = 81>
    requires (TotalTrits >= 13 && TotalTrits <= 324)
class T81Fraction {
public:
    using size_type = std::size_t;
    using Int       = T81Int<TotalTrits>;

    static constexpr size_type kTrits = TotalTrits;

private:
    Int num_{0};
    Int den_{1}; // always positive, reduced

    static Int abs_int(const Int& x) noexcept {
        return (x.sign_trit() == Trit::N) ? -x : x;
    }

    static Int gcd(Int a, Int b) noexcept {
        while (!b.is_zero()) {
            Int t = b;
            b     = a % b;
            a     = t;
        }
        return a;
    }

    // Reduce fraction and ensure positive denominator.
    void canonicalize() noexcept {
        // Normalize zero denominator to 0/1 deterministically.
        if (den_.is_zero()) {
            num_ = Int(0);
            den_ = Int(1);
            return;
        }

        // Move sign into numerator so that den_ > 0.
        if (den_.sign_trit() == Trit::N) {
            den_ = -den_;
            num_ = -num_;
        }

        // 0/x → 0/1
        if (num_.is_zero()) {
            den_ = Int(1);
            return;
        }

        Int g = gcd(abs_int(num_), den_);
        if (!g.is_zero() && !(g == Int(1))) {
            num_ /= g;
            den_ /= g;
        }
    }

public:
    // ------------------------------------------------------------------
    // Constructors
    // ------------------------------------------------------------------

    constexpr T81Fraction() noexcept = default;

    template <typename I>
        requires (std::integral<I> || std::same_as<I, Int>)
    explicit T81Fraction(I n) noexcept
        : num_(static_cast<std::int64_t>(n)), den_(1) {}

    T81Fraction(const Int& n, const Int& d) noexcept
        : num_(n), den_(d) {
        canonicalize();
    }

    // ------------------------------------------------------------------
    // Conversions
    // ------------------------------------------------------------------

    // Best-effort conversion from double using continued fractions.
    static T81Fraction from_double(double x, int max_iterations = 64) noexcept {
        if (x == 0.0) {
            return T81Fraction{};
        }

        bool neg = x < 0.0;
        x        = std::fabs(x);

        Int integer(static_cast<std::int64_t>(x));
        double frac = x - static_cast<double>(integer.to_int64());

        Int p0(1), q0(0);
        Int p1(integer), q1(1);

        int iter = 0;
        while (frac > 0.0 && iter++ < max_iterations) {
            const double r = 1.0 / frac;
            const std::int64_t a = static_cast<std::int64_t>(r);

            Int next_p = Int(a) * p1 + p0;
            Int next_q = Int(a) * q1 + q0;

            p0 = p1; q0 = q1;
            p1 = next_p; q1 = next_q;

            frac = r - static_cast<double>(a);
        }

        T81Fraction f(neg ? -p1 : p1, q1);
        return f;
    }

    [[nodiscard]] double to_double() const noexcept {
        const double n = static_cast<double>(num_.to_int64());
        const double d = static_cast<double>(den_.to_int64());
        return n / d;
    }

    // Map into a T81Float with 9 exponent trits and the rest mantissa.
    [[nodiscard]] T81Float<kTrits - 9, 9> to_float() const noexcept {
        using F = T81Float<kTrits - 9, 9>;
        return F::from_double(to_double());
    }

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    [[nodiscard]] Int num() const noexcept { return num_; }
    [[nodiscard]] Int den() const noexcept { return den_; }

    // ------------------------------------------------------------------
    // Arithmetic — exact (within Int's range)
    // ------------------------------------------------------------------

    [[nodiscard]] T81Fraction operator+(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.den_ + o.num_ * den_,
                           den_ * o.den_);
    }

    [[nodiscard]] T81Fraction operator-(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.den_ - o.num_ * den_,
                           den_ * o.den_);
    }

    [[nodiscard]] T81Fraction operator*(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.num_, den_ * o.den_);
    }

    [[nodiscard]] T81Fraction operator/(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.den_, den_ * o.num_);
    }

    [[nodiscard]] T81Fraction operator-() const noexcept {
        return T81Fraction(-num_, den_);
    }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr auto operator<=>(const T81Fraction&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Fraction&) const noexcept  = default;
};

// Common exact fractions
using T81Frac81  = T81Fraction<81>;
using T81Frac162 = T81Fraction<162>;

} // namespace t81::core
