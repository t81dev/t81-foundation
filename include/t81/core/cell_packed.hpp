#pragma once
#include "t81/core/cell.hpp"

#include <array>
#include <cstdint>
#include <type_traits>

namespace t81::core::packed {

using Trit = t81::core::Trit;
using Index = std::uint8_t;

static constexpr std::array<Trit, Cell::TRITS> k_zero_trits = {};

constexpr Index trits_to_index(std::array<Trit, Cell::TRITS> trits) noexcept {
    Index idx = 0;
    Index mul = 1;
    for (int i = 0; i < Cell::TRITS; ++i) {
        idx += static_cast<Index>(static_cast<int>(trits[i]) + 1) * mul;
        mul *= 3;
    }
    return idx;
}

constexpr std::array<Trit, Cell::TRITS> index_to_trits(Index idx) noexcept {
    std::array<Trit, Cell::TRITS> trits{};
    for (int i = 0; i < Cell::TRITS; ++i) {
        int digit = idx % 3;
        trits[i] = static_cast<Trit>(digit - 1);
        idx /= 3;
    }
    return trits;
}

struct PackedCell {
    // The state is a base-3 encoding of the 5 trits, where each trit M, Z, P
    // is mapped to a digit 0, 1, 2. The max value is 3^5 - 1 = 242.
    // Negation on a trit digit `d` is `2 - d`. For the whole number `N`,
    // negation is `(3^5 - 1) - N`, which is `242 - N`.
    static constexpr Index MAX_INDEX = 242;
    Index state = 121; // Default to 0

    constexpr PackedCell() noexcept = default;
    constexpr explicit PackedCell(Index idx) noexcept : state(idx) {}

    constexpr static PackedCell from_trits(std::array<Trit, Cell::TRITS> trits) noexcept {
        return PackedCell(trits_to_index(trits));
    }

    [[nodiscard]] constexpr PackedCell operator-() const noexcept {
        return PackedCell(MAX_INDEX - state);
    }

    constexpr std::array<Trit, Cell::TRITS> trits() const noexcept {
        return index_to_trits(state);
    }
};

} // namespace t81::core::packed
