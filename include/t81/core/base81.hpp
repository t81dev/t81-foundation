#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "t81/codec/base81.hpp"

namespace t81::core {
// Base-81 textual representation used throughout the platform. See spec/t81-data-types.md.
using Base81String = std::string;

inline bool is_base81(std::string_view value) {
  // Decode using the canonical alphabet; failure indicates non-canonical input.
  std::vector<std::uint8_t> sink;
  return t81::codec::base81::decode_bytes(value, sink);
}
}  // namespace t81::core
