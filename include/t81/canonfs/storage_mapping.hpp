#pragma once

#include <optional>
#include <string>
#include <string_view>
#include "t81/canonfs/canon_types.hpp"

namespace t81::canonfs {
// Storage-map v2: a bijection on the existing 32-byte digest, not a new identity.
inline std::string portable_storage_key(const CanonHash& hash) {
  constexpr char digits[] = "0123456789abcdef";
  std::string key;
  key.reserve(64);
  for (auto b : hash.h.bytes) {
    key += digits[b >> 4];
    key += digits[b & 15];
  }
  return key;
}

inline std::optional<CanonHash> parse_portable_storage_key(std::string_view key) {
  if (key.size() != 64) return std::nullopt;
  CanonHash hash{};
  for (std::size_t i = 0; i < 32; ++i) {
    auto digit = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      return -1;
    };
    const int high = digit(key[i * 2]), low = digit(key[i * 2 + 1]);
    if (high < 0 || low < 0) return std::nullopt;
    hash.h.bytes[i] = static_cast<std::uint8_t>(high * 16 + low);
  }
  return hash;
}
}  // namespace t81::canonfs
