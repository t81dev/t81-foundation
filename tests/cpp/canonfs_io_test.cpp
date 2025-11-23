#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "t81/canonfs.hpp"
#include "t81/canonfs_io.hpp"

int main() {
  using namespace t81;

  // Build a sample ref
  CanonRef ref{};
  const char* hash = "ABCDEF0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+-=[]{}";
  std::memset(ref.target.text.data(), 0, ref.target.text.size());
  std::memcpy(ref.target.text.data(), hash, std::min<size_t>(std::strlen(hash), ref.target.text.size()));
  ref.permissions = 0b1010'0110'0001'1111u; // arbitrary mask
  ref.expires_at = 0x1122334455667788ull;

  // Encode
  std::vector<uint8_t> buf = canonfs_io::encode_ref(ref);
  assert(buf.size() == 99);

  // Decode
  CanonRef got = canonfs_io::decode_ref(buf.data(), buf.size());

  // Check roundtrip
  assert(std::memcmp(got.target.text.data(), ref.target.text.data(), ref.target.text.size()) == 0);
  assert(got.permissions == ref.permissions);
  assert(got.expires_at == ref.expires_at);

  // permissions_allow helper
  uint16_t need = 0b0000'0000'0001'1111u;
  assert(canonfs_io::permissions_allow(ref.permissions, need));
  assert(!canonfs_io::permissions_allow(ref.permissions, 0b0100'0000'0000'0000u));

  std::cout << "canonfs_io ok\n";
  return 0;
}
