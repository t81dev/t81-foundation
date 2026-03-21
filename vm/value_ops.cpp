#include "internal/value_ops.hpp"

namespace t81::vm::internal {

std::int64_t add_int(std::int64_t lhs, std::int64_t rhs) { return lhs + rhs; }

std::int64_t sub_int(std::int64_t lhs, std::int64_t rhs) { return lhs - rhs; }

std::int64_t mul_int(std::int64_t lhs, std::int64_t rhs) { return lhs * rhs; }

std::int64_t neg_int(std::int64_t value) { return -value; }

bool div_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  *out = lhs / rhs;
  return true;
}

bool mod_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out) {
  if (rhs == 0 || out == nullptr) return false;
  *out = lhs % rhs;
  return true;
}

}  // namespace t81::vm::internal
