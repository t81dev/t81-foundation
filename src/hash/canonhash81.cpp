// src/hash/canonhash81.cpp
#include "t81/codec/base81.hpp"
#include <array>

// Expect BLAKE3 in third_party or system; fall back to reference when absent.
extern "C" void blake3_hash(const uint8_t* in, size_t len, uint8_t out[32]); // adapt

namespace t81::hash {
std::string canonhash81(std::span<const uint8_t> data) {
  // CanonFS spec: CanonHash-81(data) := base81_encode(BLAKE3(data)[0..60])
  // We need 60 bytes — use BLAKE3 XOF to 60 bytes if library supports; otherwise
  // hash chaining/extendable mode. (Wire to XOF in real impl.)
  std::array<uint8_t, 60> digest{}; /* TODO: fill via XOF */
  // ... fill digest ...
  return t81::codec::base81_encode(digest);
}
}
