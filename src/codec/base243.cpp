#include <t81/codec/base243.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <t81/bigint/divmod.hpp>

namespace t81::codec {

std::vector<digit_t> Base243::encode_bytes_be(const std::vector<std::uint8_t>& bytes) {
  if (bytes.empty()) return {};

  // Treat bytes as a big-endian base-256 integer and convert to base-243 digits.
  std::vector<std::uint8_t> buf = bytes;  // mutable copy
  std::vector<digit_t> digits;
  digits.reserve(bytes.size() + 1);

  while (!buf.empty()) {
    std::vector<std::uint8_t> next;
    next.reserve(buf.size());

    std::uint16_t carry = 0;
    for (std::uint8_t b : buf) {
      std::uint16_t cur = static_cast<std::uint16_t>((carry << 8) | b);  // base-256 step
      std::uint8_t q = static_cast<std::uint8_t>(cur / kBase);
      std::uint8_t r = static_cast<std::uint8_t>(cur % kBase);
      if (!next.empty() || q != 0) {
        next.push_back(q);
      }
      carry = r;
    }
    digits.push_back(static_cast<digit_t>(carry));
    buf.swap(next);
  }

  std::reverse(digits.begin(), digits.end());  // MSB-first
  return digits;
}

std::vector<std::uint8_t> Base243::decode_bytes_be(const std::vector<digit_t>& digits) {
  if (digits.empty()) return {};

  // Classic base conversion: accumulate digits (MSB-first) into a base-256 vector.
  std::vector<std::uint8_t> out{0};  // big-endian
  for (digit_t d : digits) {
    if (d >= kBase) throw std::invalid_argument("Base243::decode_bytes_be: digit out of range");

    std::uint16_t carry = d;
    // Multiply existing number by 243 and add the new digit.
    for (int i = static_cast<int>(out.size()) - 1; i >= 0; --i) {
      std::uint16_t cur = static_cast<std::uint16_t>(out[static_cast<std::size_t>(i)]) * kBase + carry;
      out[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(cur & 0xFF); // mod 256
      carry = static_cast<std::uint16_t>(cur >> 8);                             // div 256
    }
    while (carry) {
      out.insert(out.begin(), static_cast<std::uint8_t>(carry & 0xFF));
      carry = static_cast<std::uint16_t>(carry >> 8);
    }
  }

  // Strip leading zeros (but leave one zero if the value is zero).
  while (out.size() > 1 && out.front() == 0) {
    out.erase(out.begin());
  }
  return out;
}

std::vector<digit_t> Base243::encode_ascii(std::string_view s) {
  return encode_bytes_be(std::vector<std::uint8_t>(s.begin(), s.end()));
}

std::string Base243::decode_ascii(const std::vector<digit_t>& digits) {
  auto bytes = decode_bytes_be(digits);
  return std::string(bytes.begin(), bytes.end());
}

std::string Base243::encode_bigint(const T81BigInt& value) {
  if (value.is_zero()) return "0";

  const bool neg = value.is_negative();
  T81BigInt v = neg ? value.abs() : value;
  const T81BigInt base(kBase);

  std::vector<int> digits;
  digits.reserve(32);

  while (!v.is_zero()) {
    auto dm = divmod(v, base);
    int d = static_cast<int>(dm.r.to_int64());
    if (d < 0 || d >= static_cast<int>(kBase)) {
      throw std::runtime_error("encode_bigint: remainder out of range");
    }
    digits.push_back(d);
    v = dm.q;
  }

  std::ostringstream oss;
  if (neg) oss << '-';
  for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
    if (it != digits.rbegin()) oss << '.';
    oss << *it;
  }
  return oss.str();
}

bool Base243::decode_bigint(std::string_view s, T81BigInt& out) {
  if (s.empty()) return false;

  bool neg = false;
  std::size_t pos = 0;
  if (s[pos] == '+' || s[pos] == '-') {
    neg = (s[pos] == '-');
    ++pos;
    if (pos >= s.size()) return false;
  }

  std::vector<int> digits;
  int current = 0;
  bool have_digit = false;

  for (; pos <= s.size(); ++pos) {
    if (pos == s.size() || s[pos] == '.') {
      if (!have_digit) return false;
      if (current < 0 || current >= static_cast<int>(kBase)) return false;
      digits.push_back(current);
      current = 0;
      have_digit = false;
    } else if (s[pos] >= '0' && s[pos] <= '9') {
      have_digit = true;
      current = current * 10 + (s[pos] - '0');
      if (current >= 1000) return false; // prevent runaway parsing
    } else {
      return false;
    }
  }

  T81BigInt base(kBase);
  T81BigInt v(0);
  for (int d : digits) {
    v = T81BigInt::mul(v, base);
    v = T81BigInt::add(v, T81BigInt(d));
  }
  if (neg) v = v.neg();
  out = std::move(v);
  return true;
}

}  // namespace t81::codec
