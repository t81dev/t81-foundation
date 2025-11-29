/**
 * @file fraction.hpp
 * @brief Defines the Fraction struct for representing rational numbers.
 *
 * This file provides a simple struct, `Fraction`, for representing rational
 * numbers. It uses the `BigInt` type for both the numerator and the denominator,
 * allowing for arbitrary-precision fractional values.
 */
#pragma once

#include <string>
#include "t81/core/bigint.hpp"

namespace t81::core {
// Rational number representation per spec/t81-data-types.md.
struct Fraction {
  BigInt numerator{};
  BigInt denominator{1};

  [[nodiscard]] std::string to_string() const;
};
}  // namespace t81::core
