/**
 * @file bigint.cpp
 * @brief Implements the BigInt class methods.
 */

#include "t81/types/bigint.hpp"

#include <string>

namespace t81::core {

/**
 * @brief Converts the BigInt to its string representation.
 * @return A string representing the integer's value.
 *
 * @note Delegates to the canonical T81BigInt implementation.
 */
std::string BigInt::to_string() const { return impl_.to_string(); }

}  // namespace t81::core
