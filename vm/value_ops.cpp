#include "internal/value_ops.hpp"
#include <cstdint>
#include <limits>

namespace t81::vm::internal {

std::int64_t add_int(std::int64_t lhs, std::int64_t rhs) {
  return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) +
                                   static_cast<std::uint64_t>(rhs));
}

std::int64_t sub_int(std::int64_t lhs, std::int64_t rhs) {
  return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) -
                                   static_cast<std::uint64_t>(rhs));
}

std::int64_t mul_int(std::int64_t lhs, std::int64_t rhs) {
  return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) *
                                   static_cast<std::uint64_t>(rhs));
}

std::int64_t neg_int(std::int64_t value) {
  // Standard two's-complement negation: -INT64_MIN wraps to INT64_MIN without UB.
  return static_cast<std::int64_t>(std::uint64_t{0} - static_cast<std::uint64_t>(value));
}

bool div_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  // INT64_MIN / -1 is C++ undefined behavior and may cause a host exception (x86 IDIV #DE).
  if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) return false;
  *out = lhs / rhs;
  return true;
}

bool mod_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  // INT64_MIN % -1 is C++ undefined behavior even though the mathematical remainder is zero.
  if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) {
    *out = 0;
    return true;
  }
  *out = lhs % rhs;
  return true;
}

}  // namespace t81::vm::internal
