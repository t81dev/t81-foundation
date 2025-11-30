// include/t81/ternary.hpp
#pragma once
#include <cstdint>

namespace t81 {

// Balanced ternary trit: −1, 0, +1
enum class Trit : int8_t { Neg = -1, Zero = 0, Pos = 1 };

// 128-bit carrier for “uint81” concept in the spec.
// Falls back to two uint64_t on MSVC (which does not support __int128 on x64).

#ifdef _MSC_VER
    // MSVC: no __int128 → emulate with high/low pair
    struct uint81_t {
        std::uint64_t lo = 0;
        std::uint64_t hi = 0;

        constexpr uint81_t() noexcept = default;
        constexpr uint81_t(std::uint64_t v) noexcept : lo(v), hi(0) {}
        constexpr uint81_t(std::uint64_t l, std::uint64_t h) noexcept : lo(l), hi(h) {}

        // Minimal operators needed by the rest of the stack
        constexpr uint81_t& operator<<=(int shift) noexcept;
        constexpr uint81_t& operator>>=(int shift) noexcept;
        constexpr uint81_t  operator<<(int shift) const noexcept { auto t = *this; return t <<= shift; }
        constexpr uint81_t  operator>>(int shift) const noexcept { auto t = *this; return t >>= shift; }
    };
#else
    // GCC/Clang: native 128-bit support
    using uint81_t = unsigned __int128;
#endif

} // namespace t81

// If you need the shift operators outside the header (to avoid ODR issues),
// define them in a single .cpp file. For now we inline them for simplicity:

#ifdef _MSC_VER
inline constexpr t81::uint81_t& t81::uint81_t::operator<<=(int shift) noexcept {
    if (shift >= 128) { lo = hi = 0; return *this; }
    if (shift >= 64) { hi = lo; lo = 0; shift -= 64; }
    if (shift == 0) return *this;
    hi = (hi << shift) | (lo >> (64 - shift));
    lo <<= shift;
    return *this;
}

inline constexpr t81::uint81_t& t81::uint81_t::operator>>=(int shift) noexcept {
    if (shift >= 128) { lo = hi = 0; return *this; }
    if (shift >= 64) { lo = hi; hi = 0; shift -= 64; }
    if (shift == 0) return *this;
    lo = (lo >> shift) | (hi << (64 - shift));
    hi >>= shift;
    return *this;
}
#endif
