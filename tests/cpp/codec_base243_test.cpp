#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include "t81/codec/base243.hpp"

int main() {
  using namespace t81::codec;

  // bytes -> digits -> bytes roundtrip (stub mapping)
  std::vector<uint8_t> bytes = {0x00, 0x7F, 0xF0, 0xAB, 0xCD, 0xEF};
  auto digits = Base243::encode_bytes_be(bytes);
  auto round  = Base243::decode_bytes_be(digits);
  assert(round == bytes);

  // ascii -> digits -> ascii (stub mapping)
  std::string s = "Hello-243!";
  auto d2 = Base243::encode_ascii(s);
  auto s2 = Base243::decode_ascii(d2);
  // In the stub, encode_ascii is modulo 243; all chars < 243 so exact roundtrip.
  assert(s2.size() == s.size());
  for (size_t i = 0; i < s.size(); ++i) assert(static_cast<unsigned char>(s2[i]) == static_cast<unsigned char>(s[i]));

  std::cout << "codec_base243 ok\n";
  return 0;
}
