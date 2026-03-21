#pragma once

#include <cstdint>

namespace t81::vm::internal {

std::int64_t add_int(std::int64_t lhs, std::int64_t rhs);
std::int64_t sub_int(std::int64_t lhs, std::int64_t rhs);
std::int64_t mul_int(std::int64_t lhs, std::int64_t rhs);
std::int64_t neg_int(std::int64_t value);

bool div_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out);
bool mod_int(std::int64_t lhs, std::int64_t rhs, std::int64_t* out);

}  // namespace t81::vm::internal
