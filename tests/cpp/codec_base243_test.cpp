#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "t81/codec/base243.hpp"

int main() {
  using namespace t81::codec;

  // --- bytes <-> digits roundtrip (big-endian order preserved) ---
  {
    std::vector<uint8_t> bytes = {0xFF}; // 255 = 1*243 + 12 -> digits {1,12}
    auto digits = Base243::encode_bytes_be(bytes);
    assert((digits == std::vector<digit_t>{1, 12}));
    auto round = Base243::decode_bytes_be(digits);
    assert(round == bytes);
  }

  {
    std::vector<uint8_t> bytes = {0x01, 0x00}; // 256 = 1*243 + 13 -> digits {1,13}
    auto digits = Base243::encode_bytes_be(bytes);
    assert((digits == std::vector<digit_t>{1, 13}));
    auto round = Base243::decode_bytes_be(digits);
    assert(round == bytes);
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

  // --- bigint roundtrip ---
  {
    t81::T243BigInt a = t81::T243BigInt::from_i64(123456);
    auto s = Base243::encode_bigint(a);
    t81::T243BigInt b;
    bool ok = Base243::decode_bigint(s, b);
    assert(ok);
    assert(a == b);

    t81::T243BigInt neg = t81::T243BigInt::from_i64(-999);
    auto sn = Base243::encode_bigint(neg);
    t81::T243BigInt back;
    ok = Base243::decode_bigint(sn, back);
    assert(ok);
    assert(neg == back);
  }

  std::cout << "codec_base243 ok\n";
  return 0;
}
