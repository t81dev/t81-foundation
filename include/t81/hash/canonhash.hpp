#pragma once
#include <cstddef>
#include <cstring>
#include <vector>
#include "t81/canonfs.hpp"
#include "t81/hash/base81.hpp"

namespace t81::hash {

// Produce a CanonHash81 using the stub Base-81 encoder.
// This does NOT hash; it just encodes the raw bytes and truncates/pads to 81 bytes.
// Replace with a real digest (e.g., BLAKE3 -> Base-81) in production.
inline CanonHash81 make_canonhash81_base81stub(const void* data, size_t len) {
  CanonHash81 h{};
  if (!data || len == 0) return h;
  const auto* p = static_cast<const unsigned char*>(data);
  std::vector<uint8_t> bytes(p, p + len);
  std::string enc = encode_base81(bytes);  // e.g., "b81:...."
  // Copy up to 81 chars; zero-pad the rest.
  const size_t n = enc.size() < h.target.text.size() ? enc.size() : h.target.text.size();
  std::memcpy(h.target.text.data(), enc.data(), n);
  return h;
}

} // namespace t81::hash
