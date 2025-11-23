#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "t81/canonfs.hpp"
#include "t81/hash/base81.hpp"
#include "t81/hash/canonhash.hpp"

int main() {
  using namespace t81;

  // ---- Base-81 stub encode/decode roundtrip ----
  std::vector<uint8_t> bytes = {0x00, 0x01, 0xAB, 0xCD, 0xEF, 0x7F};
  std::string enc = hash::encode_base81(bytes);
  auto dec = hash::decode_base81(enc);
  assert(dec == bytes);

  // ---- CanonHash81 stub generation ----
  const char* payload = "hello-world";
  auto h = hash::make_canonhash81_base81stub(payload, std::strlen(payload));

  // The stub encoder prefixes "b81:", so ensure we copied something plausible.
  // We only check that the prefix appears and buffer is zero-padded beyond enc length.
  const char prefix[] = "b81:";
  assert(std::memcmp(h.text.data(), prefix, sizeof(prefix) - 1) == 0);

  // Ensure no out-of-bounds and that text is at most 81 bytes.
  static_assert(sizeof(h.text) == 81, "CanonHash81.text must be 81 bytes");

  // Basic zero padding sanity: find first zero after non-zero run.
  bool saw_zero = false;
  for (size_t i = 0; i < h.text.size(); ++i) {
    if (h.text[i] == 0) { saw_zero = true; break; }
  }
  assert(saw_zero);

  std::cout << "hash_stub ok\n";
  return 0;
}
