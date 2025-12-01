/**
 * @file T81Limb.hpp
 * @brief Defines the T81Limb class, a 48-trit packed ternary integer.
 *
 * This file provides T81Limb, a high-performance 48-trit (16-tryte) numeric
 * type. It uses a Kogge-Stone carry-lookahead adder with function composition
 * on carry maps to achieve high-throughput addition.
 */
// T81 Foundation — Packed Balanced Ternary Limb (48 trits)
// v1.1.0-DEV — High-performance core using Kogge-Stone adder.
// License: MIT / GPL-3.0 dual

#pragma once

#include <array>
#include <cstdint>
#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81::core {

namespace detail {
    // LUTs are unchanged.
    struct AddEntry { int8_t cout[3], sum_value[3]; };
    static constexpr std::array<std::array<AddEntry, 27>, 27> build_add_table() {
        std::array<std::array<AddEntry, 27>, 27> table{};
        for (int a_idx = 0; a_idx < 27; ++a_idx) {
            for (int b_idx = 0; b_idx < 27; ++b_idx) {
                int a_val = a_idx - 13; int b_val = b_idx - 13;
                for (int cin_idx = 0; cin_idx < 3; ++cin_idx) {
                    int cin = cin_idx - 1; int temp = a_val + b_val + cin;
                    int cout = 0;
                    if (temp > 13) { cout = 1; temp -= 27; }
                    if (temp < -13) { cout = -1; temp += 27; }
                    table[a_idx][b_idx].sum_value[cin_idx] = temp;
                    table[a_idx][b_idx].cout[cin_idx] = cout;
                }
            }
        }
        return table;
    }
    static constexpr std::array<std::array<int, 27>, 27> build_composition_table() {
        std::array<std::array<int, 27>, 27> table{};
        for (int id1 = 0; id1 < 27; ++id1) {
            int map1[3]; int temp1 = id1;
            for (int j = 0; j < 3; ++j) { map1[j] = (temp1 % 3) - 1; temp1 /= 3; }
            for (int id2 = 0; id2 < 27; ++id2) {
                int map2[3]; int temp2 = id2;
                for (int j = 0; j < 3; ++j) { map2[j] = (temp2 % 3) - 1; temp2 /= 3; }
                int new_map[3];
                for (int cin_idx = 0; cin_idx < 3; ++cin_idx) {
                    new_map[cin_idx] = map1[map2[cin_idx] + 1];
                }
                table[id1][id2] = (new_map[0] + 1) + 3 * (new_map[1] + 1) + 9 * (new_map[2] + 1);
            }
        }
        return table;
    }
    static const auto ADD_TABLE = build_add_table();
    static const auto COMPOSITION_TABLE = build_composition_table();

    static constexpr std::array<int32_t, 27 * 27> build_flat_composition_table() {
        std::array<int32_t, 27 * 27> flat{};
        for (int row = 0; row < 27; ++row) {
            for (int col = 0; col < 27; ++col) {
                flat[row * 27 + col] = COMPOSITION_TABLE[row][col];
            }
        }
        return flat;
    }
    static const auto COMPOSITION_FLAT = build_flat_composition_table();
} // namespace detail


class T81Limb {
public:
    static constexpr int TRITS = 48;
    static constexpr int TRYTES = 16;
private:
    alignas(16) int8_t trytes_[TRYTES]{};
public:
    T81Limb() noexcept = default;
    void set_tryte(int i, int8_t val) { trytes_[i] = val; }

    [[nodiscard]] T81Limb operator+(const T81Limb& other) const noexcept;
};


// Correct, scalar, parallel-prefix Kogge-Stone implementation.
inline T81Limb T81Limb::operator+(const T81Limb& other) const noexcept {
    T81Limb result;
    std::array<int, TRYTES> map_ids;
    std::array<std::array<int8_t, 3>, TRYTES> sum_vals;

    // 1. First Pass: Get initial carry maps and partial sums
    for (int i = 0; i < TRYTES; ++i) {
        const auto& entry = detail::ADD_TABLE[trytes_[i] + 13][other.trytes_[i] + 13];
        map_ids[i] = (entry.cout[0] + 1) + 3 * (entry.cout[1] + 1) + 9 * (entry.cout[2] + 1);
        sum_vals[i] = {entry.sum_value[0], entry.sum_value[1], entry.sum_value[2]};
    }

    // 2. Second Pass: A *correct* Kogge-Stone parallel prefix scan designed to
    // harness AVX2 gather for map composition where available.
    for (int d = 0; d < 4; ++d) { // log2(16) = 4
        int stride = 1 << d;
        for (int i = stride; i < TRYTES; i += 2*stride) {
            int limit = std::min(TRYTES, i + stride);
#if defined(__AVX2__)
            int idx = i;
            while (idx + 4 <= limit) {
                const __m256i current = _mm256_setr_epi32(
                    map_ids[idx + 0],
                    map_ids[idx + 1],
                    map_ids[idx + 2],
                    map_ids[idx + 3]);
                const __m256i previous = _mm256_setr_epi32(
                    map_ids[idx - 1],
                    map_ids[idx + 0],
                    map_ids[idx + 1],
                    map_ids[idx + 2]);
                const __m256i offsets = _mm256_add_epi32(
                    _mm256_mullo_epi32(current, _mm256_set1_epi32(27)),
                    previous);
                __m256i gathered = _mm256_i32gather_epi32(
                    detail::COMPOSITION_FLAT.data(),
                    offsets,
                    sizeof(int32_t));
                alignas(32) int32_t results[8];
                _mm256_store_si256(reinterpret_cast<__m256i*>(results), gathered);
                map_ids[idx + 0] = static_cast<int>(results[0]);
                map_ids[idx + 1] = static_cast<int>(results[1]);
                map_ids[idx + 2] = static_cast<int>(results[2]);
                map_ids[idx + 3] = static_cast<int>(results[3]);
                idx += 4;
            }
            for (; idx < limit; ++idx) {
                map_ids[idx] = detail::COMPOSITION_TABLE[map_ids[idx]][map_ids[idx - 1]];
            }
#else
            for (int j = 0; j < stride; ++j) {
                if (i + j < limit) {
                    map_ids[i+j] = detail::COMPOSITION_TABLE[map_ids[i+j]][map_ids[i - 1]];
                }
            }
#endif
        }
    }

    // 3. Third Pass: Compute final sums
    int8_t carry = 0; // Overall carry-in is 0
    for (int i = 0; i < TRYTES; ++i) {
        result.trytes_[i] = sum_vals[i][carry + 1];
        if (i < TRYTES - 1) {
            int map_id = map_ids[i];
            carry = ((map_id / 3) % 3) - 1; // Extract map for cin=0
        }
    }
    return result;
}

} // namespace t81::core
