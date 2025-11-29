/**
 * @file T81Fraction.hpp
 * @brief Defines the T81Fraction class for exact rational arithmetic.
 *
 * This file provides the T81Fraction class, which represents a fraction using
 * two balanced ternary integers (T81Int) for the numerator and denominator.
 * It performs exact rational arithmetic, automatically reducing fractions to
 * their canonical form (positive denominator, lowest terms) after every
 * operation to ensure correctness and prevent representation growth.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include <cstdint>
#include <compare>
#include <concepts>
#include <optional>

namespace t81::core {

template <size_t TotalTrits = 81>
    requires (TotalTrits >= 9 && TotalTrits <= 324)
class T81Fraction {
    using Int = T81Int<TotalTrits>;

    Int num_;
    Int den_;  // always positive, reduced

    // Canonicalize: reduce fraction, make denominator positive
    constexpr void canonicalize() noexcept {
        if (den_.is_negative()) {
            num_ = -num_;
            den_ = -den_;
        }
        if (den_.is_zero()) {
            num_ = Int(0);
            den_ = Int(1);
            return;
        }
        Int g = gcd(num_.abs(), den_);
        num_ /= g;
        den_ /= g;
    }

    static constexpr Int gcd(Int a, Int b) noexcept {
        while (!b.is_zero()) {
            Int t = b;
            b = a % b;
            a = t;
        }
        return a;
    }

public:
    static constexpr size_t trits = TotalTrits;

    constexpr T81Fraction() noexcept : num_(0), den_(1) {}

    template <typename I>
        requires std::integral<I> || std::same_as<I, T81Int<TotalTrits>>
    constexpr T81Fraction(I n) noexcept : num_(n), den_(1) {}

    constexpr T81Fraction(Int n, Int d) noexcept : num_(n), den_(d) {
        canonicalize();
    }

    // From double — best effort, exact when possible
    static constexpr T81Fraction from_double(double x, int max_iterations = 64) noexcept {
        if (x == 0.0) return T81Fraction{};
        bool neg = x < 0; x = std::abs(x);

        Int integer = Int(static_cast<int64_t>(x));
        double frac = x - static_cast<double>(integer.to_int64());

        Int p0(1), q0(0), p1(integer), q1(1);
        int i = 0;
        while (frac > 0.0 && i++ < max_iterations) {
            double r = 1.0 / frac;
            int64_t a = static_cast<int64_t>(r);
            Int next_p = Int(a) * p1 + p0;
            Int next_q = Int(a) * q1 + q0;
            p0 = p1; q0 = q1;
            p1 = next_p; q1 = next_q;
            frac = r - a;
        }
        T81Fraction f(neg ? -p1 : p1, q1);
        return f;
    }

    [[nodiscard]] constexpr Int num() const noexcept { return num_; }
    [[nodiscard]] constexpr Int den() const noexcept { return den_; }

    [[nodiscard]] constexpr double to_double() const noexcept {
        return static_cast<double>(num_.to_int64()) / static_cast<double>(den_.to_int64());
    }

    [[nodiscard]] constexpr T81Float<trits-9,9> to_float() const noexcept {
        return T81Float<trits-9,9>(num_) / T81Float<trits-9,9>(den_);
    }

    // Arithmetic — exact
    [[nodiscard]] constexpr T81Fraction operator+(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.den_ + o.num_ * den_, den_ * o.den_);
    }
    [[nodiscard]] constexpr T81Fraction operator-(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.den_ - o.num_ * den_, den_ * o.den_);
    }
    [[nodiscard]] constexpr T81Fraction operator*(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.num_, den_ * o.den_);
    }
    [[nodiscard]] constexpr T81Fraction operator/(const T81Fraction& o) const noexcept {
        return T81Fraction(num_ * o.den_, den_ * o.num_);
    }

    [[nodiscard]] constexpr T81Fraction operator-() const noexcept { return T81Fraction(-num_, den_); }

    [[nodiscard]] constexpr auto operator<=>(const T81Fraction&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Fraction&) const noexcept = default;
};

// Common exact fractions
using T81Frac81  = T81Fraction<81>;
using T81Frac162 = T81Fraction<162>;

static_assert(sizeof(T81Frac81) == 18); // 2×81 trits + padding → 18 bytes

} // namespace t81::core
