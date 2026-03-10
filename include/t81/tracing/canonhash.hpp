#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "t81/tracing/base81.hpp"

namespace t81::hash {

/**
 * @struct CanonHash81
 * @brief Represents a canonical 256-bit hash for the T81 system.
 */
struct CanonHash81 {
  std::array<std::uint8_t, 32> bytes{};  // 256-bit hash

  bool operator==(const CanonHash81& other) const noexcept { return bytes == other.bytes; }
  bool operator!=(const CanonHash81& other) const noexcept { return !(*this == other); }

  /**
   * @brief Encodes the hash to a Base-81 string.
   * @return A Base-81 string (approx 41-43 characters).
   */
  std::string to_string() const {
    std::vector<std::uint8_t> v(bytes.begin(), bytes.end());
    return encode_base81(v);
  }

  /**
   * @brief Encodes the hash to a fixed-size 81-character text buffer (zero-padded).
   * Used for CanonFS wire format.
   */
  std::array<char, 81> to_text() const {
    std::array<char, 81> text{};
    text.fill(0);
    std::string s = to_string();
    const size_t n = s.size() < 81 ? s.size() : 81;
    std::memcpy(text.data(), s.data(), n);
    return text;
  }

  /**
   * @brief Parses a CanonHash81 from a Base-81 string or 81-character text.
   */
  static CanonHash81 from_string(std::string_view s) {
    // Trim potential zero padding if it's an 81-char buffer
    std::string_view trimmed = s;
    size_t last = s.find_first_of('\0');
    if (last != std::string_view::npos) {
      trimmed = s.substr(0, last);
    }

    std::vector<std::uint8_t> v = decode_base81(std::string(trimmed));
    CanonHash81 h;
    // For the canonical implementation, we might need to pad/truncate the bytes
    // if the input isn't exactly 32 bytes, but here we expect exactly 32.
    if (v.size() > 32) {
      throw std::invalid_argument("CanonHash81::from_string: input too long");
    }
    std::copy(v.begin(), v.end(), h.bytes.begin() + (32 - v.size()));
    return h;
  }
};

/**
 * @struct CanonHash384
 * @brief Represents a post-quantum 384-bit hash for the T81 system, retaining
 *        48 bytes of the underlying SHA3-512 digest to guarantee collision-resistance
 *        against cryptanalytic quantum search.
 */
struct CanonHash384 {
  std::array<std::uint8_t, 48> bytes{};  // 384-bit hash

  bool operator==(const CanonHash384& other) const noexcept { return bytes == other.bytes; }
  bool operator!=(const CanonHash384& other) const noexcept { return !(*this == other); }

  std::string to_string() const {
    std::vector<std::uint8_t> v(bytes.begin(), bytes.end());
    return encode_base81(v);
  }
};

// Deterministic hash over bytes using SHA3-512 truncated to 256 bits.
CanonHash81 hash_bytes(const std::vector<std::uint8_t>& data);
CanonHash81 hash_bytes(std::span<const std::byte> data);
CanonHash384 hash_bytes_pq(std::span<const std::byte> data);

// Convenience wrapper for strings.
CanonHash81 hash_string(std::string_view s);
CanonHash384 hash_string_pq(std::string_view s);

// Compatibility alias
inline CanonHash81 make_canonhash81_base81stub(std::string_view s) { return hash_string(s); }

}  // namespace t81::hash
