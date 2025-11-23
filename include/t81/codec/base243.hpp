#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

namespace t81::codec {

// A tiny, deterministic **stub** for base-243 digit vectors.
// NOTE: This is *not* a true base conversion. It simply maps:
//   - bytes  -> digits via (b % 243)
//   - digits -> bytes via identity (assert digit in [0..242])
// and preserves order (big-endian for "bytes_be" variants).
// Swap with a canonical radix-243 codec later without changing signatures.

using digit_t = uint8_t;           // constrained to [0..242]
static constexpr digit_t kBase = 243;

// --- bytes (big-endian order) ---
struct Base243 {
  // Encode a byte array to base-243 digits (big-endian). Each byte → one digit (b % 243).
  static std::vector<digit_t> encode_bytes_be(const std::vector<uint8_t>& bytes) {
    std::vector<digit_t> out;
    out.reserve(bytes.size());
    for (uint8_t b : bytes) out.push_back(static_cast<digit_t>(b % kBase));
    return out;
  }

  // Decode digits (big-endian) back to bytes. Each digit must be <= 242; copied as-is.
  static std::vector<uint8_t> decode_bytes_be(const std::vector<digit_t>& digits) {
    std::vector<uint8_t> out;
    out.reserve(digits.size());
    for (digit_t d : digits) {
      if (d >= kBase) throw std::invalid_argument("Base243.decode_bytes_be: digit out of range");
      out.push_back(static_cast<uint8_t>(d));
    }
    return out;
  }

  // --- transitional helpers for ASCII (stub: char -> digit, digit -> char) ---
  static std::vector<digit_t> encode_ascii(const std::string& s) {
    std::vector<digit_t> out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back(static_cast<digit_t>(c % kBase));
    return out;
  }

  static std::string decode_ascii(const std::vector<digit_t>& digits) {
    std::string out;
    out.reserve(digits.size());
    for (digit_t d : digits) {
      if (d >= kBase) throw std::invalid_argument("Base243.decode_ascii: digit out of range");
      out.push_back(static_cast<char>(d)); // lossy inverse of modulo mapping
    }
    return out;
  }
};

} // namespace t81::codec
