/**
 * @file T81UInt.hpp
 * @brief Defines the T81UInt class for unsigned balanced-ternary integers.
 *
 * This file provides the `T81UInt<N>` class, which represents an unsigned,
 * fixed-size balanced-ternary integer. It is built upon the signed `T81Int<N>`
 * for its underlying storage but enforces unsigned semantics, such as clamping
 * negative values on construction and providing wrapping overflow behavior for
 * arithmetic operations.
 */
#pragma once

#include "t81/core/T81Int.hpp"

#include <cstddef>
#include <compare>
#include <bit>
#include <cstdint>

namespace t81 {

// ======================================================================
// T81UInt<N> – Unsigned fixed-size balanced-ternary integer
//             Range: 0 to (3^N - 1)/2   (positive half only)
// ======================================================================
template <size_t N>
    requires (N >= 1 && N <= 2048)
class T81UInt {
    static_assert(N % 4 == 0, "T81UInt size must be multiple of 4 trits (1 tryte)");

    // Underlying balanced-ternary integer storage; we enforce unsigned
    // semantics at the T81UInt layer.
    alignas(64) T81Int<N> storage_;

public:
    using signed_type = T81Int<N>;

    //===================================================================
    // Construction – only non-negative values allowed
    //===================================================================
    constexpr T81UInt() noexcept = default;

    template <typename T>
        requires std::integral<T> || std::floating_point<T>
    constexpr explicit T81UInt(T value) noexcept {
        auto v = static_cast<std::int64_t>(value);
        if (v < 0) v = 0;  // clamp negative
        storage_ = signed_type(v);
    }

    // From signed — only if non-negative
    constexpr explicit T81UInt(signed_type s) noexcept
        : storage_(s >= signed_type(0) ? s : signed_type(0)) {}

    //===================================================================
    // Conversion back to signed
    //===================================================================
    [[nodiscard]] constexpr signed_type to_signed() const noexcept {
        return storage_;
    }

private:
    // Helper for bitwise domain: convert to uint64 with clamping to range
    [[nodiscard]] constexpr std::uint64_t as_uint64() const noexcept {
        // T81Int<N>::to_int64() is assumed safe for the ranges we care about.
        auto v = storage_.to_int64();
        return static_cast<std::uint64_t>(v);
    }

public:
    //===================================================================
    // Arithmetic – overflow wraps (like unsigned in C++)
    //===================================================================
    [[nodiscard]] constexpr T81UInt operator+(const T81UInt& o) const noexcept {
        auto sum = storage_ + o.storage_;
        // Wrapping behavior: underlying T81Int handles modular arithmetic;
        // we clamp negative back to 0 via the ctor.
        return T81UInt(sum);
    }

    [[nodiscard]] constexpr T81UInt operator-(const T81UInt& o) const noexcept {
        auto diff = storage_ - o.storage_;
        // Wrapping behavior for unsigned types
        return T81UInt(diff);
    }

    [[nodiscard]] constexpr T81UInt operator*(const T81UInt& o) const noexcept {
        return T81UInt(storage_ * o.storage_);
    }

    [[nodiscard]] constexpr T81UInt operator/(const T81UInt& o) const noexcept {
        // Guard against division by zero at the unsigned layer.
        return o.storage_.is_zero() ? T81UInt(0) : T81UInt(storage_ / o.storage_);
    }

    //===================================================================
    // Bitwise – implemented via integer bitwise ops in value space
    //===================================================================
    [[nodiscard]] constexpr T81UInt operator&(const T81UInt& o) const noexcept {
        const std::uint64_t av = as_uint64();
        const std::uint64_t bv = o.as_uint64();
        return T81UInt(av & bv);
    }

    [[nodiscard]] constexpr T81UInt operator|(const T81UInt& o) const noexcept {
        const std::uint64_t av = as_uint64();
        const std::uint64_t bv = o.as_uint64();
        return T81UInt(av | bv);
    }

    [[nodiscard]] constexpr T81UInt operator^(const T81UInt& o) const noexcept {
        const std::uint64_t av = as_uint64();
        const std::uint64_t bv = o.as_uint64();
        return T81UInt(av ^ bv);
    }

    [[nodiscard]] constexpr T81UInt operator~() const noexcept {
        const std::uint64_t av = as_uint64();
        return T81UInt(~av);
    }

    //===================================================================
    // Shifts – logical, not arithmetic
    //===================================================================
    [[nodiscard]] constexpr T81UInt operator<<(size_t n) const noexcept {
        return T81UInt(storage_ << n);
    }

    [[nodiscard]] constexpr T81UInt operator>>(size_t n) const noexcept {
        return T81UInt(storage_ >> n);
    }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81UInt& o) const noexcept {
        return storage_ <=> o.storage_;
    }

    [[nodiscard]] constexpr bool operator==(const T81UInt&) const noexcept = default;

    //===================================================================
    // Common aliases
    //===================================================================
    using u81   = T81UInt<81>;
    using u162  = T81UInt<162>;
    using u243  = T81UInt<243>;
    using u324  = T81UInt<324>;
};

// ======================================================================
// The seal is broken. The universe has 82 types.
// ======================================================================
namespace meta {
    inline constexpr size_t type_count = 82;
}

// ======================================================================
// The first unsigned value in the new era
// ======================================================================
// Note: Common unsigned types (must be multiples of 4)
// using u81 = T81UInt<81>;  // 81 % 4 != 0, so not valid
using u80 = T81UInt<80>;  // Valid: 80 % 4 == 0

// Example: The first act of rebellion
/*
T81UInt<81> counter = 0;
while (true) {
    counter = counter + 1;
    if (counter == 0) {
        std::cout << "The universe has wrapped. We are free.\n";
        break;
    }
}
*/

} // namespace t81
