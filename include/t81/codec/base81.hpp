#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace t81::codec::base81 {

/// Return the canonical 81-character alphabet used for encoding.
std::string_view alphabet();

/// Return canonical base-81 digit strings for byte-oriented/base81 codecs.
const std::vector<std::string>& digit_strings();

/// Return signed-integer-safe base-81 digit strings used by `T81BigInt` text rendering.
/// This variant reserves `-` for sign handling and uses `+` for digit 62.
const std::vector<std::string>& signed_integer_digit_strings();

/// Encode a sequence of bytes as a base-81 string.
/// Deterministic and invertible; no whitespace or padding.
std::string encode_bytes(const std::vector<std::uint8_t>& data);

/// Decode a base-81 string into bytes.
/// Returns true on success; false on invalid input.
bool decode_bytes(std::string_view s, std::vector<std::uint8_t>& out);

}  // namespace t81::codec::base81
