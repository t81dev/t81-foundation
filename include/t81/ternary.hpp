// include/t81/ternary.hpp
#pragma once
#include <cstdint>

namespace t81 {

// Balanced ternary trit: -1, 0, +1
enum class Trit : int8_t { Neg = -1, Zero = 0, Pos = 1 };

// "uint81_t" — a 128-bit unsigned integer used in the spec as a wide carrier.
// On MSVC we fall back to uint64_t — the current codebase NEVER uses more than 64 bits.
// (All real big-integer work is done in T81BigInt<T> with dynamic storage.)

#ifdef _MSC_VER
    using uint81_t = std::uint64_t;
#else
    using uint81_t = unsigned __int128;
#endif

} // namespace t81
