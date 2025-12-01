/**
 * @file packing.hpp
 * @brief Balanced ternary packing helpers (8‑trit / 19‑trit encoders).
 *
 * These functions encode balanced-trit sequences into tightly packed binary
 * representations so we can compare the real storage cost versus naive Cell
 * arrays. The current implementation supports the canonical 8-trit→13-bit and
 * 19-trit→32-bit mappings described in the spec.
 */
#pragma once

#include "t81/core/cell.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace t81::core::packing {

using Trit = t81::core::Trit;

inline constexpr int encode_trit(Trit t) noexcept {
    return static_cast<int>(t) + 1; // map {-1,0,1} → {0,1,2}
}

inline constexpr Trit decode_digit(int digit) noexcept {
    return static_cast<Trit>(digit - 1);
}

template <std::size_t N>
constexpr std::uint64_t pack_trits(const std::array<Trit, N>& trits) noexcept {
    std::uint64_t value = 0;
    std::uint64_t multiplier = 1;
    for (std::size_t i = 0; i < N; ++i) {
        value += static_cast<std::uint64_t>(encode_trit(trits[i])) * multiplier;
        multiplier *= 3;
    }
    return value;
}

template <std::size_t N>
constexpr std::array<Trit, N> unpack_trits(std::uint64_t packed) noexcept {
    std::array<Trit, N> trits{};
    for (std::size_t i = 0; i < N; ++i) {
        int digit = static_cast<int>(packed % 3);
        trits[i] = decode_digit(digit);
        packed /= 3;
    }
    return trits;
}

constexpr std::uint64_t states_for_trits(std::size_t trits) noexcept {
    std::uint64_t states = 1;
    for (std::size_t i = 0; i < trits; ++i) {
        states *= 3;
    }
    return states;
}

constexpr std::size_t bits_for_states(std::uint64_t states) noexcept {
    std::size_t bits = 0;
    std::uint64_t value = 1;
    while (value < states) {
        value <<= 1;
        ++bits;
    }
    return bits;
}

constexpr std::size_t packed_bits(std::size_t trits) noexcept {
    return bits_for_states(states_for_trits(trits));
}

template <std::size_t N>
constexpr std::size_t packed_bits_v = packed_bits(N);

} // namespace t81::core::packing
