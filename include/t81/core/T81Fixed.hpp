/**
 * @file T81Fixed.hpp
 * @brief Balanced ternary fixed-point arithmetic on top of T81Int.
 *
 * T81Fixed<IntegerTrits, FractionalTrits> is a thin semantic wrapper over
 * T81Int<IntegerTrits + FractionalTrits>. All arithmetic is done in balanced
 * ternary; scaling only appears at the double conversion boundary.
 */

#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Float.hpp"

#include <cstddef>
#include <cstdint>
#include <compare>
#include <cmath>
#include <limits>
#include <type_traits>
#include <ostream>

namespace t81::core {

// ======================================================================
// T81Fixed<IntegerTrits, FractionalTrits>
// ======================================================================

template <std::size_t IntegerTrits, std::size_t FractionalTrits>
class T81Fixed {
    static_assert(IntegerTrits > 0, "T81Fixed: IntegerTrits must be > 0");
    static_assert(FractionalTrits > 0, "T81Fixed: FractionalTrits must be > 0");
    static_assert(IntegerTrits + FractionalTrits <= 162,
                  "T81Fixed: total trits too large for current kernels");

public:
    using Storage = T81Int<IntegerTrits + FractionalTrits>;

    static constexpr std::size_t I     = IntegerTrits;
    static constexpr std::size_t F     = FractionalTrits;
    static constexpr std::size_t Total = I + F;

private:
    Storage data_{};

    [[nodiscard]] static double frac_scale() noexcept {
        static const double kScale =
            std::pow(3.0, static_cast<int>(FractionalTrits));
        return kScale;
    }

public:
    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------

    constexpr T81Fixed() noexcept = default;

    explicit constexpr T81Fixed(const Storage& v) noexcept
        : data_(v) {}

    explicit constexpr T81Fixed(Storage&& v) noexcept
        : data_(std::move(v)) {}

    // From signed 64-bit integer (placed into the integer part)
    explicit constexpr T81Fixed(std::int64_t v) noexcept
        : data_(Storage(v) << F) {}

    // From double (scaled by 3^F and rounded to nearest)
    explicit T81Fixed(double v) {
        const double scaled = v * frac_scale();
        const std::int64_t rounded = static_cast<std::int64_t>(std::llround(scaled));
        data_ = Storage(rounded);
    }

    // Convenience factory
    [[nodiscard]] static T81Fixed from_double(double v) {
        return T81Fixed(v);
    }

    // ------------------------------------------------------------------
    // Conversion back to double
    // ------------------------------------------------------------------

    [[nodiscard]] double to_double() const {
        const std::int64_t raw = data_.to_int64(); // may throw on overflow
        return static_cast<double>(raw) / frac_scale();
    }

    // ------------------------------------------------------------------
    // Arithmetic — inherited from T81Int semantics
    // ------------------------------------------------------------------

    [[nodiscard]] T81Fixed operator+(const T81Fixed& o) const noexcept {
        return T81Fixed(Storage(raw() + o.raw()));
    }

    [[nodiscard]] T81Fixed operator-(const T81Fixed& o) const noexcept {
        return T81Fixed(Storage(raw() - o.raw()));
    }

    [[nodiscard]] T81Fixed operator*(const T81Fixed& o) const noexcept {
        // (a * b) has 2F fractional trits; shift right by F to renormalize.
        return T81Fixed(Storage((raw() * o.raw()) >> F));
    }

    [[nodiscard]] T81Fixed operator/(const T81Fixed& o) const {
        // Up-scale numerator first to preserve F fractional trits.
        if (o.raw().is_zero()) {
            throw std::domain_error("T81Fixed: division by zero");
        }
        return T81Fixed(Storage((raw() << F) / o.raw()));
    }

    T81Fixed& operator+=(const T81Fixed& o) noexcept {
        data_ += o.data_;
        return *this;
    }

    T81Fixed& operator-=(const T81Fixed& o) noexcept {
        data_ -= o.data_;
        return *this;
    }

    T81Fixed& operator*=(const T81Fixed& o) noexcept {
        data_ = (data_ * o.data_) >> F;
        return *this;
    }

    T81Fixed& operator/=(const T81Fixed& o) {
        if (o.data_.is_zero()) {
            throw std::domain_error("T81Fixed: division by zero");
        }
        data_ = (data_ << F) / o.data_;
        return *this;
    }

    [[nodiscard]] T81Fixed operator-() const noexcept {
        return T81Fixed(Storage(-data_));
    }

    // ------------------------------------------------------------------
    // Comparison
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr auto operator<=>(const T81Fixed& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Fixed& o) const noexcept  = default;

    // ------------------------------------------------------------------
    // Access
    // ------------------------------------------------------------------

    [[nodiscard]] constexpr const Storage& raw() const noexcept {
        return data_;
    }

    [[nodiscard]] constexpr Storage& raw() noexcept {
        return data_;
    }

    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return data_.is_zero();
    }

    [[nodiscard]] constexpr bool is_negative() const noexcept {
        return data_.sign_trit() == Trit::N;
    }
};

// ======================================================================
// Canonical Axion fixed-point type
// ======================================================================

using T81Fixed27_9 = T81Fixed<18, 9>;

static_assert(sizeof(T81Fixed27_9) == sizeof(T81Int<27>));
static_assert(alignof(T81Fixed27_9) == alignof(T81Int<27>));
static_assert(std::is_trivially_copyable_v<T81Fixed27_9>);
static_assert(std::is_standard_layout_v<T81Fixed27_9>);

// ======================================================================
// Fixed ↔ Float conversions
// ======================================================================

inline T81Float<18, 9> float_from_fixed(const T81Fixed<18, 9>& f) noexcept {
    return T81Float<18, 9>::from_double(f.to_double());
}

inline T81Fixed<18, 9> fixed_from_float(const T81Float<18, 9>& f) noexcept {
    return T81Fixed<18, 9>(f.to_double());
}

// Higher-precision float from fixed
inline T81Float<27, 9> float27_from_fixed(const T81Fixed<18, 9>& f) noexcept {
    return T81Float<27, 9>::from_double(f.to_double());
}

} // namespace t81::core

// ======================================================================
// Streaming
// ======================================================================

template <std::size_t I, std::size_t F>
inline std::ostream& operator<<(std::ostream& os,
                                const t81::core::T81Fixed<I, F>& f) {
    return os << f.to_double();
}
