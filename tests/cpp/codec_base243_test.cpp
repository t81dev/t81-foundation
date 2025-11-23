#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "t81/codec/base243.hpp"

int main() {
  using namespace t81::codec;

  // --- bytes <-> digits roundtrip (big-endian order preserved) ---
  {
    std::vector<uint8_t> bytes = {0x00, 0x01, 0x7F, 0x80, 0xF0, 0xFE};
    auto digits = Base243::encode_bytes_be(bytes);
    // Each digit should be byte % 243 -> for all <243 it's identity
    assert(digits.size() == bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) {
      assert(digits[i] == static_cast<digit_t>(bytes[i] % kBase));
    }
    auto round = Base243::decode_bytes_be(digits);
    assert(round == bytes);
  }

  // Values >= 243 wrap via modulo in encode; decode expects 0..242
  {
    std::vector<uint8_t> bytes = {242, 243, 244, 255}; // encode modulo 243 => {242,0,1,12}
    auto digits = Base243::encode_bytes_be(bytes);
    assert((digits == std::vector<digit_t>{242, 0, 1, static_cast<digit_t>(255 % 243)}));
    auto round = Base243::decode_bytes_be(digits);
    // decode copies digits back to bytes (stub behavior)
    assert(round.size() == digits.size());
    for (size_t i = 0; i < round.size(); ++i) assert(round[i] == digits[i]);
  }

  // --- ASCII helpers (stubbed mapping) ---
  {
    std::string s = "T81-base243";
    auto digits = Base243::encode_ascii(s);
    assert(digits.size() == s.size());
    // decode is lossy inverse but valid for <=242
    auto s2 = Base243::decode_ascii(digits);
    // With pure ASCII, digits are <=127 so roundtrip equals original
    assert(s2 == s);
  }

  // --- decode guard: digit out of range should throw ---
  {
    bool threw = false;
    try {
      std::vector<digit_t> bad = {0, 1, static_cast<digit_t>(244)}; // 244 >= 243
      (void)Base243::decode_bytes_be(bad);
    } catch (const std::invalid_argument&) {
      threw = true;
    }
    assert(threw);
  }

  std::cout << "codec_base243 ok\n";
  return 0;
}
