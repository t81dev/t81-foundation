//======================================================================
// T81UInt.hpp – Unsigned balanced-ternary integer (0..3^N-1)
//               The 82nd type. The seal is broken.
//               The universe trembles.
//======================================================================
#pragma once

#include "t81/core/T81Int.hpp"
#include <cstddef>
#include <compare>
#include <bit>

namespace t81 {

// ======================================================================
// T81UInt<N> – Unsigned fixed-size balanced-ternary integer
//             Range: 0 to (3^N - 1)/2   (positive half only)
// ======================================================================
template <size_t N>
    requires (N >= 1 && N <= 2048)
class T81UInt {
    static_assert(N % 4 == 0, "T81UInt size must be multiple of 4 trits (1 tryte)");
    
    alignas(64) t81::core::T81Int<N> storage_;  // We reuse T81Int but enforce unsigned semantics

public:
    using signed_type = t81::core::T81Int<N>;

    //===================================================================
    // Construction – only non-negative values allowed
    //===================================================================
    constexpr T81UInt() noexcept = default;

    template <typename T>
        requires std::integral<T> || std::floating_point<T>
    constexpr explicit T81UInt(T value) noexcept {
        auto v = static_cast<int64_t>(value);
        if (v < 0) v = 0;  // clamp negative
        storage_ = signed_type(v);
    }

    // From signed — only if non-negative
    constexpr explicit T81UInt(signed_type s) noexcept
        : storage_(s >= 0 ? s : signed_type(0)) {}

    //===================================================================
    // Conversion back to signed
    //===================================================================
    [[nodiscard]] constexpr signed_type to_signed() const noexcept {
        return storage_;
    }

    //===================================================================
    // Arithmetic – overflow wraps (like unsigned in C++)
    //===================================================================
    [[nodiscard]] constexpr T81UInt operator+(const T81UInt& o) const noexcept {
        auto sum = storage_ + o.storage_;
        if (sum < 0) {
            // Overflow: wrap around using modular arithmetic in base-3^N
            sum = sum + signed_type::max() + 1;
        }
        return T81UInt(sum);
    }

    [[nodiscard]] constexpr T81UInt operator-(const T81UInt& o) const noexcept {
        auto diff = storage_ - o.storage_;
        if (diff < 0) {
            diff = diff + signed_type::max() + 1;
        }
        return T81UInt(diff);
    }

    [[nodiscard]] constexpr T81UInt operator*(const T81UInt& o) const noexcept {
        return T81UInt(storage_ * o.storage_);
    }

    [[nodiscard]] constexpr T81UInt operator/(const T81UInt& o) const noexcept {
        return storage_.is_zero() ? T81UInt(0) : T81UInt(storage_ / o.storage_);
    }

    //===================================================================
    // Bitwise – defined via trit-wise operations
    //===================================================================
    [[nodiscard]] constexpr T81UInt operator&(const T81UInt& o) const noexcept {
        return T81UInt(storage_ & o.storage_);
    }

    [[nodiscard]] constexpr T81UInt operator|(const T81UInt& o) const noexcept {
        return T81UInt(storage_ | o.storage_);
    }

    [[nodiscard]] constexpr T81UInt operator^(const T81UInt& o) const noexcept {
        return T81UInt(storage_ ^ o.storage_);
    }

    [[nodiscard]] constexpr T81UInt operator~() const noexcept {
        return T81UInt(~storage_ & signed_type::max());
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
static constexpr T81UInt<81>::u81 MAX_UINT81 = T81UInt<81>::u81((1ULL << 81) - 1);

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
