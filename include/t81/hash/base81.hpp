#pragma once
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace t81::hash {

// -----------------------------------------------------------------------------
// Base-81 (STUB) codec
// -----------------------------------------------------------------------------
// This is a deterministic placeholder used only to wire things together.
// It DOES NOT implement real base-81 encoding. Instead, it produces a
// human-readable fallback:
//    encode: "b81:" + hex(bytes)
//    decode: if text starts with "b81:", parse hex; otherwise error.
//
// Swap with a canonical Base-81 implementation later without changing the API.
// -----------------------------------------------------------------------------

inline std::string encode_base81(const std::vector<uint8_t>& bytes) {
  std::ostringstream oss;
  oss << "b81:";
  oss << std::hex << std::setfill('0');
  for (uint8_t b : bytes) {
    oss << std::setw(2) << static_cast<unsigned>(b);
  }
  return oss.str();
}

inline std::vector<uint8_t> decode_base81(const std::string& s) {
  const std::string prefix = "b81:";
  if (s.rfind(prefix, 0) != 0) {
    throw std::invalid_argument("decode_base81: missing 'b81:' prefix (stub expects hex fallback)");
  }
  std::string hex = s.substr(prefix.size());
  if (hex.size() % 2 != 0) {
    throw std::invalid_argument("decode_base81: hex length must be even");
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
      throw std::invalid_argument("decode_base81: non-hex character");
    }
    out.push_back(static_cast<uint8_t>((hi << 4) | lo));
  }
  return out;
}

} // namespace t81::hash
