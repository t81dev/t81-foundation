#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "t81/canonfs.hpp"
#include "t81/hash/base81.hpp"

namespace t81::hash {

// -----------------------------------------------------------------------------
// CanonHash81 (STUB)
//
// This stub “hashes” input by:
//   1) Taking the raw bytes as-is,
//   2) Encoding them with the Base-81 *stub* (b81:+hex fallback),
//   3) Truncating/padding the ASCII result to exactly 81 chars.
//
// This is NON-CRYPTOGRAPHIC and only intended to let components link and
// round-trip during migration. Replace with a real digest + base-81 codec.
// -----------------------------------------------------------------------------

inline CanonHash81 make_canonhash81_base81stub(const void* data, std::size_t len) {
  const auto* p = static_cast<const uint8_t*>(data);
  std::vector<uint8_t> bytes(p, p + len);
  std::string enc = encode_base81(bytes);        // e.g., "b81:deadbeef..."
  // Truncate/pad to 81 characters.
  if (enc.size() > 81) enc.resize(81);
  CanonHash81 h = CanonHash81::from_string(enc);
  return h;
}

inline CanonHash81 make_canonhash81_base81stub(const std::vector<uint8_t>& bytes) {
  return make_canonhash81_base81stub(bytes.data(), bytes.size());
}

inline CanonHash81 make_canonhash81_base81stub(const std::string& s) {
  return make_canonhash81_base81stub(s.data(), s.size());
}

} // namespace t81::hash
