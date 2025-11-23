#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

namespace t81::hash {

// -----------------------------------------------------------------------------
// Base-81 codec
// -----------------------------------------------------------------------------
// New canonical behavior:
//   - encode_base81: bytes -> base-81 string using a fixed 81-character alphabet.
//   - decode_base81: base-81 string -> bytes.
//
// Backward compatibility:
//   - If the input to decode_base81() begins with "b81:", the old stub
//     hex format is still accepted and decoded.
//   - encode_base81() NEVER emits the "b81:" prefix.
// -----------------------------------------------------------------------------

namespace detail {

inline std::string_view base81_alphabet() {
  // 81 chars: 10 digits + 26 upper + 26 lower + 19 punctuation
  static constexpr char kAlphabet[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "!#$%&()*+,-./:;<=>?";
  static_assert(sizeof(kAlphabet) - 1 == 81, "Base-81 alphabet must have 81 chars");
  return std::string_view{kAlphabet, 81};
}

inline int base81_digit(char c) {
  auto a = base81_alphabet();
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] == c) return static_cast<int>(i);
  }
  return -1;
}

// Legacy stub decoder: "b81:" + hex bytes.
inline std::vector<uint8_t> decode_base81_stub_hex(const std::string& s) {
  const std::string prefix = "b81:";
  if (s.rfind(prefix, 0) != 0) {
    throw std::invalid_argument(
        "decode_base81_stub_hex: missing 'b81:' prefix (stub expects hex fallback)");
  }
  std::string hex = s.substr(prefix.size());
  if (hex.size() % 2 != 0) {
    throw std::invalid_argument("decode_base81_stub_hex: hex length must be even");
  }

  auto hexval = [](char c) -> int {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + (c - 'a');
    if ('A' <= c && c <= 'F') return 10 + (c - 'A');
    return -1;
  };

  std::vector<uint8_t> out;
  out.reserve(hex.size() / 2);
  for (std::size_t i = 0; i < hex.size(); i += 2) {
    int hi = hexval(hex[i]);
    int lo = hexval(hex[i + 1]);
    if (hi < 0 || lo < 0) {
      throw std::invalid_argument("decode_base81_stub_hex: non-hex character");
    }
    out.push_back(static_cast<uint8_t>((hi << 4) | lo));
  }
  return out;
}

} // namespace detail

// -----------------------------------------------------------------------------
// encode_base81: bytes -> base-81 string
// -----------------------------------------------------------------------------
//
// Interprets the input as a big-endian base-256 integer and converts it to
// base-81 using the fixed alphabet above.
//
// Properties:
//   - Deterministic.
//   - No "b81:" prefix.
//   - Empty input -> empty string.
//
inline std::string encode_base81(const std::vector<uint8_t>& bytes) {
  if (bytes.empty()) {
    return std::string{};
  }

  // Copy to a mutable buffer of base-256 "digits" (big-endian).
  std::vector<uint8_t> buf = bytes;
  std::string out;
  out.reserve(buf.size() * 2); // rough guess

  const auto alphabet = detail::base81_alphabet();

  // Repeated division-by-81 algorithm on base-256 digits.
  while (!buf.empty()) {
    std::vector<uint8_t> next;
    next.reserve(buf.size());

    uint16_t carry = 0;
    for (uint8_t b : buf) {
      uint16_t cur = static_cast<uint16_t>((carry << 8) | b); // base 256
      uint8_t q = static_cast<uint8_t>(cur / 81);
      uint8_t r = static_cast<uint8_t>(cur % 81);
      if (!next.empty() || q != 0) {
        next.push_back(q);
      }
      carry = r;
    }

    // carry is a single base-81 digit in [0,80]
    out.push_back(alphabet[carry]);
    buf.swap(next);
  }

  // Digits were produced least significant first
  std::reverse(out.begin(), out.end());
  return out;
}

// -----------------------------------------------------------------------------
// decode_base81: base-81 string (or old stub) -> bytes
// -----------------------------------------------------------------------------
//
// Behavior:
//   - If s starts with "b81:", treat it as the legacy stub hex encoding.
//   - Otherwise, treat s as a base-81 string using the fixed alphabet.
//
// Properties (new format):
//   - Empty string -> empty vector.
//   - Throws std::invalid_argument on invalid characters.
// -----------------------------------------------------------------------------
inline std::vector<uint8_t> decode_base81(const std::string& s) {
  // Backward-compat: accept old stub strings.
  if (s.rfind("b81:", 0) == 0) {
    return detail::decode_base81_stub_hex(s);
  }

  if (s.empty()) {
    return {};
  }

  // Convert base-81 string back into a big-endian base-256 byte vector.
  std::vector<uint8_t> buf; // base-256 digits, big-endian

  for (char c : s) {
    int d = detail::base81_digit(c);
    if (d < 0) {
      throw std::invalid_argument("decode_base81: invalid base-81 character");
    }

    std::vector<uint8_t> next;
    next.reserve(buf.size() + 1);

    uint16_t carry = static_cast<uint16_t>(d);
    for (uint8_t b : buf) {
      uint16_t cur = static_cast<uint16_t>(b) * 81 + carry;
      uint8_t q = static_cast<uint8_t>(cur >> 8);     // / 256
      uint8_t r = static_cast<uint8_t>(cur & 0xFF);   // % 256
      if (!next.empty() || q != 0) {
        next.push_back(q);
      }
      carry = r;
    }
    if (!next.empty() || carry != 0) {
      next.push_back(static_cast<uint8_t>(carry));
    }

    buf.swap(next);
  }

  return buf;
}

} // namespace t81::hash
