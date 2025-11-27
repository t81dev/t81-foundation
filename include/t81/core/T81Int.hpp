/**
 * @file T81Int.hpp
 * @brief Defines the T81Int class for fixed-precision ternary integers.
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <limits>

namespace t81::core {

// Represents a single balanced ternary digit: -1, 0, or +1.
enum class Trit : int8_t {
  N = -1,  // Negative
  Z = 0,   // Zero
  P = 1,   // Positive
};

template <size_t N>
class T81Int;

template <size_t N>
T81Int<N> operator+(const T81Int<N>& lhs, const T81Int<N>& rhs);

template <size_t N>
T81Int<N> operator-(const T81Int<N>& lhs, const T81Int<N>& rhs);

template <size_t N>
T81Int<N> operator*(const T81Int<N>& lhs, const T81Int<N>& rhs);

/**
 * @class T81Int
 * @brief Represents a fixed-precision, balanced ternary integer.
 *
 * This class provides a header-only, templated implementation of a base-81
 * integer type using balanced ternary encoding. The template parameter N
 * specifies the number of trits for the integer's precision.
 *
 * @tparam N The number of trits.
 */
template <size_t N>
class T81Int {
  friend T81Int<N> operator+<>(const T81Int<N>& lhs, const T81Int<N>& rhs);
  friend T81Int<N> operator-<>(const T81Int<N>& lhs, const T81Int<N>& rhs);
  friend T81Int<N> operator*<>(const T81Int<N>& lhs, const T81Int<N>& rhs);

 public:
  /**
   * @brief Default constructor, initializes the integer to zero.
   */
  T81Int() { _trytes.fill(zero_tryte()); }

  /**
   * @brief Constructs a T81Int from a 64-bit signed integer.
   * @param value The initial value.
   */
  explicit T81Int(std::int64_t value) {
    using BigT = __int128;
    _trytes.fill(zero_tryte());
    BigT val = value;
    for (size_t i = 0; i < N && val != 0; ++i) {
      int remainder = val % 3;
      val /= 3;
      if (remainder == 2) {
        set_trit(i, Trit::N);
        val++;
      } else if (remainder == -2) {
        set_trit(i, Trit::P);
        val--;
      } else {
        set_trit(i, static_cast<Trit>(remainder));
      }
    }
  }

  /**
   * @brief Performs unary negation.
   * @return A new T81Int with the negated value.
   */
  T81Int operator-() const {
    T81Int result;
    for (size_t i = 0; i < N; ++i) {
      Trit current_trit = get_trit(i);
      result.set_trit(i, static_cast<Trit>(-static_cast<int8_t>(current_trit)));
    }
    return result;
  }

  /**
   * @brief Converts the T81Int to a standard binary integer type.
   * @tparam T The target integer type (e.g., int64_t).
   * @return The value converted to the target type.
   */
  template <typename T>
  T to_binary() const {
    using BigT = __int128;  // Wider type; assumes compiler support (GCC/Clang).
    static_assert(std::is_integral_v<T>, "T must be an integral type");

    BigT result = 0;
    for (size_t i = N; i-- > 0;) {
        result = result * 3 + static_cast<int8_t>(get_trit(i));
    }

    if (result > static_cast<BigT>(std::numeric_limits<T>::max()) ||
        result < static_cast<BigT>(std::numeric_limits<T>::min())) {
        throw std::overflow_error("T81Int to_binary conversion out of range");
    }

    return static_cast<T>(result);
  }

  /**
   * @brief Converts the T81Int to a string of trits for debugging.
   * @return A string representation (e.g., "+0-").
   */
  std::string str() const {
    std::string s = "";
    for (size_t i = 0; i < N; ++i) {
      Trit trit = get_trit(N - 1 - i);
      if (trit == Trit::P) {
        s += '+';
      } else if (trit == Trit::Z) {
        s += '0';
      } else {
        s += '-';
      }
    }
    return s;
  }

 private:
  // A tryte is a group of 4 trits, representing values from -40 to +40.
  // We store N trits in a compact array of bytes (trytes).
  static constexpr size_t num_trytes = (N + 3) / 4;
  std::array<uint8_t, num_trytes> _trytes;

  // Compile-time lookup table for powers of 3.
  static constexpr std::array<int, 5> powers_of_3 = {1, 3, 9, 27, 81};

  /**
   * @brief Returns the byte representation of a zero tryte.
   * @return The uint8_t value for a zero tryte.
   */
  static constexpr uint8_t zero_tryte() {
    return powers_of_3[0] + powers_of_3[1] + powers_of_3[2] + powers_of_3[3]; // 40
  }

  /**
   * @brief Gets the value of the trit at a given position.
   * @param index The index of the trit to retrieve (0 to N-1).
   * @return The Trit value at the specified index.
   */
  Trit get_trit(size_t index) const {
    if (index >= N) {
      // Out of bounds, return zero (sign extension)
      return Trit::Z;
    }

    const size_t tryte_index = index / 4;
    const size_t trit_pos_in_tryte = index % 4;

    const uint8_t byte_val = _trytes[tryte_index];
    const int unbalanced_trit = (byte_val / powers_of_3[trit_pos_in_tryte]) % 3;

    return static_cast<Trit>(unbalanced_trit - 1);
  }

  /**
   * @brief Sets the value of the trit at a given position.
   * @param index The index of the trit to set (0 to N-1).
   * @param value The new Trit value.
   */
  void set_trit(size_t index, Trit value) {
    if (index >= N) {
      // Out of bounds, do nothing.
      return;
    }

    const size_t tryte_index = index / 4;
    const size_t trit_pos_in_tryte = index % 4;
    const int power_of_3 = powers_of_3[trit_pos_in_tryte];

    uint8_t byte_val = _trytes[tryte_index];

    // Clear the old trit's value
    const int old_unbalanced_trit = (byte_val / power_of_3) % 3;
    byte_val -= old_unbalanced_trit * power_of_3;

    // Add the new trit's value
    const int new_unbalanced_trit = static_cast<int8_t>(value) + 1;
    byte_val += new_unbalanced_trit * power_of_3;

    _trytes[tryte_index] = byte_val;
  }

  /**
   * @brief Constructs a T81Int from a raw tryte array.
   * @param trytes The array of trytes.
   */
  explicit T81Int(const std::array<uint8_t, num_trytes>& trytes) : _trytes(trytes) {}
};

/**
 * @brief Performs addition of two T81Int numbers.
 * @tparam N The number of trits.
 * @param lhs The left-hand operand.
 * @param rhs The right-hand operand.
 * @return The sum of the two numbers.
 */
template <size_t N>
T81Int<N> operator+(const T81Int<N>& lhs, const T81Int<N>& rhs) {
  // Lookup table for balanced ternary addition.
  // Index is sum + 3. Value is {result_trit, carry_trit}.
  constexpr static std::array<std::pair<Trit, Trit>, 7> addition_table = {{
      {Trit::Z, Trit::N}, // sum = -3
      {Trit::P, Trit::N}, // sum = -2
      {Trit::N, Trit::Z}, // sum = -1
      {Trit::Z, Trit::Z}, // sum = 0
      {Trit::P, Trit::Z}, // sum = 1
      {Trit::N, Trit::P}, // sum = 2
      {Trit::Z, Trit::P}, // sum = 3
  }};

  Trit carry = Trit::Z;
  T81Int<N> result;

  for (size_t i = 0; i < N; ++i) {
    int sum = static_cast<int8_t>(lhs.get_trit(i)) +
              static_cast<int8_t>(rhs.get_trit(i)) +
              static_cast<int8_t>(carry);

    const auto& [result_trit, new_carry] = addition_table[sum + 3];
    result.set_trit(i, result_trit);
    carry = new_carry;
  }
  // Note: Overflow carry is discarded as this is a fixed-precision integer.
  return result;
}

/**
 * @brief Performs subtraction of two T81Int numbers.
 * @tparam N The number of trits.
 * @param lhs The left-hand operand.
 * @param rhs The right-hand operand.
 * @return The difference of the two numbers.
 */
template <size_t N>
T81Int<N> operator-(const T81Int<N>& lhs, const T81Int<N>& rhs) {
  return lhs + (-rhs);
}

/**
 * @brief Performs multiplication of two T81Int numbers.
 * @tparam N The number of trits.
 * @param lhs The left-hand operand.
 * @param rhs The right-hand operand.
 * @return The product of the two numbers..
 */
template <size_t N>
T81Int<N> operator*(const T81Int<N>& lhs, const T81Int<N>& rhs) {
  T81Int<N> result;
  for (size_t i = 0; i < N; ++i) {
    Trit rhs_trit = rhs.get_trit(i);
    if (rhs_trit == Trit::Z) {
      continue;
    }

    T81Int<N> shifted_lhs;
    for (size_t j = 0; j < N - i; ++j) {
      shifted_lhs.set_trit(j + i, lhs.get_trit(j));
    }

    if (rhs_trit == Trit::P) {
      result = result + shifted_lhs;
    } else { // rhs_trit == Trit::N
      result = result - shifted_lhs;
    }
  }
  return result;
}

}  // namespace t81::core
