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
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

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

public:
  constexpr Cell() noexcept = default;

  // ———————— Conversion ————————
  static constexpr Cell from_int(int64_t v) {
    if (v < MIN || v > MAX) throw std::overflow_error("Cell overflow in from_int");
    Cell c;
    bool negative = v < 0;
    if (negative) v = -v;

    for (int i = 0; i < TRITS; ++i) {
      int rem = v % 3;
      if (rem == 2) {
        c.t_[i] = Trit::M;
        v = v / 3 + 1;
      } else if (rem == 1) {
        c.t_[i] = Trit::P;
        v /= 3;
      } else {
        c.t_[i] = Trit::Z;
        v /= 3;
      }
      if (v == 0) break;
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
    for (int i = 0; i < TRITS; ++i) neg.t_[i] = static_cast<Trit>(-static_cast<int8_t>(t_[i]));
    return neg;
  }

  // ———————— Addition ————————
  [[nodiscard]] constexpr Cell operator+(const Cell& o) const {
    Cell r;
    int carry = 0;

    // Unrolled 5-trit addition to avoid loop overhead and facilitate optimization
    auto add_trit = [&](int i, int& c) {
        int sum = static_cast<int>(t_[i]) + static_cast<int>(o.t_[i]) + c;
        if (sum > 1) {
            r.t_[i] = static_cast<Trit>(sum - 3);
            c = 1;
        } else if (sum < -1) {
            r.t_[i] = static_cast<Trit>(sum + 3);
            c = -1;
        } else {
            r.t_[i] = static_cast<Trit>(sum);
            c = 0;
        }
    };

    add_trit(0, carry);
    add_trit(1, carry);
    add_trit(2, carry);
    add_trit(3, carry);
    add_trit(4, carry);

    if (carry) throw std::overflow_error("Cell addition overflow");
    return r;
  }

  // ———————— Subtraction ————————
  [[nodiscard]] constexpr Cell operator-(const Cell& o) const { return *this + (-o); }

  // ———————— Multiplication (shift-and-add) ————————
  [[nodiscard]] constexpr Cell operator*(const Cell& o) const {
    Cell result;
    for (int i = 0; i < TRITS; ++i) {
      if (o.t_[i] == Trit::P) {
        Cell shifted = *this << i;
        result = result + shifted;
      } else if (o.t_[i] == Trit::M) {
        Cell shifted = *this << i;
        result = result - shifted;
      }
    }
    return result;
  }

  // ———————— Left shift (multiply by power of 3) ————————
  [[nodiscard]] constexpr Cell operator<<(int n) const {
    if (n < 0) throw std::domain_error("Negative shift");
    if (n == 0) return *this;
    if (n >= TRITS) {
        // Only allow shift if result is 0
        for (int i = 0; i < TRITS; ++i) {
            if (t_[i] != Trit::Z) throw std::overflow_error("Shift overflow (non-zero trit lost)");
        }
        return Cell();
    }
    Cell shifted;
    for (int i = TRITS - n; i < TRITS; ++i) {
        if (t_[i] != Trit::Z) throw std::overflow_error("Shift overflow (non-zero trit lost)");
    }
    for (int i = 0; i < TRITS - n; ++i) shifted.t_[i + n] = t_[i];
    return shifted;
  }

  // ———————— Division (restoring division, exact when divisible) ————————
  [[nodiscard]] constexpr Cell operator/(const Cell& divisor) const {
    int64_t a = this->to_int();
    int64_t b = divisor.to_int();
    if (b == 0) throw std::domain_error("Division by zero");
    return Cell::from_int(a / b);
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
  [[nodiscard]] constexpr bool operator==(const Cell& o) const noexcept { return t_ == o.t_; }
  [[nodiscard]] constexpr bool operator!=(const Cell& o) const noexcept { return !(*this == o); }
  [[nodiscard]] constexpr bool operator<(const Cell& o) const noexcept {
    // Little-endian: t_[4] is most significant
    for (int i = TRITS - 1; i >= 0; --i) {
        if (t_[i] < o.t_[i]) return true;
        if (t_[i] > o.t_[i]) return false;
    }
    return false;
  }
  [[nodiscard]] constexpr bool operator<=(const Cell& o) const noexcept {
    for (int i = TRITS - 1; i >= 0; --i) {
        if (t_[i] < o.t_[i]) return true;
        if (t_[i] > o.t_[i]) return false;
    }
    return true;
  }
  [[nodiscard]] constexpr bool operator>(const Cell& o) const noexcept {
    return o < *this;
  }
  [[nodiscard]] constexpr bool operator>=(const Cell& o) const noexcept {
    return o <= *this;
  }

  // ———————— Constants ————————
  static constexpr Cell zero() noexcept { return Cell(); }
  static constexpr Cell one() noexcept { return Cell::from_int(1); }
  static constexpr Cell minus_one() noexcept { return Cell::from_int(-1); }
};

}  // namespace t81::core
