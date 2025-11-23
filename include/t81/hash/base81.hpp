#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

namespace t81::hash {

// Base-81 codec interface (stub).
// Replace these with your canonical alphabet + implementation.
// These functions exist so other modules can depend on a stable API
// without caring whether the real codec is wired yet.

inline std::string encode_base81(const std::vector<uint8_t>& bytes) {
  // STUB: returns a hex-like fallback "bNN" chunks so outputs are deterministic.
  static const char* hex = "0123456789abcdef";
  std::string out;
  out.reserve(bytes.size() * 2 + 2);
  out.push_back('b'); out.push_back('8'); out.push_back('1'); out.push_back(':');
  for (uint8_t b : bytes) {
    out.push_back(hex[(b >> 4) & 0xF]);
    out.push_back(hex[b & 0xF]);
  }
  return out; // e.g., "b81:0a1b..."
}

inline std::vector<uint8_t> decode_base81(const std::string& s) {
  // STUB: accepts the stub format produced by encode_base81 above.
  if (s.size() < 4 || s[0] != 'b' || s[1] != '8' || s[2] != '1' || s[3] != ':')
    throw std::invalid_argument("decode_base81: unsupported stub format");
  auto hexval = [](char c)->int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };
  std::vector<uint8_t> out;
  const size_t n = s.size() - 4;
  if (n % 2 != 0) throw std::invalid_argument("decode_base81: odd hex length in stub");
  out.reserve(n/2);
  for (size_t i = 4; i < s.size(); i += 2) {
    int hi = hexval(s[i]);
    int lo = hexval(s[i+1]);
    if (hi < 0 || lo < 0) throw std::invalid_argument("decode_base81: invalid hex in stub");
    out.push_back(static_cast<uint8_t>((hi << 4) | lo));
  }
  return out;
}

} // namespace t81::hash
