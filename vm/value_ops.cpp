#include "internal/value_ops.hpp"
#include <limits>

namespace t81::vm::internal {

std::int64_t add_int(std::int64_t lhs, std::int64_t rhs) {
  return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) + static_cast<std::uint64_t>(rhs));
}

std::int64_t sub_int(std::int64_t lhs, std::int64_t rhs) {
  return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) - static_cast<std::uint64_t>(rhs));
}

std::int64_t mul_int(std::int64_t lhs, std::int64_t rhs) {
  return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) * static_cast<std::uint64_t>(rhs));
}

std::int64_t neg_int(std::int64_t value) {
  // Standard two's-complement negation: -INT64_MIN wraps to INT64_MIN without UB
  return static_cast<std::int64_t>(-static_cast<std::uint64_t>(value));
}

bool div_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  // In two's complement, INT64_MIN / -1 overflows (+2^63) and triggers #DE / SIGFPE
  if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) return false;
  *out = lhs / rhs;
  return true;
}

bool mod_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  // In two's complement, INT64_MIN % -1 triggers #DE on x86 idiv even though mathematically r = 0
  if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) {
    *out = 0;
    return true;
  }
  *out = lhs % rhs;
  return true;
}

}  // namespace t81::vm::internal
