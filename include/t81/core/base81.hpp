#pragma once

#include <string>
#include <string_view>

namespace t81::core {
// Base-81 textual representation used throughout the platform. See spec/t81-data-types.md.
using Base81String = std::string;

inline bool is_base81(std::string_view value) {
  // Placeholder validation; TODO: enforce full alphabet per spec/t81-data-types.md.
  return !value.empty();
}
}  // namespace t81::core

