#include "internal/value_ops.hpp"
#include <limits>

namespace t81::vm::internal {

std::int64_t add_int(std::int64_t lhs, std::int64_t rhs) { return lhs + rhs; }

std::int64_t sub_int(std::int64_t lhs, std::int64_t rhs) { return lhs - rhs; }

std::int64_t mul_int(std::int64_t lhs, std::int64_t rhs) { return lhs * rhs; }

std::int64_t neg_int(std::int64_t value) {
  if (value == std::numeric_limits<std::int64_t>::min()) {
    return std::numeric_limits<std::int64_t>::max();
  }
  return -value;
}

bool div_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) return false;
  *out = lhs / rhs;
  return true;
}

bool mod_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  if (lhs == std::numeric_limits<std::int64_t>::min() && rhs == -1) {
    *out = 0;
    return true;
  }
  *out = lhs % rhs;
  return true;
}

}  // namespace t81::vm::internal
