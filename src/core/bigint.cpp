#include "t81/core/bigint.hpp"

#include <string>

namespace t81::core {
std::string BigInt::to_string() const { return std::to_string(value_); }
}  // namespace t81::core

