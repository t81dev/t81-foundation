#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <t81/codec/base81.hpp>
#include <t81/types/base81.hpp>

int main() {
  using namespace t81::codec::base81;

  assert(digit_strings().size() == 81);
  assert(signed_integer_digit_strings().size() == 81);
  assert(digit_strings()[62] == "-");
  assert(signed_integer_digit_strings()[62] == "+");

  // Roundtrip bytes (leading zeros are not preserved by the canonical integer codec)
  {
    [[maybe_unused]] std::vector<std::uint8_t> bytes = {0xFFu, 0x10u};
    [[maybe_unused]] auto enc = encode_bytes(bytes);
    [[maybe_unused]] std::vector<std::uint8_t> dec;
    [[maybe_unused]] bool ok = decode_bytes(enc, dec);
    assert(ok);
    assert(dec == bytes);
    assert(t81::core::is_base81(enc));
  }

  // Empty input
  {
    [[maybe_unused]] std::vector<std::uint8_t> out;
    [[maybe_unused]] bool ok = decode_bytes("", out);
    assert(ok);
    assert(out.empty());
  }

  // Invalid character
  {
    [[maybe_unused]] std::vector<std::uint8_t> out;
    [[maybe_unused]] bool ok = decode_bytes("~", out);  // '~' not in canonical alphabet
    assert(!ok);
    assert(!t81::core::is_base81("~"));
  }

  // Non-canonical leading zero should fail
  {
    [[maybe_unused]] std::vector<std::uint8_t> out;
    [[maybe_unused]] bool ok = decode_bytes("00", out);
    assert(!ok);
  }

  // Multi-byte codepoint correctness (uses UTF-8 symbols from the alphabet)
  {
    [[maybe_unused]] std::vector<std::uint8_t> bytes = {0x12u, 0x34u};
    [[maybe_unused]] auto enc = encode_bytes(bytes);
    assert(!enc.empty());  // sanity check

    [[maybe_unused]] std::vector<std::uint8_t> dec;
    [[maybe_unused]] bool ok = decode_bytes(enc, dec);
    assert(ok);
    assert(dec == bytes);
  }

  return 0;
}
