#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "t81/hash/base81.hpp"

namespace t81::hash {

struct CanonHash81 {
  std::array<std::uint8_t, 32> bytes{};  // 256-bit hash

  bool operator==(const CanonHash81& other) const noexcept {
    return bytes == other.bytes;
  }
  bool operator!=(const CanonHash81& other) const noexcept {
    return !(*this == other);
  }

  // Encode to base-81 string using the canonical codec.
  std::string to_string() const {
    std::vector<std::uint8_t> v(bytes.begin(), bytes.end());
    return encode_base81(v);
  }

  // Parse from base-81 string representation.
  // Throws std::invalid_argument on invalid input or wrong length.
  static CanonHash81 from_string(std::string_view s) {
    std::vector<std::uint8_t> v = decode_base81(std::string(s));
    CanonHash81 h;
    if (v.size() != h.bytes.size()) {
      throw std::invalid_argument("CanonHash81::from_string: wrong byte length");
    }
    std::copy(v.begin(), v.end(), h.bytes.begin());
    return h;
  }
};

// Deterministic, non-cryptographic hash over bytes.
CanonHash81 hash_bytes(const std::vector<std::uint8_t>& data);

// Convenience wrapper for strings.
CanonHash81 hash_string(std::string_view s);

// Optional compatibility alias for old stub name:
inline CanonHash81 make_canonhash81_base81stub(std::string_view s) {
  return hash_string(s);
}

}  // namespace t81::hash
