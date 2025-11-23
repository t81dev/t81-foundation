#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "t81/codec/base81.hpp"

namespace t81::hash {

// -----------------------------------------------------------------------------
// Base-81 codec (canonical Unicode alphabet, spec/v1.1.0-canonical.md)
// -----------------------------------------------------------------------------
//   - encode_base81: bytes -> base-81 string using canonical alphabet
//   - decode_base81: base-81 string -> bytes
//   - Backwards compatibility: strings starting with "b81:" still decode via
//     the legacy hex stub.
// -----------------------------------------------------------------------------

inline std::string encode_base81(const std::vector<uint8_t>& bytes) {
  return t81::codec::base81::encode_bytes(bytes);
}

inline std::vector<uint8_t> decode_base81(const std::string& s) {
  if (s.rfind("b81:", 0) == 0) {
    throw std::invalid_argument("decode_base81: legacy b81: prefix rejected");
  }
  std::vector<uint8_t> out;
  if (!t81::codec::base81::decode_bytes(s, out)) {
    throw std::invalid_argument("decode_base81: invalid base-81 character");
  }
  return out;
}

} // namespace t81::hash
