#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring>
#include "t81/canonfs.hpp"

namespace t81::canonfs_io {

// Wire format (v0, little-endian), 99 bytes total:
// [0..80]   : CanonHash81.text (81 bytes, zero-padded if shorter)
// [81..82]  : permissions (uint16 LE)
// [83..90]  : expires_at (uint64 LE)
// [91..98]  : reserved (8 bytes = 0)
//
// NOTE: This is a minimal, non-cryptographic transport. Hash content is used as-is;
//       real systems should validate canonical Base-81 encoding and signature chains.

inline std::vector<uint8_t> encode_ref(const CanonRef& r) {
  std::vector<uint8_t> out;
  out.resize(99, 0);
  // hash
  std::memcpy(out.data() + 0,  r.target.text.data(), r.target.text.size());
  // permissions
  out[81] = static_cast<uint8_t>(r.permissions & 0xFF);
  out[82] = static_cast<uint8_t>((r.permissions >> 8) & 0xFF);
  // expires_at
  uint64_t e = r.expires_at;
  for (int i = 0; i < 8; ++i) out[83 + i] = static_cast<uint8_t>((e >> (8*i)) & 0xFF);
  // reserved already zeroed [91..98]
  return out;
}

inline CanonRef decode_ref(const uint8_t* data, size_t len) {
  if (!data || len < 99) throw std::invalid_argument("canonfs_io: buffer too small (need 99 bytes)");
  CanonRef r{};
  std::memcpy(r.target.text.data(), data + 0, r.target.text.size());
  r.permissions = static_cast<uint16_t>(data[81] | (static_cast<uint16_t>(data[82]) << 8));
  uint64_t e = 0;
  for (int i = 0; i < 8; ++i) e |= (static_cast<uint64_t>(data[83 + i]) << (8*i));
  r.expires_at = e;
  // reserved bytes ignored
  return r;
}

inline bool permissions_allow(uint16_t perms, uint16_t required_mask) {
  return (perms & required_mask) == required_mask;
}

} // namespace t81::canonfs_io
