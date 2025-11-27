// include/t81/core/cell.hpp
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
    std::array<Trit, TRITS> t_{};  // little-endian: t_[0] = least significant trit

    // Internal: convert a single decimal digit (0,1,2) → balanced trit + carry
    static constexpr std::pair<Trit, int> digit_to_trit(int d) noexcept {
        if (d == 0) return {Trit::Z, 0};
        if (d == 1) return {Trit::P, 0};
        if (d == 2) return {Trit::M, 1};   // 2 → -1 + carry 1
        return {Trit::Z, 0};               // unreachable
    }

public:
    constexpr Cell() noexcept = default;

    // ———————— Conversion ————————
    static constexpr Cell from_int(int64_t v) {
        if (v < MIN || v > MAX) throw std::overflow_error("Cell overflow in from_int");
        Cell c;
        bool negative = v < 0;
        if (negative) v = -v;

        for (int i = 0; v != 0 && i < TRITS; ++i) {
            int rem = v % 3;
            if (rem == 2) {
                c.t_[i] = Trit::M;
                v = v / 3 + 1;
            } else {
                c.t_[i] = static_cast<Trit>(rem - 1);  // 0→Z, 1→P
                v /= 3;
            }
        }
        if (negative) c = -c;
        return c;
    }

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
        for (int i = 0; i < TRITS; ++i)
            neg.t_[i] = static_cast<Trit>(-static_cast<int8_t>(t_[i]));
        return neg;
    }

    // ———————— Addition ————————
    [[nodiscard]] constexpr Cell operator+(const Cell& o) const {
        Cell r;
        int carry = 0;
        for (int i = 0; i < TRITS; ++i) {
            int sum = static_cast<int>(t_[i]) + static_cast<int>(o.t_[i]) + carry;
            if (sum == 3)      { r.t_[i] = Trit::P; carry =  1; }
            else if (sum == -3){ r.t_[i] = Trit::M; carry = -1; }
            else if (sum == 2) { r.t_[i] = Trit::M; carry =  1; }
            else if (sum == -2){ r.t_[i] = Trit::P; carry = -1; }
            else               { r.t_[i] = static_cast<Trit>(sum); carry = 0; }
        }
        if (carry) throw std::overflow_error("Cell addition overflow");
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

} // namespace t81::core
