#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

namespace t81::codec {

// Base-243 digit range: [0..242]
using digit_t = uint8_t;

// Interface for a canonical Base-243 codec.
// This header defines the API only; provide a platform implementation later.
// NOTE: Our current BigInt uses a placeholder ASCII mapping. When a real codec
// lands, wire T243BigInt to use these functions instead.

struct Base243 {
  // Encode a big-endian byte string into LSB-first base-243 digits.
  // Example: bytes {0x01, 0x02} -> digits [ .. ]
  static std::vector<digit_t> encode_bytes_be(const std::vector<uint8_t>& bytes);

  // Decode LSB-first base-243 digits into a big-endian byte string.
  // Leading zero digits are permitted and should be ignored.
  static std::vector<uint8_t> decode_bytes_be(const std::vector<digit_t>& digits);

  // Convenience: encode ASCII text to base-243 digits (implementation-defined).
  // This is a transitional API; prefer encode_bytes_be for canonical behavior.
  static std::vector<digit_t> encode_ascii(const std::string& s);

  // Convenience: decode digits to an ASCII-like string (transitional).
  static std::string decode_ascii(const std::vector<digit_t>& digits);
};

// ------------------------------
// Stub (header-only, deterministic, NOT canonical)
// ------------------------------
// Provides trivial implementations so callers can compile. Replace with a
// mathematically correct base-conversion (radix-243) when ready.
inline std::vector<digit_t> Base243::encode_bytes_be(const std::vector<uint8_t>& bytes) {
  // Map each byte to one digit (mod 243); real codec should do big-radix conversion.
  std::vector<digit_t> out;
  out.reserve(bytes.size());
  for (uint8_t b : bytes) out.push_back(static_cast<digit_t>(b % 243));
  return out; // LSB-first semantics aren't meaningful in this stub.
}

inline std::vector<uint8_t> Base243::decode_bytes_be(const std::vector<digit_t>& digits) {
  // Inverse of the stub above: map each digit back to a byte.
  std::vector<uint8_t> out;
  out.reserve(digits.size());
  for (digit_t d : digits) out.push_back(static_cast<uint8_t>(d));
  return out;
}

inline std::vector<digit_t> Base243::encode_ascii(const std::string& s) {
  std::vector<digit_t> out;
  out.reserve(s.size());
  for (unsigned char c : s) out.push_back(static_cast<digit_t>(c % 243));
  return out;
}

inline std::string Base243::decode_ascii(const std::vector<digit_t>& digits) {
  std::string s;
  s.reserve(digits.size());
  for (digit_t d : digits) s.push_back(static_cast<char>(d));
  return s;
}

} // namespace t81::codec
