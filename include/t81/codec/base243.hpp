#pragma once
#include <string>
#include <string_view>
#include <t81/bigint.hpp>

namespace t81::codec::base243 {

/// Encode a T243BigInt as a base-243 string.
/// Convention: optional leading '+' or '-' for sign; digits in [0,242].
std::string encode_bigint(const T243BigInt& value);

/// Decode a base-243 string into T243BigInt.
/// Returns true on success, false on invalid syntax.
bool decode_bigint(std::string_view s, T243BigInt& out);

} // namespace t81::codec::base243
