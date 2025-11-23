#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <t81/bigint.hpp>

namespace t81::codec {

// Digit representation for base-243; digits are MSB-first unless noted.
using digit_t = std::uint8_t;          // 0..242
constexpr digit_t kBase = 243;

struct Base243 {
  // Interpret big-endian bytes as a base-256 integer and emit base-243 digits (MSB-first).
  static std::vector<digit_t> encode_bytes_be(const std::vector<std::uint8_t>& bytes);

  // Inverse of encode_bytes_be; throws std::invalid_argument on digit >= 243.
  static std::vector<std::uint8_t> decode_bytes_be(const std::vector<digit_t>& digits);

  // ASCII helpers built on the byte encoder/decoder.
  static std::vector<digit_t> encode_ascii(std::string_view s);
  static std::string decode_ascii(const std::vector<digit_t>& digits);

  // Canonical bigint codec: emit digits separated by '.' (MSB-first); optional leading '-'.
  static std::string encode_bigint(const T81BigInt& value);
  static bool decode_bigint(std::string_view s, T81BigInt& out);
};

// Backward-compat alias for callers that used the old namespace.
namespace base243 {
  using ::t81::codec::digit_t;
  constexpr digit_t kBase = ::t81::codec::kBase;
  using ::t81::codec::Base243;
} // namespace base243

} // namespace t81::codec
