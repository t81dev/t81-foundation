/**
 * @file T81Fixed.hpp
 * @brief Defines the T81Fixed class for balanced ternary fixed-point arithmetic.
 *
 * This file provides a fixed-point numerical representation, T81Fixed, which is
 * built upon the T81Int class. It allows for deterministic arithmetic with a
 * fixed number of fractional trits, making it suitable for applications like
 * quantized neural network inference and other scenarios where floating-point
 * hardware is unavailable or non-deterministic behavior is undesirable.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include <cstdint>
#include <compare>
#include <cmath>
#include <limits>

namespace t81::core {

// ======================================================================
// T81Fixed<MantissaTrits, FracTrits>  —  Balanced ternary fixed-point
// ======================================================================
//
// Layout (for T81Fixed<27,9>):
//   [ 18 integer trits ] [ 9 fractional trits ]   → 27 trits total
//   ←────── integer ──────→←──── fractional ──→
//   Sign is part of the balanced ternary integer — no separate sign bit.
//
// This is literally just a constrained T81Int<27> with semantic sugar.
// All arithmetic is 100% identical to integer — no rounding, no overflow checks.
// Conversion to/from float is the ONLY place where scaling happens.
//
// This is why it's Priority #3: it's free once you have T81Int and T81Float.
//
// Used for:
//   • QAT-aware training (integer gradients)
//   • 4-bit / 3-trit quantized inference
//   • Edge devices with zero floating-point units
//   • Deterministic integer transformers
//
template <size_t IntegerTrits, size_t FractionalTrits>
class T81Fixed {
    static_assert(IntegerTrits + FractionalTrits <= 162, "T81Fixed too large for current kernels");

public:
    using Storage = T81Int<IntegerTrits + FractionalTrits>;

    static constexpr size_t I = IntegerTrits;
    static constexpr size_t F = FractionalTrits;
    static constexpr size_t Total = I + F;

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------
    constexpr T81Fixed() noexcept = default;

    constexpr T81Fixed(const Storage& v) noexcept : data(v) {}
    constexpr T81Fixed(Storage&& v) noexcept : data(std::move(v)) {}

    // From integer (places value at integer part)
    template <size_t N>
    constexpr T81Fixed(const T81Int<N>& v) noexcept {
        data = (v << F);  // shift left by fractional bits
    }

    // From double — with correct rounding
    explicit constexpr T81Fixed(double v) {
        // Scale by 3^F
        double scaled = v * std::pow(3.0, static_cast<int>(F));
        // Round to nearest integer in ternary
        int64_t rounded = static_cast<int64_t>(std::round(scaled));
        data = Storage(rounded);
    }

    // ------------------------------------------------------------------
    // Conversion back to double
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr double to_double() const noexcept {
        return static_cast<double>(data.to_int64()) / std::pow(3.0, static_cast<int>(F));
    }

    // ------------------------------------------------------------------
    // Arithmetic — fully reused from T81Int
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr T81Fixed operator+(const T81Fixed& o) const noexcept { return data + o.data; }
    [[nodiscard]] constexpr T81Fixed operator-(const T81Fixed& o) const noexcept { return data - o.data; }
    [[nodiscard]] constexpr T81Fixed operator*(const T81Fixed& o) const noexcept { return (data * o.data) >> F; }
    [[nodiscard]] constexpr T81Fixed operator/(const T81Fixed& o) const noexcept { return (data << F) / o.data; }

    [[nodiscard]] constexpr T81Fixed& operator+=(const T81Fixed& o) noexcept { data += o.data; return *this; }
    [[nodiscard]] constexpr T81Fixed& operator-=(const T81Fixed& o) noexcept { data -= o.data; return *this; }
    [[nodiscard]] constexpr T81Fixed& operator*=(const T81Fixed& o) noexcept { data = (data * o.data) >> F; return *this; }
    [[nodiscard]] constexpr T81Fixed& operator/=(const T81Fixed& o) noexcept { data = (data << F) / o.data; return *this; }

    [[nodiscard]] constexpr T81Fixed operator-() const noexcept { return -data; }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Fixed& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Fixed& o) const noexcept = default;

    // ------------------------------------------------------------------
    // Access
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr const Storage& raw() const noexcept { return data; }
    [[nodiscard]] constexpr Storage& raw() noexcept { return data; }

    [[nodiscard]] constexpr bool is_zero() const noexcept { return data.is_zero(); }
    [[nodiscard]] constexpr bool is_negative() const noexcept { return data.is_negative(); }

private:
    Storage data{};
};

// ======================================================================
// The One True Fixed-Point Type for Axion Inference
// ======================================================================
using T81Fixed27_9 = T81Fixed<18, 9>;

// Static asserts — these are the guarantees the compiler and hardware use
static_assert(sizeof(T81Fixed27_9) == sizeof(T81Int<27>));
static_assert(alignof(T81Fixed27_9) == alignof(T81Int<27>));
static_assert(std::is_trivially_copyable_v<T81Fixed27_9>);
static_assert(std::is_standard_layout_v<T81Fixed27_9>);

// ======================================================================
// Free functions — because hardware doesn't care about types
// ======================================================================

// Zero-cost conversion: fixed ↔ float (same storage size!)
[[nodiscard]] constexpr T81Float<18,9> float_from_fixed(T81Fixed<18,9> f) noexcept {
    return std::bit_cast<T81Float<18,9>>(f.raw());
}
[[nodiscard]] constexpr T81Fixed<18,9> fixed_from_float(T81Float<18,9> f) noexcept {
    return std::bit_cast<T81Fixed<18,9>>(f.raw());
}

// Same for higher precision
[[nodiscard]] constexpr T81Float<27,9> float27_from_fixed(T81Fixed<18,9> f) noexcept {
    return T81Float<27,9>(f.raw() << 9);  // zero-extend fractional part
}

} // namespace t81::core

// Optional: make it printable
template <size_t I, size_t F>
inline std::ostream& operator<<(std::ostream& os, t81::core::T81Fixed<I,F> f) {
    return os << f.to_double();
}
