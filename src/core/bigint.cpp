/**
 * @file bigint.cpp
 * @brief Implements the BigInt class methods.
 */

#include "t81/core/bigint.hpp"

#include <string>

namespace t81::core {

/**
 * @brief Converts the BigInt to its string representation.
 * @return A string representing the integer's value.
 *
 * @note This is a temporary implementation that relies on std::to_string.
 *       The final version will handle arbitrary-precision integers.
 */
std::string BigInt::to_string() const { return std::to_string(value_); }

}  // namespace t81::core
