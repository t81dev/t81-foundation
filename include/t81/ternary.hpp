#pragma once
#include <cstdint>

namespace t81 {

// Balanced ternary trit: −1, 0, +1
enum class Trit : int8_t { Neg = -1, Zero = 0, Pos = 1 };

// A pragmatic 128-bit carrier used where “uint81” appears in specs.
// (It is *not* base-81; just a wide unsigned integer placeholder.)
using uint81_t = unsigned __int128;

} // namespace t81
