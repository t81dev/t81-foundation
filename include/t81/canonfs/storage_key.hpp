#pragma once

#include <optional>
#include <string>
#include <string_view>
#include "t81/canonfs/canon_types.hpp"

namespace t81::canonfs {
inline constexpr std::string_view kStorageFormatFile = ".canonfs-format";
inline constexpr std::string_view kStorageFormatV1 = "t81-canonfs-hex-v1\n";

// A filesystem component, never the canonical identifier or a new hash function.
inline std::string storage_key_v1(const CanonHash& hash) {
  constexpr char digits[] = "0123456789abcdef";
  std::string key = "h1-";
  key.reserve(67);
  for (auto byte : hash.h.bytes) {
    key.push_back(digits[byte >> 4]);
    key.push_back(digits[byte & 15]);
  }
  return key;
}

inline std::optional<CanonHash> parse_storage_key_v1(std::string_view key) {
  if (key.size() != 67 || !key.starts_with("h1-")) return std::nullopt;
  auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
  };
  CanonHash hash{};
  for (std::size_t i = 0; i < hash.h.bytes.size(); ++i) {
    const int high = digit(key[3 + 2 * i]);
    const int low = digit(key[4 + 2 * i]);
    if (high < 0 || low < 0) return std::nullopt;
    hash.h.bytes[i] = static_cast<std::uint8_t>((high << 4) | low);
  }
  return hash;
}
}  // namespace t81::canonfs
