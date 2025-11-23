#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "t81/hash/base81.hpp"
#include "t81/hash/canonhash.hpp"

int main() {
  using namespace t81;

  // ---------------------------------------------------------------------------
  // Base-81: encode/decode roundtrip over arbitrary bytes.
  // ---------------------------------------------------------------------------
  {
    std::vector<std::uint8_t> bytes = {
        0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11,
        0x00, 0x00, 0xFF, 0x7F
    };

    std::string enc = hash::encode_base81(bytes);
    // Should not be empty for non-empty input.
    assert(!enc.empty());

    // Roundtrip must exactly recover original bytes.
    std::vector<std::uint8_t> round = hash::decode_base81(enc);
    assert(round == bytes);
  }

  // ---------------------------------------------------------------------------
  // CanonHash81: deterministic hashing + base-81 representation.
  // ---------------------------------------------------------------------------
  {
    std::string payload = "hello canonhash base81";

    hash::CanonHash81 h1 = hash::hash_string(payload);
    hash::CanonHash81 h2 = hash::hash_string(payload);

    // Same input -> same hash.
    assert(h1 == h2);

    // Simple difference sanity check.
    hash::CanonHash81 hA = hash::hash_string("A");
    hash::CanonHash81 hB = hash::hash_string("B");
    assert(hA != hB);

    // String representation roundtrip.
    std::string s = h1.to_string();
    assert(!s.empty());

    hash::CanonHash81 h_round = hash::CanonHash81::from_string(s);
    assert(h_round == h1);
  }

  // ---------------------------------------------------------------------------
  // CanonHash81: from_string(to_string(h)) is identity for valid hashes.
  // ---------------------------------------------------------------------------
  {
    hash::CanonHash81 h = hash::hash_string("roundtrip sentinel");
    std::string encoded = h.to_string();

    hash::CanonHash81 parsed = hash::CanonHash81::from_string(encoded);
    assert(parsed == h);
    assert(parsed.to_string() == encoded);
  }

  std::cout << "hash tests ok\n";
  return 0;
}
