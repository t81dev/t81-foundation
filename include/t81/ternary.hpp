#pragma once
#include <cstdint>

namespace t81 {

enum class Trit : int8_t { Neg = -1, Zero = 0, Pos = 1 };

#ifdef _MSC_VER
    using uint81_t = std::uint64_t;
#else
    using uint81_t = unsigned __int128;
#endif

} // namespace t81
