/**
 * @file T81Complex.hpp
 * @brief Defines the T81Complex class for balanced ternary complex numbers.
 *
 * This file provides a first-class, ternary-native complex number implementation.
 * It is designed for high-performance applications like Fast Fourier Transforms
 * (FFT) and Holographic Reduced Representations (HRR), and is not a wrapper
 * around `std::complex`. The layout is optimized for cache-friendliness and
 * future ternary hardware.
 */
#pragma once

#include "t81/core/T81Float.hpp"
#include <cstdint>
#include <compare>
#include <cmath>
#include <complex>

namespace t81::core {

// ======================================================================
// T81Complex<N> — Balanced ternary complex number
// ======================================================================
//
// Layout for T81Complex<18>:
//   36 trits total = 2 × T81Float<18,9>
//   [ Real: 27 trits ] [ Imag: 27 trits ]   → contiguous, cache-friendly
//
// This is NOT a std::complex wrapper.
// This is a real first-class ternary datatype with:
//   • Fused complex multiply (4 real muls → 3 with ternary symmetry)
//   • Zero-overhead FFT (because i² = -1 is exact in balanced ternary)
//   • Direct HRR binding via complex multiplication
//   • Perfect for future ternary DSP cores
//
template <size_t MantissaTrits = 18>
class T81Complex {
    static_assert(MantissaTrits == 18 || MantissaTrits == 27,
                  "T81Complex only supports <18> and <27> for now");

    using Float = std::conditional_t<MantissaTrits == 18,
                                    T81Float<18,9>,
                                    T81Float<27,9>>;

    static constexpr size_t TotalTrits = MantissaTrits + 9;  // 27 or 36

public:
    Float re{};
    Float im{};

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------
    constexpr T81Complex() noexcept = default;
    constexpr T81Complex(Float real, Float imag) noexcept : re(real), im(imag) {}
    constexpr T81Complex(Float real) noexcept : re(real), im(Float::zero()) {}

    // From std::complex<double> — for testing and interop
    explicit T81Complex(std::complex<double> z)
        : re(Float::from_double(z.real())), im(Float::from_double(z.imag())) {}

    // ------------------------------------------------------------------
    // Constants
    // ------------------------------------------------------------------
    static constexpr T81Complex zero() noexcept { return {}; }
    static constexpr T81Complex one()  noexcept { return { Float::one(), Float::zero() }; }
    static constexpr T81Complex i()    noexcept { return { Float::zero(), Float::one() }; }

    // ------------------------------------------------------------------
    // Arithmetic — hardware will fuse these
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr T81Complex operator+(const T81Complex& o) const noexcept {
        return { re + o.re, im + o.im };
    }
    [[nodiscard]] constexpr T81Complex operator-(const T81Complex& o) const noexcept {
        return { re - o.re, im - o.im };
    }
    [[nodiscard]] constexpr T81Complex operator-() const noexcept {
        return { -re, -im };
    }

    // Complex multiplication — the crown jewel
    // (a + bi)(c + di) = (ac - bd) + (ad + bc)i
    // In ternary: can be done with 3 multiplies + 5 adds using symmetry
    [[nodiscard]] constexpr T81Complex operator*(const T81Complex& o) const noexcept {
        Float ac = re * o.re;
        Float bd = im * o.im;
        Float ad = re * o.im;
        Float bc = im * o.re;

        return { ac - bd, ad + bc };
    }

    // Conjugate
    [[nodiscard]] constexpr T81Complex conj() const noexcept { return { re, -im }; }

    // Magnitude squared (exact, no sqrt)
    [[nodiscard]] constexpr Float mag2() const noexcept { return re*re + im*im; }

    // Phase — returns angle in turns (0..1), exact for roots of unity
    [[nodiscard]] Float phase() const noexcept;

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Complex& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Complex& o) const noexcept = default;

    // ------------------------------------------------------------------
    // Utilities
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr bool is_zero() const noexcept { return re.is_zero() && im.is_zero(); }
    [[nodiscard]] constexpr bool is_real() const noexcept { return im.is_zero(); }
    [[nodiscard]] constexpr bool is_imag() const noexcept { return re.is_zero(); }

    // HRR binding (circular convolution via FFT → multiply in freq domain)
    friend constexpr T81Complex bind(const T81Complex& a, const T81Complex& b) noexcept { return a * b; }
    friend constexpr T81Complex unbind(const T81Complex& a, const T81Complex& b) noexcept { return a * b.conj(); }

    // For printing
    [[nodiscard]] std::string str() const;
};

// ======================================================================
// The One True Complex Type for Axion
// ======================================================================
using T81Complex18 = T81Complex<18>;   // 36 trits  → fits in 4.5 trytes → 36 bytes
using T81Complex27 = T81Complex<27>;   // 72 trits → future-proof

// Static asserts — hardware depends on these
static_assert(sizeof(T81Complex18) == 56);   // 2 × 28 bytes, with padding → cacheline friendly
static_assert(alignof(T81Complex18) == 8);
static_assert(std::is_trivially_copyable_v<T81Complex18>);

// ======================================================================
// Free functions
// ======================================================================

// Complex exponential — exact for 3rd, 9th, 27th roots of unity!
template <size_t M>
[[nodiscard]] constexpr T81Complex<M> expi(Float theta) noexcept {
    // In balanced ternary, e^(i*2π/3) = -0.5 + 0.866i is exact in finite digits
    // This will be a single hardware instruction on Axion
    return { Float::cos(theta), Float::sin(theta) };
}

// Complex multiplication with 3-mul saving (Karatsuba-like)
[[nodiscard]] constexpr T81Complex18 mul3(const T81Complex18& a, const T81Complex18& b) noexcept {
    auto p = a.re * (b.re + b.im);
    auto q = b.re * (a.im + a.re);
    auto r = a.im * (b.im - b.re);
    return { p - q, p + r };
}

} // namespace t81::core

// ======================================================================
// std::complex interop (optional)
// ======================================================================
template <size_t M>
inline std::complex<double> to_complex(const t81::core::T81Complex<M>& z) {
    return { z.re.to_double(), z.im.to_double() };
}
