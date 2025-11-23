#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "t81/canonfs.hpp"

namespace t81::canonfs_io {

// Fixed wire format for CanonRef (99 bytes total):
//  [ 0..80] : CanonHash81.text           (81 bytes, zero-padded)
//  [81..82] : permissions                 (uint16, little-endian)
//  [83..90] : expires_at                  (uint64, little-endian)
//  [91..98] : reserved (0)
static constexpr std::size_t kWireSize = 99;

inline void encode_ref(const CanonRef& r, uint8_t out[kWireSize]) {
  std::memset(out, 0, kWireSize);
  // hash text
  std::memcpy(out + 0, r.target.text.data(), r.target.text.size());
  // permissions
  out[81] = static_cast<uint8_t>(r.permissions & 0xFF);
  out[82] = static_cast<uint8_t>((r.permissions >> 8) & 0xFF);
  // expires_at
  uint64_t t = r.expires_at;
  for (int i = 0; i < 8; ++i) out[83 + i] = static_cast<uint8_t>((t >> (8 * i)) & 0xFF);
  // [91..98] already zeroed
}

inline CanonRef decode_ref(const uint8_t in[kWireSize]) {
  CanonRef r{};
  std::memcpy(r.target.text.data(), in + 0, r.target.text.size());
  r.permissions = static_cast<uint16_t>(in[81]) |
                  (static_cast<uint16_t>(in[82]) << 8);
  uint64_t t = 0;
  for (int i = 0; i < 8; ++i) t |= (static_cast<uint64_t>(in[83 + i]) << (8 * i));
  r.expires_at = t;
  return r;
}

inline bool permissions_allow(uint16_t have, uint16_t want_mask) {
  return (have & want_mask) == want_mask;
}

} // namespace t81::canonfs_io
