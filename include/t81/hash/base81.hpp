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

namespace detail {
// Legacy stub decoder: "b81:" + hex bytes.
inline std::vector<uint8_t> decode_base81_stub_hex(const std::string& s) {
  const std::string prefix = "b81:";
  if (s.rfind(prefix, 0) != 0) {
    throw std::invalid_argument(
        "decode_base81_stub_hex: missing 'b81:' prefix (stub expects hex fallback)");
  }
  std::string hex = s.substr(prefix.size());
  if (hex.size() % 2 != 0) {
    throw std::invalid_argument("decode_base81_stub_hex: hex length must be even");
  }

  auto hexval = [](char c) -> int {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + (c - 'a');
    if ('A' <= c && c <= 'F') return 10 + (c - 'A');
    return -1;
  };

  std::vector<uint8_t> out;
  out.reserve(hex.size() / 2);
  for (std::size_t i = 0; i < hex.size(); i += 2) {
    int hi = hexval(hex[i]);
    int lo = hexval(hex[i + 1]);
    if (hi < 0 || lo < 0) {
      throw std::invalid_argument("decode_base81_stub_hex: non-hex character");
    }
    out.push_back(static_cast<uint8_t>((hi << 4) | lo));
  }
  return out;
}

} // namespace detail

inline std::string encode_base81(const std::vector<uint8_t>& bytes) {
  return t81::codec::base81::encode_bytes(bytes);
}

inline std::vector<uint8_t> decode_base81(const std::string& s) {
  // Backward-compat: accept old stub strings.
  if (s.rfind("b81:", 0) == 0) {
    return detail::decode_base81_stub_hex(s);
  }

  std::vector<uint8_t> out;
  if (!t81::codec::base81::decode_bytes(s, out)) {
    throw std::invalid_argument("decode_base81: invalid base-81 character");
  }
  return out;
}

} // namespace t81::hash
