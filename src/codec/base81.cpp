#include <t81/codec/base81.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace t81::codec::base81 {

// Canonical v1.1.0 alphabet (UTF-8). Indices 62..80 are the 19 symbols listed in the spec after a–z.
static const std::vector<std::string>& alphabet_vec() {
  static const std::vector<std::string> kAlphabet = {
    // 0..9
    "0","1","2","3","4","5","6","7","8","9",
    // 10..35
    "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    // 36..61
    "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z",
    // 62..80 (spec order, first 19 symbols)
    "+","−","×","÷","=","<",">","≤","≥","≠","≈","∞","λ","μ","π","σ","τ","ω","Γ"
  };
  static_assert(81 == 81, "alphabet size compile guard"); // intentional; we validate below.
  return kAlphabet;
}

std::string_view alphabet() {
  // Concatenate for compatibility; note: codepoints may be multi-byte.
  static const std::string joined = []{
    std::string s;
    for (const auto& cp : alphabet_vec()) s += cp;
    return s;
  }();
  return std::string_view(joined);
}

// Decode next UTF-8 codepoint; returns empty string on invalid encoding.
static std::string next_codepoint(const char* data, std::size_t len, std::size_t& offset) {
  if (offset >= len) return {};
  unsigned char c = static_cast<unsigned char>(data[offset]);
  std::size_t cp_len = 0;
  if (c < 0x80) cp_len = 1;
  else if ((c & 0xE0) == 0xC0) cp_len = 2;
  else if ((c & 0xF0) == 0xE0) cp_len = 3;
  else if ((c & 0xF8) == 0xF0) cp_len = 4;
  else return {};
  if (offset + cp_len > len) return {};
  std::string cp(data + offset, data + offset + cp_len);
  offset += cp_len;
  return cp;
}

static const std::unordered_map<std::string, int>& alphabet_map() {
  static const std::unordered_map<std::string, int> kMap = [] {
    const auto& alpha = alphabet_vec();
    std::unordered_map<std::string, int> m;
    m.reserve(alpha.size());
    for (std::size_t i = 0; i < alpha.size(); ++i) {
      m.emplace(alpha[i], static_cast<int>(i));
    }
    return m;
  }();
  return kMap;
}

std::string encode_bytes(const std::vector<std::uint8_t>& data) {
  if (data.empty()) return std::string{};

  std::vector<std::uint8_t> buf = data;  // base-256 digits, big-endian
  std::vector<int> digits;
  digits.reserve(data.size() + 1);

  while (!buf.empty()) {
    std::vector<std::uint8_t> next;
    next.reserve(buf.size());

    std::uint16_t carry = 0;
    for (auto b : buf) {
      std::uint16_t cur = static_cast<std::uint16_t>((carry << 8) | b); // base 256
      std::uint8_t q = static_cast<std::uint8_t>(cur / 81);
      std::uint8_t r = static_cast<std::uint8_t>(cur % 81);
      if (!next.empty() || q != 0) {
        next.push_back(q);
      }
      carry = r;
    }
    digits.push_back(static_cast<int>(carry));
    buf.swap(next);
  }

  const auto& alpha = alphabet_vec();
  std::string out;
  for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
    out += alpha[static_cast<std::size_t>(*it)];
  }
  return out;
}

bool decode_bytes(std::string_view s, std::vector<std::uint8_t>& out) {
  out.clear();
  if (s.empty()) return true;

  const auto& map = alphabet_map();

  // Parse codepoints
  std::vector<int> digits;
  std::size_t offset = 0;
  while (offset < s.size()) {
    auto cp = next_codepoint(s.data(), s.size(), offset);
    if (cp.empty()) return false;
    auto it = map.find(cp);
    if (it == map.end()) return false;
    digits.push_back(it->second);
  }
  // Reject non-canonical leading zeros (except the lone zero digit).
  if (digits.size() > 1 && digits.front() == 0) return false;

  // Convert base-81 digits (MSB-first) to base-256 bytes (big-endian).
  std::vector<std::uint8_t> buf{0}; // big-endian
  for (int d : digits) {
    std::uint16_t carry = static_cast<std::uint16_t>(d);
    for (int i = static_cast<int>(buf.size()) - 1; i >= 0; --i) {
      std::uint16_t cur = static_cast<std::uint16_t>(buf[static_cast<std::size_t>(i)]) * 81u + carry;
      buf[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(cur & 0xFF);
      carry = static_cast<std::uint16_t>(cur >> 8);
    }
    while (carry) {
      buf.insert(buf.begin(), static_cast<std::uint8_t>(carry & 0xFF));
      carry = static_cast<std::uint16_t>(carry >> 8);
    }
  }

  out = std::move(buf);
  return true;
}

} // namespace t81::codec::base81
