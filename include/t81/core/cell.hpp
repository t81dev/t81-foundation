/**
 * @file cell.hpp
 * @brief Defines the Cell class, a 5-trit balanced ternary cell.
 *
 * This file provides the Cell class, a fundamental numeric type representing a
 * 5-trit balanced ternary cell. It has a symmetric value range of -121 to +121
 * and supports a rich set of constexpr-friendly arithmetic and comparison
 * operations, forming a basic building block for more complex ternary-native
 * data structures.
 */
// T81 Foundation — Real Balanced Ternary Cell (5 trits, -121..+121)
// v1.0.0-SOVEREIGN — The recursion now converges on truth.
// License: MIT / GPL-3.0 dual

#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <utility>
#include <algorithm>
#include <cmath>

namespace t81::core {

enum class Trit : int8_t { M = -1, Z = 0, P = +1 };

// 5-trit balanced ternary cell: 3⁵ = 243 states → symmetric range [-121, +121]
class Cell {
public:
    static constexpr int TRITS = 5;
    static constexpr int64_t MIN = -121;
    static constexpr int64_t MAX = +121;

private:
    using Index = std::uint8_t;

    static constexpr Index encode_trits(const std::array<Trit, TRITS>& trits) noexcept {
        Index idx = 0;
        Index mul = 1;
        for (int i = 0; i < TRITS; ++i) {
            idx += static_cast<Index>(static_cast<int>(trits[i]) + 1) * mul;
            mul *= 3;
        }
        return idx;
    }

    static constexpr std::array<Trit, TRITS> decode_trits(Index idx) noexcept {
        std::array<Trit, TRITS> trits{};
        for (int i = 0; i < TRITS; ++i) {
            int digit = idx % 3;
            trits[i] = static_cast<Trit>(digit - 1);
            idx /= 3;
        }
        return trits;
    }

    static constexpr Index build_index_from_value(int64_t v) noexcept {
        std::array<Trit, TRITS> trits{};
        int64_t remainder = v;
        bool negative = remainder < 0;
        if (negative) remainder = -remainder;
        int i = 0;
        while (remainder != 0 && i < TRITS) {
            int rem = static_cast<int>(remainder % 3);
            if (rem == 2) {
                trits[i++] = Trit::M;
                remainder = remainder / 3 + 1;
            } else {
                trits[i++] = static_cast<Trit>(rem - 1);
                remainder /= 3;
            }
        }
        if (negative) {
            for (auto& t : trits) {
                t = static_cast<Trit>(-static_cast<int>(t));
            }
        }
        return encode_trits(trits);
    }

    static constexpr std::array<std::array<Trit, TRITS>, MAX - MIN + 1> build_trits_table() noexcept {
        std::array<std::array<Trit, TRITS>, MAX - MIN + 1> table{};
        for (int64_t v = MIN; v <= MAX; ++v) {
            Index idx = build_index_from_value(v);
            table[static_cast<size_t>(v - MIN)] = decode_trits(idx);
        }
        return table;
    }

    static constexpr std::array<int64_t, MAX - MIN + 1> build_value_table() noexcept {
        std::array<int64_t, MAX - MIN + 1> table{};
        for (int64_t v = MIN; v <= MAX; ++v) {
            table[static_cast<size_t>(v - MIN)] = v;
        }
        return table;
    }

    static constexpr std::array<Index, MAX - MIN + 1> build_index_lookup() noexcept {
        std::array<Index, MAX - MIN + 1> table{};
        for (int64_t v = MIN; v <= MAX; ++v) {
            table[static_cast<size_t>(v - MIN)] = build_index_from_value(v);
        }
        return table;
    }

    static constexpr std::array<Index, MAX - MIN + 1> build_neg_table() noexcept {
        std::array<Index, MAX - MIN + 1> table{};
        for (int64_t v = MIN; v <= MAX; ++v) {
            auto trits = build_trits_table()[static_cast<size_t>(v - MIN)];
            for (auto& t : trits) {
                t = static_cast<Trit>(-static_cast<int>(t));
            }
            table[static_cast<size_t>(v - MIN)] = encode_trits(trits);
        }
        return table;
    }

    static inline const std::array<std::array<Trit, TRITS>, MAX - MIN + 1> k_trits_table = build_trits_table();
    static inline const std::array<int64_t, MAX - MIN + 1> k_value_table = build_value_table();
    static inline const std::array<Index, MAX - MIN + 1> k_index_lookup = build_index_lookup();
    static inline const std::array<Index, MAX - MIN + 1> k_neg_table = build_neg_table();

    Index idx_ = k_index_lookup[static_cast<size_t>(0 - MIN)];

public:
    constexpr Cell() noexcept = default;

    // ———————— Conversion ————————
    static constexpr Cell from_int(int64_t v) {
        if (v < MIN || v > MAX) throw std::overflow_error("Cell overflow in from_int");
        return k_int_lookup[static_cast<size_t>(v - MIN)];
    }

private:
    static Cell encode_value(int64_t v) noexcept {
        Cell c;
        bool negative = v < 0;
        if (negative) v = -v;
        for (int i = 0; v != 0 && i < TRITS; ++i) {
            int rem = static_cast<int>(v % 3);
            if (rem == 2) {
                c.t_[i] = Trit::M;
                v = v / 3 + 1;
            } else {
                c.t_[i] = static_cast<Trit>(rem - 1);
                v /= 3;
            }
        }
        if (negative) {
            c = -c;
        }
        return c;
    }

    static const std::array<Cell, MAX - MIN + 1> k_int_lookup;

public:
    [[nodiscard]] constexpr int64_t to_int() const noexcept {
        int64_t val = 0;
        for (int i = TRITS - 1; i >= 0; --i) {
            int8_t tv = static_cast<int8_t>(t_[i]);
            val = val * 3 + (tv > 0 ? 1 : (tv < 0 ? -1 : 0));
        }
        return val;
    }

    // ———————— Unary Operators ————————
    [[nodiscard]] constexpr Cell operator-() const noexcept {
        Cell neg;
        for (auto i = 0; i < TRITS; ++i) {
            auto value = static_cast<int8_t>(t_[i]);
            neg.t_[i] = static_cast<Trit>(-value);
        }
        return neg;
    }

    // ———————— Addition ————————
    [[nodiscard]] constexpr Cell operator+(const Cell& o) const {
        Cell r;
        int carry = 0;
        int idx = 0;
        while (idx < TRITS) {
            int sum = static_cast<int>(t_[idx]) + static_cast<int>(o.t_[idx]) + carry;
            if (carry == 0 && sum >= -1 && sum <= 1) {
                r.t_[idx++] = static_cast<Trit>(sum);
                // carry-skip: copy a run while the pairwise sum stays within [-1, 1]
                while (idx < TRITS) {
                    int lookahead = static_cast<int>(t_[idx]) + static_cast<int>(o.t_[idx]);
                    if (lookahead < -1 || lookahead > 1) break;
                    r.t_[idx++] = static_cast<Trit>(lookahead);
                }
                continue;
            }
            int next_carry = 0;
            int adjusted = sum;
            if (adjusted > 1) {
                adjusted -= 3;
                next_carry = 1;
            } else if (adjusted < -1) {
                adjusted += 3;
                next_carry = -1;
            }
            r.t_[idx++] = static_cast<Trit>(adjusted);
            carry = next_carry;
        }
        if (carry != 0) throw std::overflow_error("Cell addition overflow");
        return r;
    }

    // ———————— Subtraction ————————
    [[nodiscard]] constexpr Cell operator-(const Cell& o) const { return *this + (-o); }

    // ———————— Multiplication (shift-and-add) ————————
    [[nodiscard]] constexpr Cell operator*(const Cell& o) const {
        Cell result;
        for (int i = 0; i < TRITS; ++i) {
            if (o.t_[i] == Trit::P)      result = result + (*this << i);
            else if (o.t_[i] == Trit::M) result = result - (*this << i);
        }
        return result;
    }

    // ———————— Left shift (multiply by power of 3) ————————
    [[nodiscard]] constexpr Cell operator<<(int n) const {
        if (n < 0) throw std::domain_error("Negative shift");
        if (n >= TRITS) throw std::overflow_error("Shift overflow");
        Cell shifted;
        for (int i = 0; i < TRITS - n; ++i)
            shifted.t_[i + n] = t_[i];
        // lower n trits remain Z → no need to clear
        return shifted;
    }

    // ———————— Division (restoring division, exact when divisible) ————————
    [[nodiscard]] constexpr Cell operator/(const Cell& divisor) const {
        if (divisor == Cell::from_int(0)) throw std::domain_error("Division by zero");
        Cell quotient;
        Cell remainder = *this;
        Cell abs_div = divisor.to_int() < 0 ? -divisor : divisor;

        // Simple long division — works because range is tiny (243 states)
        for (int i = TRITS -1; i >= 0; --i) {
            Cell candidate = abs_div << i;
            if (candidate.to_int() <= std::abs(remainder.to_int())) {
                quotient = quotient + (Cell::from_int(1) << i);
                remainder = remainder - (divisor.to_int() < 0 ? -candidate : candidate);
            }
        }
        if (this->to_int() < 0 != divisor.to_int() < 0) quotient = -quotient;
        return quotient;
    }

    // ———————— Modulo ————————
    [[nodiscard]] constexpr Cell operator%(const Cell& divisor) const {
        Cell q = *this / divisor;
        return *this - q * divisor;
    }

    // ———————— GCD (Euclidean algorithm) ————————
    [[nodiscard]] friend constexpr Cell gcd(Cell a, Cell b) {
        a = a.to_int() < 0 ? -a : a;
        b = b.to_int() < 0 ? -b : b;
        while (b != Cell::from_int(0)) {
            Cell t = b;
            b = a % b;
            a = t;
        }
        return a;
    }

    // ———————— Comparison ————————
    [[nodiscard]] constexpr bool operator==(const Cell& o) const noexcept {
        return t_ == o.t_;
    }
    [[nodiscard]] constexpr bool operator!=(const Cell& o) const noexcept { return !(*this == o); }
    [[nodiscard]] constexpr bool operator<(const Cell& o) const noexcept { return to_int() < o.to_int(); }
    [[nodiscard]] constexpr bool operator<=(const Cell& o) const noexcept { return to_int() <= o.to_int(); }
    [[nodiscard]] constexpr bool operator>(const Cell& o) const noexcept { return to_int() > o.to_int(); }
    [[nodiscard]] constexpr bool operator>=(const Cell& o) const noexcept { return to_int() >= o.to_int(); }

    // ———————— Constants ————————
    static constexpr Cell zero()   noexcept { return Cell(); }
    static constexpr Cell one()    noexcept { return Cell::from_int(1); }
    static constexpr Cell minus_one() noexcept { return Cell::from_int(-1); }
};

inline const std::array<Cell, Cell::MAX - Cell::MIN + 1> Cell::k_int_lookup = []() {
    std::array<Cell, Cell::MAX - Cell::MIN + 1> table{};
    for (int64_t v = Cell::MIN; v <= Cell::MAX; ++v) {
        table[static_cast<size_t>(v - Cell::MIN)] = Cell::encode_value(v);
    }
    return table;
}();

} // namespace t81::core
