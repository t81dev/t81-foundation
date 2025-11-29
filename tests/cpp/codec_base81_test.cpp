#include <cassert>
#include <string>
#include <vector>
#include <t81/codec/base81.hpp>
#include <t81/core/base81.hpp>

int main() {
  using namespace t81::codec::base81;

  // Roundtrip bytes (leading zeros are not preserved by the canonical integer codec)
  {
    std::vector<uint8_t> bytes = {0xFF, 0x10};
    auto enc = encode_bytes(bytes);
    std::vector<uint8_t> dec;
    bool ok = decode_bytes(enc, dec);
    assert(ok);
    assert(dec == bytes);
    assert(t81::is_base81(enc));
  }

  // Empty input
  {
    std::vector<uint8_t> out;
    bool ok = decode_bytes("", out);
    assert(ok);
    assert(out.empty());
  }

  // Invalid character
  {
    std::vector<uint8_t> out;
    bool ok = decode_bytes("~", out); // '~' not in canonical alphabet
    assert(!ok);
    assert(!t81::core::is_base81("~"));
  }

  // Non-canonical leading zero should fail
  {
    std::vector<uint8_t> out;
    bool ok = decode_bytes("00", out);
    assert(!ok);
  }

  // Multi-byte codepoint correctness (uses UTF-8 symbols from the alphabet)
  {
    std::vector<uint8_t> bytes = {0x12, 0x34};
    auto enc = encode_bytes(bytes);
    // Ensure encoded string isn't empty and contains multi-byte chars for high digits
    assert(!enc.empty());
    std::vector<uint8_t> dec;
    bool ok = decode_bytes(enc, dec);
    assert(ok);
    assert(dec == bytes);
  }

  return 0;
}
