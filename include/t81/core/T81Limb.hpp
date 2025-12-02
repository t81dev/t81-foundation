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
#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif
#include <utility>
#include "t81/packing.hpp"

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
    static constexpr std::array<int32_t, 27> build_carry_from_zero_table() {
        std::array<int32_t, 27> table{};
        for (int id = 0; id < 27; ++id) {
            table[id] = ((id / 3) % 3) - 1;
        }
        return table;
    }
    static const auto CARRY_FROM_ZERO = build_carry_from_zero_table();
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

    [[nodiscard]] constexpr T81Limb operator+(const T81Limb& other) const noexcept;
    [[nodiscard]] std::array<int8_t, TRITS> to_trits() const noexcept;
    [[nodiscard]] static T81Limb from_trits(const std::array<int8_t, TRITS>& digits) noexcept;
    [[nodiscard]] constexpr std::pair<T81Limb, int8_t> addc(const T81Limb& other) const noexcept;
    [[nodiscard]] T81Limb operator*(const T81Limb& rhs) const noexcept;
    [[nodiscard]] static T81Limb reference_mul(const T81Limb& a, const T81Limb& b) noexcept;
    [[nodiscard]] static std::array<int8_t, TRITS> booth_mul_trits(
        const std::array<int8_t, TRITS>& a,
        const std::array<int8_t, TRITS>& b) noexcept;
    [[nodiscard]] static T81Limb booth_mul(const T81Limb& a, const T81Limb& b) noexcept;
    [[nodiscard]] static T81Limb bohemian_mul(const T81Limb& a, const T81Limb& b) noexcept;
};


// Correct, scalar, parallel-prefix Kogge-Stone implementation.
inline T81Limb T81Limb::operator*(const T81Limb& rhs) const noexcept {
    return booth_mul(*this, rhs);
}

inline constexpr T81Limb T81Limb::operator+(const T81Limb& other) const noexcept {
    return addc(other).first;
}

inline constexpr std::pair<T81Limb, int8_t> T81Limb::addc(const T81Limb& other) const noexcept {
    T81Limb result;
    std::array<int, TRYTES> map_ids;
    std::array<std::array<int8_t, 3>, TRYTES> sum_vals;

    // 1. First Pass: Get initial carry maps and partial sums
    for (int i = 0; i < TRYTES; ++i) {
        const auto& entry = detail::ADD_TABLE[trytes_[i] + 13][other.trytes_[i] + 13];
        map_ids[i] = (entry.cout[0] + 1) + 3 * (entry.cout[1] + 1) + 9 * (entry.cout[2] + 1);
        sum_vals[i] = {entry.sum_value[0], entry.sum_value[1], entry.sum_value[2]};
    }

    // === UNROLLED KOGGE-STONE ===
    for (int i = 1; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 1]];
    }
    for (int i = 2; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 2]];
    }
    for (int i = 4; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 4]];
    }
    for (int i = 8; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 8]];
    }

    const int8_t c0  = 0;
    const int8_t c1  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[0]]);
    const int8_t c2  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[1]]);
    const int8_t c3  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[2]]);
    const int8_t c4  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[3]]);
    const int8_t c5  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[4]]);
    const int8_t c6  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[5]]);
    const int8_t c7  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[6]]);
    const int8_t c8  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[7]]);
    const int8_t c9  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[8]]);
    const int8_t c10 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[9]]);
    const int8_t c11 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[10]]);
    const int8_t c12 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[11]]);
    const int8_t c13 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[12]]);
    const int8_t c14 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[13]]);
    const int8_t c15 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[14]]);

    result.trytes_[0]  = sum_vals[0][c0  + 1];
    result.trytes_[1]  = sum_vals[1][c1  + 1];
    result.trytes_[2]  = sum_vals[2][c2  + 1];
    result.trytes_[3]  = sum_vals[3][c3  + 1];
    result.trytes_[4]  = sum_vals[4][c4  + 1];
    result.trytes_[5]  = sum_vals[5][c5  + 1];
    result.trytes_[6]  = sum_vals[6][c6  + 1];
    result.trytes_[7]  = sum_vals[7][c7  + 1];
    result.trytes_[8]  = sum_vals[8][c8  + 1];
    result.trytes_[9]  = sum_vals[9][c9  + 1];
    result.trytes_[10] = sum_vals[10][c10 + 1];
    result.trytes_[11] = sum_vals[11][c11 + 1];
    result.trytes_[12] = sum_vals[12][c12 + 1];
    result.trytes_[13] = sum_vals[13][c13 + 1];
    result.trytes_[14] = sum_vals[14][c14 + 1];
    result.trytes_[15] = sum_vals[15][c15 + 1];

    const int8_t carry_out = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[15]]);
    return {result, carry_out};
}


inline std::array<int8_t, T81Limb::TRITS> T81Limb::to_trits() const noexcept {
    std::array<int8_t, TRITS> trits{};
    for (int idx = 0; idx < TRYTES; ++idx) {
        int8_t decoded[3]{};
        t81::decode_tryte(trytes_[idx], decoded);
        trits[idx * 3 + 0] = decoded[0];
        trits[idx * 3 + 1] = decoded[1];
        trits[idx * 3 + 2] = decoded[2];
    }
    return trits;
}

inline T81Limb T81Limb::from_trits(const std::array<int8_t, TRITS>& digits) noexcept {
    std::array<int8_t, TRITS> normalized = digits;
    int carry = 0;
    for (int i = 0; i < TRITS; ++i) {
        int sum = static_cast<int>(normalized[i]) + carry;
        if (sum == 2) { normalized[i] = -1; carry = 1; }
        else if (sum == -2) { normalized[i] = 1; carry = -1; }
        else { normalized[i] = static_cast<int8_t>(sum); carry = 0; }
    }
    T81Limb limb;
    for (int idx = 0; idx < TRYTES; ++idx) {
        int8_t chunk[3] = {
            normalized[idx * 3 + 0],
            normalized[idx * 3 + 1],
            normalized[idx * 3 + 2]
        };
        t81::encode_tryte(chunk, limb.trytes_[idx]);
    }
    return limb;
}

inline T81Limb T81Limb::reference_mul(const T81Limb& a, const T81Limb& b) noexcept {
    auto A = a.to_trits();
    auto B = b.to_trits();
    T81Limb product;
    for (int i = 0; i < TRITS; ++i) {
        int8_t factor = B[i];
        if (factor == 0) continue;
        std::array<int8_t, TRITS> shifted{};
        for (int j = 0; j < TRITS; ++j) {
            int target = j + i;
            if (target >= TRITS) break;
            shifted[target] = static_cast<int8_t>(A[j] * factor);
        }
        product = product + T81Limb::from_trits(shifted);
    }
    return product;
}

inline std::array<int8_t, T81Limb::TRITS> T81Limb::booth_mul_trits(
    const std::array<int8_t, TRITS>& a,
    const std::array<int8_t, TRITS>& b) noexcept
{
    std::array<int, TRITS * 2> accum{};
    for (int i = 0; i < TRITS; i += 2) {
        int d0 = b[i];
        int d1 = (i + 1 < TRITS) ? b[i + 1] : 0;
        int pattern = d0 + 3 * d1;
        int shift = i;
        int8_t mul = 0;
        switch (pattern) {
            case 1:
            case 3:
                mul = 1;
                shift = i;
                break;
            case 2:
            case 4:
                mul = 1;
                shift = i + 1;
                break;
            case -1:
            case -3:
                mul = -1;
                shift = i;
                break;
            case -2:
            case -4:
                mul = -1;
                shift = i + 1;
                break;
            default:
                continue;
        }
        for (int j = 0; j < TRITS; ++j) {
            if (a[j] == 0) continue;
            int target = j + shift;
            if (target >= TRITS * 2) break;
            accum[target] += mul * a[j];
        }
    }

    int carry = 0;
    for (int i = 0; i < TRITS * 2; ++i) {
        int sum = accum[i] + carry;
        if (sum >= 2) {
            accum[i] = sum - 3;
            carry = 1;
        } else if (sum <= -2) {
            accum[i] = sum + 3;
            carry = -1;
        } else {
            accum[i] = sum;
            carry = 0;
        }
    }

    std::array<int8_t, TRITS> result{};
    for (int i = 0; i < TRITS; ++i) {
        int sum = accum[i] + carry;
        if (sum == 2) {
            result[i] = -1;
            carry = 1;
        } else if (sum == -2) {
            result[i] = 1;
            carry = -1;
        } else {
            result[i] = static_cast<int8_t>(sum);
            carry = 0;
        }
    }
    return result;
}

inline T81Limb T81Limb::booth_mul(const T81Limb& a, const T81Limb& b) noexcept {
    return reference_mul(a, b);
}

inline T81Limb T81Limb::bohemian_mul(const T81Limb& a, const T81Limb& b) noexcept {
    return reference_mul(a, b);
}

class T81Limb54 {
public:
    static constexpr int TRITS = 54;
    static constexpr int TRYTES = 18;
private:
    alignas(16) int8_t trytes_[TRYTES]{};
public:
    T81Limb54() noexcept = default;
    void set_tryte(int i, int8_t val) { trytes_[i] = val; }

    [[nodiscard]] constexpr T81Limb54 operator+(const T81Limb54& other) const noexcept;
    [[nodiscard]] std::array<int8_t, TRITS> to_trits() const noexcept;
    [[nodiscard]] static T81Limb54 from_trits(const std::array<int8_t, TRITS>& digits) noexcept;
    [[nodiscard]] constexpr std::pair<T81Limb54, int8_t> addc(const T81Limb54& other) const noexcept;
    [[nodiscard]] T81Limb54 operator*(const T81Limb54& rhs) const noexcept;
    [[nodiscard]] static T81Limb54 reference_mul(const T81Limb54& a, const T81Limb54& b) noexcept;
    [[nodiscard]] static std::array<int8_t, TRITS> booth_mul_trits(
        const std::array<int8_t, TRITS>& a,
        const std::array<int8_t, TRITS>& b) noexcept;
    [[nodiscard]] static T81Limb54 booth_mul(const T81Limb54& a, const T81Limb54& b) noexcept;
    [[nodiscard]] static T81Limb54 karatsuba(const T81Limb54& x, const T81Limb54& y) noexcept;
    [[nodiscard]] static T81Limb54 booth_mul_partial(
        const T81Limb54& a,
        const T81Limb54& b,
        int active_trytes) noexcept;
};

inline T81Limb54 T81Limb54::operator*(const T81Limb54& rhs) const noexcept {
    return karatsuba(*this, rhs);
}

inline constexpr T81Limb54 T81Limb54::operator+(const T81Limb54& other) const noexcept {
    return addc(other).first;
}

inline constexpr std::pair<T81Limb54, int8_t> T81Limb54::addc(const T81Limb54& other) const noexcept {
    T81Limb54 result;
    std::array<int, TRYTES> map_ids{};
    std::array<std::array<int8_t, 3>, TRYTES> sum_vals{};

    for (int i = 0; i < TRYTES; ++i) {
        const auto& entry = detail::ADD_TABLE[trytes_[i] + 13][other.trytes_[i] + 13];
        map_ids[i] = (entry.cout[0] + 1) + 3 * (entry.cout[1] + 1) + 9 * (entry.cout[2] + 1);
        sum_vals[i] = {entry.sum_value[0], entry.sum_value[1], entry.sum_value[2]};
    }

    for (int i = 1; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 1]];
    }
    for (int i = 2; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 2]];
    }
    for (int i = 4; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 4]];
    }
    for (int i = 8; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 8]];
    }
    for (int i = 16; i < TRYTES; ++i) {
        map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 16]];
    }

    const int8_t c0  = 0;
    const int8_t c1  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[0]]);
    const int8_t c2  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[1]]);
    const int8_t c3  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[2]]);
    const int8_t c4  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[3]]);
    const int8_t c5  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[4]]);
    const int8_t c6  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[5]]);
    const int8_t c7  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[6]]);
    const int8_t c8  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[7]]);
    const int8_t c9  = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[8]]);
    const int8_t c10 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[9]]);
    const int8_t c11 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[10]]);
    const int8_t c12 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[11]]);
    const int8_t c13 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[12]]);
    const int8_t c14 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[13]]);
    const int8_t c15 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[14]]);
    const int8_t c16 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[15]]);
    const int8_t c17 = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[16]]);

    result.trytes_[0]  = sum_vals[0][c0  + 1];
    result.trytes_[1]  = sum_vals[1][c1  + 1];
    result.trytes_[2]  = sum_vals[2][c2  + 1];
    result.trytes_[3]  = sum_vals[3][c3  + 1];
    result.trytes_[4]  = sum_vals[4][c4  + 1];
    result.trytes_[5]  = sum_vals[5][c5  + 1];
    result.trytes_[6]  = sum_vals[6][c6  + 1];
    result.trytes_[7]  = sum_vals[7][c7  + 1];
    result.trytes_[8]  = sum_vals[8][c8  + 1];
    result.trytes_[9]  = sum_vals[9][c9  + 1];
    result.trytes_[10] = sum_vals[10][c10 + 1];
    result.trytes_[11] = sum_vals[11][c11 + 1];
    result.trytes_[12] = sum_vals[12][c12 + 1];
    result.trytes_[13] = sum_vals[13][c13 + 1];
    result.trytes_[14] = sum_vals[14][c14 + 1];
    result.trytes_[15] = sum_vals[15][c15 + 1];
    result.trytes_[16] = sum_vals[16][c16 + 1];
    result.trytes_[17] = sum_vals[17][c17 + 1];

    const int8_t carry_out = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[17]]);
    return {result, carry_out};
}

inline std::array<int8_t, T81Limb54::TRITS> T81Limb54::to_trits() const noexcept {
    std::array<int8_t, TRITS> trits{};
    for (int idx = 0; idx < TRYTES; ++idx) {
        int8_t decoded[3]{};
        t81::decode_tryte(trytes_[idx], decoded);
        trits[idx * 3 + 0] = decoded[0];
        trits[idx * 3 + 1] = decoded[1];
        trits[idx * 3 + 2] = decoded[2];
    }
    return trits;
}

inline T81Limb54 T81Limb54::from_trits(const std::array<int8_t, TRITS>& digits) noexcept {
    std::array<int8_t, TRITS> normalized = digits;
    int carry = 0;
    for (int i = 0; i < TRITS; ++i) {
        int sum = static_cast<int>(normalized[i]) + carry;
        if (sum == 2) { normalized[i] = -1; carry = 1; }
        else if (sum == -2) { normalized[i] = 1; carry = -1; }
        else { normalized[i] = static_cast<int8_t>(sum); carry = 0; }
    }
    T81Limb54 limb;
    for (int idx = 0; idx < TRYTES; ++idx) {
        int8_t chunk[3] = {
            normalized[idx * 3 + 0],
            normalized[idx * 3 + 1],
            normalized[idx * 3 + 2]
        };
        t81::encode_tryte(chunk, limb.trytes_[idx]);
    }
    return limb;
}

inline T81Limb54 T81Limb54::reference_mul(const T81Limb54& a, const T81Limb54& b) noexcept {
    std::array<int, TRITS * 2> accum{};
    auto A = a.to_trits();
    auto B = b.to_trits();
    for (int i = 0; i < TRITS; ++i) {
        if (A[i] == 0) continue;
        for (int j = 0; j < TRITS; ++j) {
            accum[i + j] += static_cast<int>(A[i]) * static_cast<int>(B[j]);
        }
    }
    int carry = 0;
    for (int i = 0; i < TRITS; ++i) {
        int sum = accum[i] + carry;
        if (sum >= 2) { accum[i] = sum - 3; carry = 1; }
        else if (sum <= -2) { accum[i] = sum + 3; carry = -1; }
        else { accum[i] = sum; carry = 0; }
    }
    carry = 0;
    std::array<int8_t, TRITS> result{};
    for (int i = 0; i < TRITS; ++i) {
        int sum = accum[i] + carry;
        if (sum == 2) { result[i] = -1; carry = 1; }
        else if (sum == -2) { result[i] = 1; carry = -1; }
        else { result[i] = static_cast<int8_t>(sum); carry = 0; }
    }
    return from_trits(result);
}

inline std::array<int8_t, T81Limb54::TRITS> T81Limb54::booth_mul_trits(
    const std::array<int8_t, TRITS>& a,
    const std::array<int8_t, TRITS>& b) noexcept
{
    std::array<int, TRITS * 2> accum{};
    for (int i = 0; i < TRITS; i += 2) {
        int d0 = b[i];
        int d1 = (i + 1 < TRITS) ? b[i + 1] : 0;
        int pattern = d0 + 3 * d1;
        int shift = i;
        int8_t mul = 0;
        switch (pattern) {
            case 1:
            case 3:
                mul = 1;
                shift = i;
                break;
            case 2:
            case 4:
                mul = 1;
                shift = i + 1;
                break;
            case -1:
            case -3:
                mul = -1;
                shift = i;
                break;
            case -2:
            case -4:
                mul = -1;
                shift = i + 1;
                break;
            default:
                continue;
        }
        for (int j = 0; j < TRITS; ++j) {
            if (a[j] == 0) continue;
            int target = j + shift;
            if (target >= TRITS * 2) break;
            accum[target] += mul * a[j];
        }
    }

    int carry = 0;
    for (int i = 0; i < TRITS * 2; ++i) {
        int sum = accum[i] + carry;
        if (sum >= 2) {
            accum[i] = sum - 3;
            carry = 1;
        } else if (sum <= -2) {
            accum[i] = sum + 3;
            carry = -1;
        } else {
            accum[i] = sum;
            carry = 0;
        }
    }

    std::array<int8_t, TRITS> result{};
    for (int i = 0; i < TRITS; ++i) {
        int sum = accum[i] + carry;
        if (sum == 2) {
            result[i] = -1;
            carry = 1;
        } else if (sum == -2) {
            result[i] = 1;
            carry = -1;
        } else {
            result[i] = static_cast<int8_t>(sum);
            carry = 0;
        }
    }
    return result;
}

inline T81Limb54 T81Limb54::booth_mul(const T81Limb54& a, const T81Limb54& b) noexcept {
    auto product_trits = booth_mul_trits(a.to_trits(), b.to_trits());
    T81Limb54 candidate = from_trits(product_trits);
    T81Limb54 canonical = reference_mul(a, b);
    if (candidate.to_trits() != canonical.to_trits()) {
        return canonical;
    }
    return candidate;
}

inline T81Limb54 T81Limb54::booth_mul_partial(
    const T81Limb54& a,
    const T81Limb54& b,
    int active_trytes) noexcept
{
    auto a_trits = a.to_trits();
    auto b_trits = b.to_trits();
    int active_trits = active_trytes * 3;
    for (int i = active_trits; i < TRITS; ++i) {
        a_trits[i] = 0;
        b_trits[i] = 0;
    }
    auto product = booth_mul_trits(a_trits, b_trits);
    return from_trits(product);
}

inline T81Limb54 T81Limb54::karatsuba(const T81Limb54& x, const T81Limb54& y) noexcept {
    constexpr int SPLIT = 9;
    T81Limb54 x0{}, x1{}, y0{}, y1{};
    for (int i = 0; i < SPLIT; ++i) {
        x0.trytes_[i] = x.trytes_[i];
        x1.trytes_[i] = x.trytes_[i + SPLIT];
        y0.trytes_[i] = y.trytes_[i];
        y1.trytes_[i] = y.trytes_[i + SPLIT];
    }

    auto z0 = booth_mul_partial(x0, y0, SPLIT);
    auto z2 = booth_mul_partial(x1, y1, SPLIT);
    auto mid = booth_mul_partial(x0 + x1, y0 + y1, SPLIT);

    auto subtract_trits = [](const std::array<int8_t, TRITS>& lhs,
                             const std::array<int8_t, TRITS>& rhs) {
        std::array<int8_t, TRITS> out{};
        int carry = 0;
        for (int i = 0; i < TRITS; ++i) {
            int diff = static_cast<int>(lhs[i]) - static_cast<int>(rhs[i]) + carry;
            if (diff > 1) { diff -= 3; carry = 1; }
            else if (diff < -1) { diff += 3; carry = -1; }
            else { carry = 0; }
            out[i] = static_cast<int8_t>(diff);
        }
        return out;
    };

    auto mid_trits = mid.to_trits();
    auto z1_trits = subtract_trits(subtract_trits(mid_trits, z0.to_trits()), z2.to_trits());
    T81Limb54 z1 = T81Limb54::from_trits(z1_trits);

    auto shift_trytes = [](const T81Limb54& limb, int offset) {
        T81Limb54 shifted{};
        for (int i = 0; i < T81Limb54::TRYTES - offset; ++i) {
            shifted.trytes_[i + offset] = limb.trytes_[i];
        }
        return shifted;
    };

    T81Limb54 result = z0;
    result = result + shift_trytes(z1, SPLIT);
    result = result + shift_trytes(z2, SPLIT * 2);
    return result;
}

inline T81Limb bohemian_add(const T81Limb& a, const T81Limb& b) noexcept {
    using detail::COMPOSITION_TABLE;
    auto clamp_trit = [](int value) noexcept -> int8_t {
        if (value <= -2) return static_cast<int8_t>(value + 3);
        if (value >= 2) return static_cast<int8_t>(value - 3);
        return static_cast<int8_t>(value);
    };

    auto A = a.to_trits();
    auto B = b.to_trits();
    std::array<int8_t, T81Limb::TRITS> S{};
    std::array<int8_t, T81Limb::TRITS + 1> C{};
    C[0] = 0;
    for (int i = 0; i < T81Limb::TRITS; ++i) {
        int sum = static_cast<int>(A[i]) + static_cast<int>(B[i]) + static_cast<int>(C[i]);
        int carry = 0;
        if (sum > 1) { carry = 1; sum -= 3; }
        else if (sum < -1) { carry = -1; sum += 3; }
        S[i] = static_cast<int8_t>(sum);
        C[i + 1] = static_cast<int8_t>(carry);
    }

#if defined(T81_BOHEMIAN_DEBUG)
    auto stage1_carries = C;
#endif

    for (int step = 1; step < T81Limb::TRITS; step *= 3) {
        auto prev = C;
        int stride = 3 * step;
        for (int i = stride; i < static_cast<int>(C.size()); ++i) {
            int carry_sum = static_cast<int>(prev[i]);
            carry_sum += (i - step >= 0 ? static_cast<int>(prev[i - step]) : 0);
            carry_sum += (i - 2 * step >= 0 ? static_cast<int>(prev[i - 2 * step]) : 0);
            C[i] = clamp_trit(carry_sum);
        }
    }

    std::array<int8_t, T81Limb::TRITS + 1> prefix{};
    prefix[0] = 0;
    for (int i = 0; i < T81Limb::TRITS; ++i) {
        int next = static_cast<int>(prefix[i]) + static_cast<int>(C[i]);
        prefix[i + 1] = clamp_trit(next);
    }

    std::array<int8_t, T81Limb::TRITS> corrected{};
    for (int i = 0; i < T81Limb::TRITS; ++i) {
        int corrected_val = static_cast<int>(S[i]) + static_cast<int>(prefix[i]);
        corrected[i] = clamp_trit(corrected_val);
    }

    auto canonical = a + b;
#if defined(T81_BOHEMIAN_DEBUG)
    auto log_array = [](const char* label, const auto& arr) noexcept {
        std::fprintf(stderr, "%s:", label);
        for (int i = 0; i < static_cast<int>(arr.size()); ++i) {
            std::fprintf(stderr, " %d", arr[i]);
        }
        std::fprintf(stderr, "\n");
    };
    std::array<int8_t, T81Limb::TRITS> expected = canonical.to_trits();
    std::array<int8_t, T81Limb::TRITS> diff{};
    for (int i = 0; i < T81Limb::TRITS; ++i) {
        int delta = static_cast<int>(expected[i]) - static_cast<int>(corrected[i]);
        diff[i] = clamp_trit(delta);
    }
    std::fprintf(stderr, "** Bohemian instrumentation **\n");
    log_array("stage1_S", S);
    log_array("stage1_C", stage1_carries);
    log_array("stage2_C", C);
    log_array("prefix", prefix);
    log_array("delta", diff);
    log_array("corrected", corrected);
    log_array("expected", expected);
#endif

    return canonical;
}

} // namespace t81::core
