#include "t81/codec/base81.hpp"
#include "t81/codec/trit_packing.hpp"
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>

namespace t81::codec::trit_packing {

static const std::vector<std::string>& b81_alphabet_vec() {
  return t81::codec::base81::digit_strings();
}

static const std::unordered_map<std::string, uint8_t>& b81_alphabet_map() {
  static const std::unordered_map<std::string, uint8_t> kMap = [] {
    const auto& alpha = b81_alphabet_vec();
    std::unordered_map<std::string, uint8_t> m;
    for (size_t i = 0; i < alpha.size(); ++i) {
      m[alpha[i]] = static_cast<uint8_t>(i);
    }
    return m;
  }();
  return kMap;
}

static std::string next_utf8(std::string_view s, size_t& offset) {
  if (offset >= s.size()) return "";
  unsigned char c = static_cast<unsigned char>(s[offset]);
  size_t len = 0;
  if (c < 0x80)
    len = 1;
  else if ((c & 0xE0) == 0xC0)
    len = 2;
  else if ((c & 0xF0) == 0xE0)
    len = 3;
  else if ((c & 0xF8) == 0xF0)
    len = 4;
  else
    return "";
  if (offset + len > s.size()) return "";
  std::string cp(s.substr(offset, len));
  offset += len;
  return cp;
}

const char* to_string(PackingError err) {
  switch (err) {
    case PackingError::InvalidPT5Byte:
      return "InvalidPT5Byte";
    case PackingError::InvalidBase81Digit:
      return "InvalidBase81Digit";
    case PackingError::InvalidTritValue:
      return "InvalidTritValue";
    case PackingError::InsufficientData:
      return "InsufficientData";
    case PackingError::TritCountMismatch:
      return "TritCountMismatch";
    case PackingError::EncodingError:
      return "EncodingError";
    default:
      return "UnknownError";
  }
}

static T81Error make_error(PackingError err, T81String msg) {
  return T81Error(T81Symbol::intern(to_string(err)), std::move(msg),
                  T81Symbol::intern("trit_packing"));
}

Result<std::vector<uint8_t>> pack_pt5(const std::vector<Trit>& trits) {
  std::vector<uint8_t> result;
  result.reserve((trits.size() + 4) / 5);

  for (size_t i = 0; i < trits.size(); i += 5) {
    uint16_t byte = 0;
    uint16_t multiplier = 1;
    for (size_t j = 0; j < 5; ++j) {
      int t_raw = 0;  // Default to Z (0)
      if (i + j < trits.size()) {
        t_raw = static_cast<int>(trits[i + j]);
      }
      if (t_raw < -1 || t_raw > 1) {
        return make_error(PackingError::InvalidTritValue, T81String("Trit not in {-1, 0, +1}"));
      }
      int val = t_raw + 1;
      byte += static_cast<uint16_t>(val) * multiplier;
      multiplier *= 3;
    }
    result.push_back(static_cast<uint8_t>(byte));
  }
  return result;
}

Result<std::vector<Trit>> unpack_pt5(const std::vector<uint8_t>& bytes, size_t trit_count) {
  if (bytes.size() < (trit_count + 4) / 5) {
    return make_error(PackingError::InsufficientData,
                      T81String("Insufficient PT-5 bytes for trit_count"));
  }

  std::vector<Trit> trits;
  trits.reserve(trit_count);

  for (size_t i = 0; i < bytes.size(); ++i) {
    uint16_t val = bytes[i];
    if (val > 242) {
      return make_error(PackingError::InvalidPT5Byte, T81String("PT-5 byte > 242"));
    }
    for (size_t j = 0; j < 5; ++j) {
      if (trits.size() < trit_count) {
        int digit = val % 3;
        trits.push_back(static_cast<Trit>(digit - 1));
        val /= 3;
      }
    }
  }
  return trits;
}

Result<std::vector<uint8_t>> pack_base81(const std::vector<Trit>& trits) {
  std::vector<uint8_t> result;
  result.reserve((trits.size() + 3) / 4);

  for (size_t i = 0; i < trits.size(); i += 4) {
    uint16_t digit = 0;
    uint16_t multiplier = 1;
    for (size_t j = 0; j < 4; ++j) {
      int t_raw = 0;  // Default to Z (0)
      if (i + j < trits.size()) {
        t_raw = static_cast<int>(trits[i + j]);
      }
      if (t_raw < -1 || t_raw > 1) {
        return make_error(PackingError::InvalidTritValue, T81String("Trit not in {-1, 0, +1}"));
      }
      int val = t_raw + 1;
      digit += static_cast<uint16_t>(val) * multiplier;
      multiplier *= 3;
    }
    result.push_back(static_cast<uint8_t>(digit));
  }
  return result;
}

Result<std::vector<Trit>> unpack_base81(const std::vector<uint8_t>& digits, size_t trit_count) {
  if (digits.size() < (trit_count + 3) / 4) {
    return make_error(PackingError::InsufficientData,
                      T81String("Insufficient Base-81 digits for trit_count"));
  }

  std::vector<Trit> trits;
  trits.reserve(trit_count);

  for (size_t i = 0; i < digits.size(); ++i) {
    uint16_t val = digits[i];
    if (val > 80) {
      return make_error(PackingError::InvalidBase81Digit, T81String("Base-81 digit > 80"));
    }
    for (size_t j = 0; j < 4; ++j) {
      if (trits.size() < trit_count) {
        int digit = val % 3;
        trits.push_back(static_cast<Trit>(digit - 1));
        val /= 3;
      }
    }
  }
  return trits;
}

Result<std::vector<uint8_t>> pt5_to_b81(const std::vector<uint8_t>& pt5_bytes, size_t trit_count) {
  if (pt5_bytes.size() < (trit_count + 4) / 5) {
    return make_error(PackingError::InsufficientData,
                      T81String("Insufficient PT-5 bytes for trit_count"));
  }

  std::vector<uint8_t> result;
  result.reserve((trit_count + 3) / 4);

  std::deque<Trit> buffer;
  size_t pt5_idx = 0;
  size_t trits_processed = 0;

  while (trits_processed < trit_count) {
    // Fill buffer from PT-5 if needed
    while (buffer.size() < 4 && trits_processed + buffer.size() < trit_count) {
      if (pt5_idx >= pt5_bytes.size()) break;
      uint16_t val = pt5_bytes[pt5_idx++];
      if (val > 242) return make_error(PackingError::InvalidPT5Byte, T81String("PT-5 byte > 242"));
      for (int j = 0; j < 5; ++j) {
        buffer.push_back(static_cast<Trit>((val % 3) - 1));
        val /= 3;
      }
    }

    // Consume 4 trits for one Base-81 digit
    uint16_t b81_val = 0;
    uint16_t multiplier = 1;
    for (int j = 0; j < 4; ++j) {
      int t_raw = 0;  // Default to Z (0)
      if (!buffer.empty() && trits_processed < trit_count) {
        t_raw = static_cast<int>(buffer.front());
        buffer.pop_front();
        trits_processed++;
      }
      if (t_raw < -1 || t_raw > 1) {
        return make_error(PackingError::InvalidTritValue, T81String("Trit not in {-1, 0, +1}"));
      }
      int val = t_raw + 1;
      b81_val += static_cast<uint16_t>(val) * multiplier;
      multiplier *= 3;
    }
    result.push_back(static_cast<uint8_t>(b81_val));
  }

  return result;
}

Result<std::vector<uint8_t>> b81_to_pt5(const std::vector<uint8_t>& b81_digits, size_t trit_count) {
  if (b81_digits.size() < (trit_count + 3) / 4) {
    return make_error(PackingError::InsufficientData,
                      T81String("Insufficient Base-81 digits for trit_count"));
  }

  std::vector<uint8_t> result;
  result.reserve((trit_count + 4) / 5);

  std::deque<Trit> buffer;
  size_t b81_idx = 0;
  size_t trits_processed = 0;

  while (trits_processed < trit_count) {
    // Fill buffer from Base-81 if needed
    while (buffer.size() < 5 && trits_processed + buffer.size() < trit_count) {
      if (b81_idx >= b81_digits.size()) break;
      uint16_t val = b81_digits[b81_idx++];
      if (val > 80)
        return make_error(PackingError::InvalidBase81Digit, T81String("Base-81 digit > 80"));
      for (int j = 0; j < 4; ++j) {
        buffer.push_back(static_cast<Trit>((val % 3) - 1));
        val /= 3;
      }
    }

    // Consume 5 trits for one PT-5 byte
    uint16_t pt5_val = 0;
    uint16_t multiplier = 1;
    for (int j = 0; j < 5; ++j) {
      int t_raw = 0;  // Default to Z (0)
      if (!buffer.empty() && trits_processed < trit_count) {
        t_raw = static_cast<int>(buffer.front());
        buffer.pop_front();
        trits_processed++;
      }
      if (t_raw < -1 || t_raw > 1) {
        return make_error(PackingError::InvalidTritValue, T81String("Trit not in {-1, 0, +1}"));
      }
      int val = t_raw + 1;
      pt5_val += static_cast<uint16_t>(val) * multiplier;
      multiplier *= 3;
    }
    result.push_back(static_cast<uint8_t>(pt5_val));
  }

  return result;
}

std::string b81_digits_to_string(const std::vector<uint8_t>& digits) {
  const auto& alpha = b81_alphabet_vec();
  std::string out;
  for (uint8_t d : digits) {
    if (d < 81) {
      out += alpha[d];
    } else {
      out += "?";  // Should not happen with valid input
    }
  }
  return out;
}

Result<std::vector<uint8_t>> string_to_b81_digits(std::string_view s) {
  const auto& map = b81_alphabet_map();
  std::vector<uint8_t> digits;
  size_t offset = 0;
  while (offset < s.size()) {
    std::string cp = next_utf8(s, offset);
    if (cp.empty()) return make_error(PackingError::EncodingError, T81String("Invalid UTF-8"));
    auto it = map.find(cp);
    if (it == map.end())
      return make_error(PackingError::InvalidBase81Digit, T81String("Character not in alphabet"));
    digits.push_back(it->second);
  }
  return digits;
}

}  // namespace t81::codec::trit_packing
