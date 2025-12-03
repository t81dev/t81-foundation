/**
 * @file T81Complex.hpp
 * @brief Defines the T81Complex class, a balanced ternary complex number type.
 */

#pragma once

#include "t81/core/T81Float.hpp"

#include <cstddef>
#include <cstdint>
#include <compare>
#include <cmath>
#include <complex>
#include <string>
#include <type_traits>

namespace t81 {

// ======================================================================
// T81Complex<MantissaTrits> — Balanced ternary complex number
// ======================================================================

template <std::size_t MantissaTrits = 18>
class T81Complex {
    static_assert(MantissaTrits == 18 || MantissaTrits == 27,
                  "T81Complex only supports MantissaTrits = 18 or 27 for now");

public:
    using FloatType = std::conditional_t<MantissaTrits == 18,
                                         T81Float<18, 9>,
                                         T81Float<27, 9>>;

    using Float = FloatType;

private:
    static constexpr std::size_t kExponentTrits = 9;
    static constexpr std::size_t kTotalTrits    = MantissaTrits + kExponentTrits;

public:
    Float re{};
    Float im{};

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------

    constexpr T81Complex() noexcept = default;

    constexpr T81Complex(const Float& real, const Float& imag) noexcept
        : re(real), im(imag) {}

    constexpr explicit T81Complex(const Float& real) noexcept
        : re(real), im(Float::from_double(0.0)) {}

    explicit T81Complex(const std::complex<double>& z)
        : re(Float::from_double(z.real())),
          im(Float::from_double(z.imag())) {}

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr const Float& real() const noexcept { return re; }
    [[nodiscard]] constexpr const Float& imag() const noexcept { return im; }

    [[nodiscard]] constexpr Float& real() noexcept { return re; }
    [[nodiscard]] constexpr Float& imag() noexcept { return im; }

    // ------------------------------------------------------------------
    // Constants
    // ------------------------------------------------------------------

    static constexpr T81Complex zero() noexcept { return {}; }

    static T81Complex one() noexcept {
        return { Float::from_double(1.0), Float::from_double(0.0) };
    }

    static T81Complex i() noexcept {
        return { Float::from_double(0.0), Float::from_double(1.0) };
    }

    // ------------------------------------------------------------------
    // Arithmetic
    // ------------------------------------------------------------------

    [[nodiscard]] T81Complex operator+(const T81Complex& o) const noexcept {
        return { re + o.re, im + o.im };
    }

    [[nodiscard]] T81Complex operator-(const T81Complex& o) const noexcept {
        return { re - o.re, im - o.im };
    }

    [[nodiscard]] T81Complex operator-() const noexcept {
        return { -re, -im };
    }

    [[nodiscard]] T81Complex operator*(const T81Complex& o) const noexcept {
        const Float ac = re * o.re;
        const Float bd = im * o.im;
        const Float ad = re * o.im;
        const Float bc = im * o.re;
        return { ac - bd, ad + bc };
    }

    [[nodiscard]] T81Complex conj() const noexcept {
        return { re, -im };
    }

    [[nodiscard]] Float mag2() const noexcept {
        return re * re + im * im;
    }

    [[nodiscard]] Float phase() const noexcept {
        const double x = re.to_double();
        const double y = im.to_double();

        if (x == 0.0 && y == 0.0) {
            return Float::from_double(0.0);
        }

        constexpr double kTwoPi = 6.28318530717958647692;
        double angle = std::atan2(y, x);
        double turns = angle / kTwoPi;
        if (turns < 0.0) turns += 1.0;
        return Float::from_double(turns);
    }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr auto operator<=>(const T81Complex& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Complex& o) const noexcept = default;

    // ------------------------------------------------------------------
    // Utilities
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return re.is_zero() && im.is_zero();
    }

    [[nodiscard]] constexpr bool is_real() const noexcept { return im.is_zero(); }
    [[nodiscard]] constexpr bool is_imag() const noexcept { return re.is_zero(); }

    friend T81Complex bind(const T81Complex& a, const T81Complex& b) noexcept { return a * b; }
    friend T81Complex unbind(const T81Complex& a, const T81Complex& b) noexcept { return a * b.conj(); }

    // ------------------------------------------------------------------
    // Debug formatting (non-trivial, but acceptable — we pay the cost only when used)
    // ------------------------------------------------------------------

    [[nodiscard]] std::string str() const {
        const double rv = re.to_double();
        const double iv = im.to_double();
        std::string out = "(" + std::to_string(rv);
        out += (iv < 0.0) ? " - " : " + ";
        out += std::to_string(std::abs(iv)) + "i)";
        return out;
    }
};

// ======================================================================
// Aliases
// ======================================================================

using T81Complex18 = T81Complex<18>;
using T81Complex27 = T81Complex<27>;

// T81Float is now trivially copyable → T81Complex is trivially copyable
// This enables memcpy, zero-init, FFI, and bulk tensor operations
static_assert(std::is_trivially_copyable_v<T81Complex18>);
static_assert(std::is_trivially_copyable_v<T81Complex27>);

// ======================================================================
// Free functions
// ======================================================================

template <std::size_t M>
[[nodiscard]] T81Complex<M> expi(typename T81Complex<M>::FloatType theta) noexcept {
    constexpr double kTwoPi = 6.28318530717958647692;
    const double angle = theta.to_double() * kTwoPi;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    using Float = typename T81Complex<M>::FloatType;
    return T81Complex<M>(Float::from_double(c), Float::from_double(s));
}

[[nodiscard]] inline T81Complex18 mul3(const T81Complex18& a, const T81Complex18& b) noexcept {
    using Float = T81Complex18::Float;
    const Float p = a.re * (b.re + b.im);
    const Float q = b.re * (a.im + a.re);
    const Float r = a.im * (b.im - b.re);
    return T81Complex18{ p - q, p + r };
}

} // namespace t81

// std::complex interop
template <std::size_t M>
inline std::complex<double> to_complex(const t81::T81Complex<M>& z) {
    return { z.re.to_double(), z.im.to_double() };
}
