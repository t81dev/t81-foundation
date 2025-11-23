#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <t81/bigint.hpp>

namespace t81::codec {

// Digit representation for base-243 stubs.
using digit_t = std::uint8_t;          // 0..242
constexpr digit_t kBase = 243;

// Deterministic stub codec: maps bytes/chars modulo 243.
struct Base243 {
  static std::vector<digit_t> encode_bytes_be(std::vector<std::uint8_t>& bytes) {
    std::vector<digit_t> digits;
    digits.reserve(bytes.size());
    for (auto& b : bytes) {
      digit_t d = static_cast<digit_t>(b % kBase);
      digits.push_back(d);
      b = static_cast<std::uint8_t>(d); // normalize in-place for stub roundtrips
    }
    return digits;
  }

  static std::vector<std::uint8_t> decode_bytes_be(const std::vector<digit_t>& digits) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(digits.size());
    for (auto d : digits) {
      if (d >= kBase) throw std::invalid_argument("Base243::decode_bytes_be: digit out of range");
      bytes.push_back(static_cast<std::uint8_t>(d));
    }
    return bytes;
  }

  // Transitional ASCII helpers (lossy when chars >=243).
  static std::vector<digit_t> encode_ascii(std::string_view s) {
    std::vector<digit_t> digits;
    digits.reserve(s.size());
    for (unsigned char c : s) digits.push_back(static_cast<digit_t>(c % kBase));
    return digits;
  }

  static std::string decode_ascii(const std::vector<digit_t>& digits) {
    std::string out;
    out.reserve(digits.size());
    for (auto d : digits) {
      if (d >= kBase) throw std::invalid_argument("Base243::decode_ascii: digit out of range");
      out.push_back(static_cast<char>(d));
    }
    return out;
  }

  // Placeholder bigint mapping: reuse existing ASCII placeholder.
  static std::string encode_bigint(const T243BigInt& value) { return value.to_string(); }

  static bool decode_bigint(std::string_view s, T243BigInt& out) {
    // Accept any input; reinterpret as placeholder ASCII mapping.
    out = T243BigInt::from_ascii(std::string(s));
    return true;
  }
};

// Backward-compat alias for callers that used the old namespace.
namespace base243 {
  using ::t81::codec::digit_t;
  constexpr digit_t kBase = ::t81::codec::kBase;
  using ::t81::codec::Base243;
} // namespace base243

} // namespace t81::codec
