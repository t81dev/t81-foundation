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

  // ---------------------------------------------------------------------------
  // Base-81 codec: encode/decode roundtrip over arbitrary bytes.
  // Long-term contract:
  //   - encode_base81 is deterministic.
  //   - decode_base81(encode_base81(x)) == x for all byte sequences x.
  //   - No assumptions about prefixes like "b81:" or fixed length.
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
  // CanonHash81: hashing + base-81 representation.
  //
  // Long-term contract:
  //   - hash_string(s) is deterministic.
  //   - Equal inputs => equal hashes.
  //   - Different simple inputs => almost certainly different hashes.
  //   - CanonHash81::to_string() encodes internal bytes via base-81.
  //   - CanonHash81::from_string(to_string(h)) == h.
  // ---------------------------------------------------------------------------
  {
    std::string payload = "hello canonhash base81";

    // Real hashing API: you can alias this to your current stub until
    // the implementation is upgraded.
    CanonHash81 h1 = hash::hash_string(payload);
    CanonHash81 h2 = hash::hash_string(payload);

    // Deterministic: same input, same hash.
    assert(h1 == h2);

    // Different inputs should give different hashes (sanity check).
    CanonHash81 hA = hash::hash_string("A");
    CanonHash81 hB = hash::hash_string("B");
    assert(!(hA == hB));

    // String representation roundtrip.
    std::string s = h1.to_string();
    assert(!s.empty());

    CanonHash81 h_round = CanonHash81::from_string(s);
    assert(h_round == h1);
  }

  // ---------------------------------------------------------------------------
  // CanonHash81: from_string / to_string roundtrip on arbitrary base-81 strings.
  //
  // Long-term contract:
  //   - If `in` is a valid CanonHash81 base-81 representation,
  //     then CanonHash81::from_string(in).to_string() == in.
  //   - For now, we only test roundtrip, not a specific literal value.
  // ---------------------------------------------------------------------------
  {
    // Generate a valid representation via the API itself instead of hard-coding
    // stub-era strings like "b81:feedfacecafef00d".
    CanonHash81 h = hash::hash_string("roundtrip sentinel");
    std::string in = h.to_string();

    CanonHash81 parsed = CanonHash81::from_string(in);
    assert(parsed == h);
    assert(parsed.to_string() == in);
  }

  std::cout << "hash tests ok\n";
  return 0;
}
