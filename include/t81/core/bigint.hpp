/**
 * @file bigint.hpp
 * @brief Defines the BigInt class for arbitrary-precision integers.
 */

#pragma once

#include <cstdint>
#include <string>

namespace t81::core {

/**
 * @class BigInt
 * @brief Represents a canonical arbitrary-precision integer.
 *
 * This class provides a foundational data type for handling integers that may
 * exceed the limits of standard primitive types. It is used throughout the
 * T81 ecosystem for both numerical calculations and data representation.
 *
 * @note This is a placeholder implementation. The full, specification-compliant
 *       BigInt is defined in `spec/t81-data-types.md`.
 */
class BigInt {
 public:
  /**
   * @brief Default constructor, initializes the BigInt to zero.
   */
  BigInt() = default;

  /**
   * @brief Constructs a BigInt from a 64-bit signed integer.
   * @param v The initial value.
   */
  explicit BigInt(std::int64_t v) : value_(v) {}

  /**
   * @brief Retrieves the underlying value.
   * @return The 64-bit integer value.
   * @note This is a temporary method for the placeholder implementation.
   */
  [[nodiscard]] std::int64_t value() const noexcept { return value_; }

  /**
   * @brief Converts the BigInt to its string representation.
   * @return A string representing the integer's value.
   */
  [[nodiscard]] std::string to_string() const;

 private:
  std::int64_t value_{0};
};

}  // namespace t81::core
