#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "t81/hash/base81.hpp"
#include "t81/hash/canonhash.hpp"
#include "t81/canonfs.hpp"

int main() {
  using namespace t81;

  // Base-81 stub: encode -> "b81:" + hex; decode reverses it.
  {
    std::vector<uint8_t> bytes = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11};
    std::string enc = hash::encode_base81(bytes);
    // Expect "b81:" prefix
    assert(enc.rfind("b81:", 0) == 0);
    auto round = hash::decode_base81(enc);
    assert(round == bytes);
  }

  // CanonHash81 stub: encode -> base81 stub, truncated/padded to 81 chars.
  {
    std::string payload = "hello canonhash base81 stub";
    CanonHash81 h = hash::make_canonhash81_base81stub(payload);
    std::string s = h.to_string();
    // Should be <= 81 chars (exactly 81 unless input very short)
    assert(s.size() <= 81);
    // Must start with b81: (since stub uses base81 stub)
    assert(s.rfind("b81:", 0) == 0);
  }

  // CanonHash81::from_string / to_string roundtrip (trunc/pad semantics)
  {
    std::string in = "b81:feedfacecafef00d";
    CanonHash81 h = CanonHash81::from_string(in);
    assert(h.to_string() == in);
  }

  std::cout << "hash_stub ok\n";
  return 0;
}
