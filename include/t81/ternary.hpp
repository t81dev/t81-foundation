// ─────────────────────────────────────────────────────────────────────────────
// FILE: include/t81/ternary.hpp
// ─────────────────────────────────────────────────────────────────────────────
#pragma once
#include <cstdint>

namespace t81 {

enum class Trit : int8_t { Neg = -1, Zero = 0, Pos = 1 };

// Minimal uint81_t carrier: 128-bit storage envelope to hold up to 81-bit values.
struct uint81_t {
uint64_t lo{0};
uint32_t mid{0};
uint16_t hi{0};
constexpr uint81_t() = default;
constexpr explicit uint81_t(uint64_t v) : lo(v), mid(0), hi(0) {}
};

} // namespace t81
