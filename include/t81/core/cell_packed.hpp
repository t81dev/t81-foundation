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

constexpr std::array<Index, Cell::MAX - Cell::MIN + 1> build_neg_table() noexcept {
    std::array<Index, Cell::MAX - Cell::MIN + 1> table{};
    for (Index i = 0; i < table.size(); ++i) {
        auto trits = index_to_trits(i);
        for (auto& t : trits) {
            t = static_cast<Trit>(-static_cast<int>(t));
        }
        table[i] = trits_to_index(trits);
    }
    return table;
}

inline const std::array<Index, Cell::MAX - Cell::MIN + 1> k_neg_lookup = build_neg_table();

struct PackedCell {
    Index state = 0;

    constexpr PackedCell() noexcept = default;
    constexpr explicit PackedCell(Index idx) noexcept : state(idx) {}

    constexpr static PackedCell from_trits(std::array<Trit, Cell::TRITS> trits) noexcept {
        return PackedCell(trits_to_index(trits));
    }

    constexpr PackedCell neg() const noexcept {
        return PackedCell(k_neg_lookup[state]);
    }

    constexpr std::array<Trit, Cell::TRITS> trits() const noexcept {
        return index_to_trits(state);
    }
};

} // namespace t81::core::packed
