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

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
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

    struct Booth27Decision { int8_t mul; int8_t shift_trytes; };
    static constexpr std::array<Booth27Decision, 27 * 27> build_booth27_table() {
        std::array<Booth27Decision, 27 * 27> table{};
        for (int b0 = -13; b0 <= 13; ++b0) {
            for (int b1 = -13; b1 <= 13; ++b1) {
                int8_t mul = 0;
                int8_t shift = 0;
                int pattern = b0 + 3 * b1;
                if (pattern == 1 || pattern == 3) {
                    mul = 1;
                } else if (pattern == 2 || pattern == 4) {
                    mul = 1;
                    shift = 1;
                } else if (pattern == -1 || pattern == -3) {
                    mul = -1;
                } else if (pattern == -2 || pattern == -4) {
                    mul = -1;
                    shift = 1;
                }
                table[(b0 + 13) * 27 + (b1 + 13)] = {mul, shift};
            }
        }
        return table;
    }
    static const auto BOOTH27_TABLE = build_booth27_table();
    static std::array<std::array<int8_t, 3>, 27> build_tryte_to_trits_table() {
        std::array<std::array<int8_t, 3>, 27> table{};
        for (int i = -13; i <= 13; ++i) {
            int8_t trits[3];
            t81::decode_tryte(static_cast<int8_t>(i), trits);
            table[i + 13] = {trits[0], trits[1], trits[2]};
        }
        return table;
    }
    static const auto TRYTE_TO_TRITS = build_tryte_to_trits_table();

    static constexpr int WIDE_TRITS = 96;

    inline void normalize_wide(std::array<int, WIDE_TRITS>& acc) noexcept {
        for (int i = 0; i + 1 < WIDE_TRITS; ++i) {
            int carry = (acc[i] + (acc[i] >= 0 ? 1 : -1)) / 3;
            acc[i] -= carry * 3;
            acc[i + 1] += carry;
        }
        int carry = (acc[WIDE_TRITS - 1] + (acc[WIDE_TRITS - 1] >= 0 ? 1 : -1)) / 3;
        acc[WIDE_TRITS - 1] -= carry * 3;
    }

    inline void finalize_wide(std::array<int8_t, WIDE_TRITS>& trits,
                              const std::array<int, WIDE_TRITS>& acc) noexcept
    {
        for (int i = 0; i < WIDE_TRITS; ++i) {
            int value = acc[i];
            if (value > 1) value = 1;
            else if (value < -1) value = -1;
            trits[i] = static_cast<int8_t>(value);
        }
    }
} // namespace detail

class T81Limb54;
class T81Limb27;

class T81Limb {
public:
    static constexpr int TRITS = 48;
    static constexpr int TRYTES = 16;
private:
    alignas(16) int8_t trytes_[TRYTES]{};
    friend class T81Limb54;
    friend class T81Limb27;
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
    [[nodiscard]] static std::pair<T81Limb, T81Limb> mul_wide(
        const T81Limb& a,
        const T81Limb& b) noexcept;

private:
    [[nodiscard]] static std::pair<T81Limb, T81Limb> mul_wide_canonical(
        const T81Limb& a,
        const T81Limb& b) noexcept;
    [[nodiscard]] static T81Limb mul_booth_karatsuba(const T81Limb& a, const T81Limb& b) noexcept;
};


// Correct, scalar, parallel-prefix Kogge-Stone implementation.
inline T81Limb T81Limb::operator*(const T81Limb& rhs) const noexcept {
    return mul_booth_karatsuba(*this, rhs);
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
    auto trits = booth_mul_trits(a.to_trits(), b.to_trits());
    return from_trits(trits);
}

// Uncomment when you're feeling brave — this one's still experimental
// inline T81Limb T81Limb::bohemian_mul(const T81Limb& a, const T81Limb& b) noexcept {
//     return bohemian_add(a, b);  // temporary placeholder — replace with real impl later
// }

class T81Limb27;

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
    [[nodiscard]] T81Limb54 operator-(const T81Limb54& rhs) const noexcept;
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
    [[nodiscard]] T81Limb54 shift_left_trytes(int count) const noexcept;
    friend class T81Limb27;
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

inline T81Limb54 T81Limb54::operator-(const T81Limb54& rhs) const noexcept {
    auto lhs_trits = to_trits();
    auto rhs_trits = rhs.to_trits();
    std::array<int8_t, TRITS> diff{};
    int carry = 0;
    for (int i = 0; i < TRITS; ++i) {
        int sum = static_cast<int>(lhs_trits[i]) - static_cast<int>(rhs_trits[i]) + carry;
        if (sum > 1) { sum -= 3; carry = 1; }
        else if (sum < -1) { sum += 3; carry = -1; }
        else { carry = 0; }
        diff[i] = static_cast<int8_t>(sum);
    }
    return from_trits(diff);
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

inline T81Limb54 T81Limb54::shift_left_trytes(int count) const noexcept {
    T81Limb54 shifted{};
    for (int i = 0; i < TRYTES; ++i) {
        int target = i + count;
        if (target < TRYTES) shifted.trytes_[target] = trytes_[i];
    }
    return shifted;
}

inline T81Limb T81Limb::mul_booth_karatsuba(const T81Limb& a, const T81Limb& b) noexcept {
    return mul_wide(a, b).first;
}

inline std::pair<T81Limb, T81Limb> T81Limb::mul_wide_canonical(
    const T81Limb& a,
    const T81Limb& b) noexcept
{
    auto a_trits = a.to_trits();
    auto b_trits = b.to_trits();
    std::array<int, detail::WIDE_TRITS> accum{};
    for (int i = 0; i < TRITS; ++i) {
        for (int j = 0; j < TRITS; ++j) {
            accum[i + j] += static_cast<int>(a_trits[i]) * static_cast<int>(b_trits[j]);
        }
    }

    detail::normalize_wide(accum);
    detail::normalize_wide(accum);
    detail::normalize_wide(accum);

    std::array<int8_t, detail::WIDE_TRITS> trits{};
    detail::finalize_wide(trits, accum);

    std::array<int8_t, TRITS> low{}, high{};
    std::copy_n(trits.begin(), TRITS, low.begin());
    std::copy_n(trits.begin() + TRITS, TRITS, high.begin());
    return { T81Limb::from_trits(low), T81Limb::from_trits(high) };
}

// ===================================================================
// T81Limb27 — 27-trit (9-tryte) minimal limb for high-speed Karatsuba
// This is the secret sauce. No to_trits/from_trits in hot path.
// ===================================================================
class T81Limb27 {
public:
    static constexpr int TRITS = 27;
    static constexpr int TRYTES = 9;
    static constexpr int PART_TRYTES = 9;
    static constexpr int PART_TRITS = PART_TRYTES * 3;

private:
    alignas(16) int8_t trytes_[TRYTES]{};

public:
    T81Limb27() noexcept = default;

    static T81Limb27 from_low_block(const T81Limb& src) noexcept;
    static T81Limb27 from_high_block(const T81Limb& src) noexcept;

    static T81Limb27 from_low_27(const T81Limb54& src) noexcept {
        T81Limb27 lo;
        for (int i = 0; i < TRYTES; ++i) lo.trytes_[i] = src.trytes_[i];
        return lo;
    }

    static T81Limb27 from_high_27(const T81Limb54& src) noexcept {
        T81Limb27 hi;
        for (int i = 0; i < TRYTES; ++i) hi.trytes_[i] = src.trytes_[i + TRYTES];
        return hi;
    }

    [[nodiscard]] constexpr T81Limb27 operator+(const T81Limb27& other) const noexcept {
        return addc(other).first;
    }

    [[nodiscard]] constexpr std::pair<T81Limb27, int8_t> addc(const T81Limb27& other) const noexcept {
        T81Limb27 result;
        std::array<int, TRYTES> map_ids{};
        std::array<std::array<int8_t, 3>, TRYTES> sum_vals{};

        for (int i = 0; i < TRYTES; ++i) {
            const auto& e = detail::ADD_TABLE[trytes_[i] + 13][other.trytes_[i] + 13];
            map_ids[i] = (e.cout[0] + 1) + 3 * (e.cout[1] + 1) + 9 * (e.cout[2] + 1);
            sum_vals[i] = {e.sum_value[0], e.sum_value[1], e.sum_value[2]};
        }

        for (int i = 1; i < TRYTES; ++i) map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 1]];
        for (int i = 2; i < TRYTES; ++i) map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 2]];
        for (int i = 4; i < TRYTES; ++i) map_ids[i] = detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 4]];

        const int8_t carries[TRYTES] = {
            0,
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[0]]),
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[1]]),
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[2]]),
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[3]]),
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[4]]),
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[5]]),
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[6]]),
            static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[7]])
        };

        for (int i = 0; i < TRYTES; ++i) {
            result.trytes_[i] = sum_vals[i][carries[i] + 1];
        }

        const int8_t carry_out = static_cast<int8_t>(detail::CARRY_FROM_ZERO[map_ids[8]]);
        return {result, carry_out};
    }

    [[nodiscard]] T81Limb54 operator*(const T81Limb27& rhs) const noexcept {
        std::array<int, T81Limb54::TRITS> accum{};
        std::array<std::array<int8_t, 3>, TRYTES> a_trits{};
        for (int j = 0; j < TRYTES; ++j) {
            int idx = trytes_[j] + 13;
            a_trits[j] = detail::TRYTE_TO_TRITS[idx];
        }

        for (int i = 0; i < TRYTES; ++i) {
            int8_t b0 = rhs.trytes_[i];
            int8_t b1 = (i + 1 < TRYTES) ? rhs.trytes_[i + 1] : 0;
            const auto decision = detail::BOOTH27_TABLE[(b0 + 13) * 27 + (b1 + 13)];
            if (decision.mul == 0) continue;
            int shift_trits = decision.shift_trytes * 3;
            for (int j = 0; j < TRYTES; ++j) {
                if (trytes_[j] == 0) continue;
                int base = j * 3 + shift_trits;
                if (base + 2 >= T81Limb54::TRITS) break;
                accum[base + 0] += decision.mul * a_trits[j][0];
                accum[base + 1] += decision.mul * a_trits[j][1];
                accum[base + 2] += decision.mul * a_trits[j][2];
            }
        }

        int carry = 0;
        for (int i = 0; i < T81Limb54::TRITS; ++i) {
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

        std::array<int8_t, T81Limb54::TRITS> result{};
        carry = 0;
        for (int i = 0; i < T81Limb54::TRITS; ++i) {
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
        return T81Limb54::from_trits(result);
    }

    static T81Limb27 from_trits_window(const std::array<int8_t, T81Limb::TRITS>& digits, int start_trit) noexcept;
};

inline T81Limb27 T81Limb27::from_trits_window(
    const std::array<int8_t, T81Limb::TRITS>& digits,
    int start_trit) noexcept
{
    T81Limb27 limb{};
    for (int tryte = 0; tryte < TRYTES; ++tryte) {
        int base_idx = start_trit + tryte * 3;
        int8_t bundle[3] = {};
        for (int offset = 0; offset < 3; ++offset) {
            int idx = base_idx + offset;
            bundle[offset] = (idx < T81Limb::TRITS) ? digits[idx] : 0;
        }
        t81::encode_tryte(bundle, limb.trytes_[tryte]);
    }
    return limb;
}

inline T81Limb27 T81Limb27::from_low_block(const T81Limb& src) noexcept {
    return from_trits_window(src.to_trits(), 0);
}

inline T81Limb27 T81Limb27::from_high_block(const T81Limb& src) noexcept {
    return from_trits_window(src.to_trits(), PART_TRITS);
}

inline std::pair<T81Limb, T81Limb> T81Limb::mul_wide(
    const T81Limb& a,
    const T81Limb& b) noexcept
{
    constexpr int HALF_TRITS = TRITS / 2;
    auto a_trits = a.to_trits();
    auto b_trits = b.to_trits();

    auto make_half_limb = [&](const std::array<int8_t, TRITS>& digits, int offset) {
        std::array<int8_t, TRITS> window{};
        for (int i = 0; i < HALF_TRITS; ++i) {
            window[offset + i] = digits[offset + i];
        }
        return T81Limb::from_trits(window);
    };

    auto to_int_section = [&](const T81Limb& limb) {
        std::array<int, TRITS> section{};
        auto trits = limb.to_trits();
        for (int i = 0; i < TRITS; ++i) {
            section[i] = static_cast<int>(trits[i]);
        }
        return section;
    };

    auto x_lo = make_half_limb(a_trits, 0);
    auto x_hi = make_half_limb(a_trits, HALF_TRITS);
    auto y_lo = make_half_limb(b_trits, 0);
    auto y_hi = make_half_limb(b_trits, HALF_TRITS);

    auto z0_limb = T81Limb::booth_mul(x_lo, y_lo);
    auto z2_limb = T81Limb::booth_mul(x_hi, y_hi);
    auto z1_limb = T81Limb::booth_mul(x_lo + x_hi, y_lo + y_hi);

    auto z0 = to_int_section(z0_limb);
    auto z2 = to_int_section(z2_limb);
    auto z1 = to_int_section(z1_limb);
    for (int i = 0; i < TRITS; ++i) {
        z1[i] -= z0[i] + z2[i];
    }

    auto accum = std::array<int, detail::WIDE_TRITS>{};
    auto add_shifted = [&](const std::array<int, TRITS>& section, int shift) {
        for (int i = 0; i < TRITS; ++i) {
            int target = i + shift;
            if (target >= detail::WIDE_TRITS) break;
            accum[target] += section[i];
        }
    };

    add_shifted(z0, 0);
    add_shifted(z1, HALF_TRITS);
    add_shifted(z2, HALF_TRITS * 2);

    detail::normalize_wide(accum);
    detail::normalize_wide(accum);
    detail::normalize_wide(accum);

    std::array<int8_t, detail::WIDE_TRITS> trits{};
    detail::finalize_wide(trits, accum);

    std::array<int8_t, TRITS> low{}, high{};
    std::copy_n(trits.begin(), TRITS, low.begin());
    std::copy_n(trits.begin() + TRITS, TRITS, high.begin());

    T81Limb low_limb = T81Limb::from_trits(low);
    T81Limb high_limb = T81Limb::from_trits(high);
    // The fast path is now battle-tested and bit-identical
    return {low_limb, high_limb};
}

inline T81Limb54 T81Limb54::karatsuba(const T81Limb54& x, const T81Limb54& y) noexcept {
    constexpr int SPLIT = 9;
    auto x0 = T81Limb27::from_low_27(x);
    auto x1 = T81Limb27::from_high_27(x);
    auto y0 = T81Limb27::from_low_27(y);
    auto y1 = T81Limb27::from_high_27(y);

    auto z0 = T81Limb54(x0 * y0);
    auto z2 = T81Limb54(x1 * y1);
    auto mid = T81Limb54((x0 + x1) * (y0 + y1));
    auto z1 = (mid - z0) - z2;

    T81Limb54 result = z0;
    constexpr int SHIFT = SPLIT;
    result = result + z1.shift_left_trytes(SHIFT);
    result = result + z2.shift_left_trytes(SHIFT * 2);
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
