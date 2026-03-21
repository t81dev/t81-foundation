#include "t81/codec/base81_balanced.hpp"
#include <algorithm>
#include <cmath>

namespace t81::codec::base81_balanced {

int8_t pack_digit(const Trit trits[4]) {
  int val = 0;
  int mult = 1;
  for (int i = 0; i < 4; ++i) {
    val += static_cast<int>(trits[i]) * mult;
    mult *= 3;
  }
  return static_cast<int8_t>(val);
}

void unpack_digit(int8_t digit, Trit trits[4]) {
  int val = digit;
  for (int i = 0; i < 4; ++i) {
    int r = val % 3;
    if (r > 1) {
      r -= 3;
      val = val / 3 + 1;
    } else if (r < -1) {
      r += 3;
      val = val / 3 - 1;
    } else {
      val /= 3;
    }
    trits[i] = static_cast<Trit>(r);
  }
}

Result<size_t> pack(std::span<const Trit> trits, std::span<int8_t> digits) {
  size_t num_digits = (trits.size() + 3) / 4;
  if (digits.size() < num_digits) {
    return T81Error(T81Symbol::intern("INSUFFICIENT_BUFFER"),
                    T81String("Output digits buffer too small"));
  }

  for (size_t i = 0; i < num_digits; ++i) {
    Trit block[4] = {Trit::Z, Trit::Z, Trit::Z, Trit::Z};
    for (size_t j = 0; j < 4; ++j) {
      size_t idx = i * 4 + j;
      if (idx < trits.size()) {
        block[j] = trits[idx];
      }
    }
    digits[i] = pack_digit(block);
  }
  return num_digits;
}

Result<size_t> unpack(std::span<const int8_t> digits, std::span<Trit> trits, size_t trit_count) {
  size_t num_digits = (trit_count + 3) / 4;
  if (digits.size() < num_digits) {
    return T81Error(T81Symbol::intern("INSUFFICIENT_DATA"), T81String("Insufficient input digits"));
  }
  if (trits.size() < trit_count) {
    return T81Error(T81Symbol::intern("INSUFFICIENT_BUFFER"),
                    T81String("Output trits buffer too small"));
  }

  for (size_t i = 0; i < num_digits; ++i) {
    Trit block[4];
    if (digits[i] < -40 || digits[i] > 40) {
      return T81Error(T81Symbol::intern("INVALID_DIGIT"),
                      T81String("Balanced Base-81 digit out of range [-40, 40]"));
    }
    unpack_digit(digits[i], block);
    for (size_t j = 0; j < 4; ++j) {
      size_t idx = i * 4 + j;
      if (idx < trit_count) {
        trits[idx] = block[j];
      }
    }
  }
  return trit_count;
}

Result<std::vector<int8_t>> pack_vector(const std::vector<Trit>& trits) {
  std::vector<int8_t> digits((trits.size() + 3) / 4);
  auto res = pack(trits, digits);
  if (!res) return res.error();
  return digits;
}

Result<std::vector<Trit>> unpack_vector(const std::vector<int8_t>& digits, size_t trit_count) {
  std::vector<Trit> trits(trit_count);
  auto res = unpack(digits, trits, trit_count);
  if (!res) return res.error();
  return trits;
}

void to_balanced(std::span<const uint8_t> unbalanced, std::span<int8_t> balanced) {
  size_t n = std::min(unbalanced.size(), balanced.size());
  for (size_t i = 0; i < n; ++i) {
    balanced[i] = static_cast<int8_t>(static_cast<int>(unbalanced[i]) - 40);
  }
}

void to_unbalanced(std::span<const int8_t> balanced, std::span<uint8_t> unbalanced) {
  size_t n = std::min(balanced.size(), unbalanced.size());
  for (size_t i = 0; i < n; ++i) {
    unbalanced[i] = static_cast<uint8_t>(static_cast<int>(balanced[i]) + 40);
  }
}

}  // namespace t81::codec::base81_balanced
