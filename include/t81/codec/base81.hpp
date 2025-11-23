#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace t81::codec::base81 {

/// Return the canonical 81-character alphabet used for encoding.
std::string_view alphabet();

/// Encode a sequence of bytes as a base-81 string.
/// Deterministic and invertible; no whitespace or padding.
std::string encode_bytes(const std::vector<std::uint8_t>& data);

/// Decode a base-81 string into bytes.
/// Returns true on success; false on invalid input.
bool decode_bytes(std::string_view s, std::vector<std::uint8_t>& out);

} // namespace t81::codec::base81
