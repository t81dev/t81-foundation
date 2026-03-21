#include "t81/codec/base81_packed.hpp"
#include <array>

namespace t81::codec::base81_packed {

struct Digits2 {
  uint8_t d0, d1;
};

static const std::array<Digits2, 6561>& get_unpack_lut() {
  static const auto lut = [] {
    std::array<Digits2, 6561> l{};
    for (int i = 0; i < 6561; ++i) {
      l[i].d0 = static_cast<uint8_t>(i % 81);
      l[i].d1 = static_cast<uint8_t>(i / 81);
    }
    return l;
  }();
  return lut;
}

uint32_t pack5(const uint8_t digits[5]) {
  uint32_t res = 0;
  uint32_t mult = 1;
  for (int i = 0; i < 5; ++i) {
    res += static_cast<uint32_t>(digits[i]) * mult;
    mult *= 81;
  }
  return res;
}

void unpack5(uint32_t packed, uint8_t digits[5]) {
  const auto& lut = get_unpack_lut();

  uint32_t chunk0 = packed % 6561;
  packed /= 6561;
  uint32_t chunk1 = packed % 6561;
  packed /= 6561;
  uint32_t d4 = packed;  // remaining is d4

  digits[0] = lut[chunk0].d0;
  digits[1] = lut[chunk0].d1;
  digits[2] = lut[chunk1].d0;
  digits[3] = lut[chunk1].d1;
  digits[4] = static_cast<uint8_t>(d4);
}

std::vector<uint32_t> pack_stream(std::span<const uint8_t> digits) {
  size_t n = (digits.size() + 4) / 5;
  std::vector<uint32_t> blocks;
  blocks.reserve(n);

  for (size_t i = 0; i < digits.size(); i += 5) {
    uint8_t block[5] = {0, 0, 0, 0, 0};
    for (size_t j = 0; j < 5; ++j) {
      if (i + j < digits.size()) {
        block[j] = digits[i + j];
      }
    }
    blocks.push_back(pack5(block));
  }
  return blocks;
}

std::vector<uint8_t> unpack_stream(std::span<const uint32_t> blocks, size_t original_count) {
  std::vector<uint8_t> digits;
  digits.reserve(blocks.size() * 5);

  for (uint32_t b : blocks) {
    uint8_t block[5];
    unpack5(b, block);
    for (int j = 0; j < 5; ++j) {
      if (digits.size() < original_count) {
        digits.push_back(block[j]);
      }
    }
  }
  return digits;
}

}  // namespace t81::codec::base81_packed
