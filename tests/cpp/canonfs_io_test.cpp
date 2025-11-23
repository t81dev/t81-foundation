#include <cassert>
#include <cstring>
#include <iostream>
#include "t81/canonfs.hpp"
#include "t81/canonfs_io.hpp"

int main() {
  using namespace t81;

  // Build a CanonRef
  CanonHash81 h = CanonHash81::from_string("b81:deadbeef0123456789");
  CanonRef ref = CanonRef::make(h, CANON_PERM_READ | CANON_PERM_WRITE, 0x1122334455667788ull);

  // Encode → bytes[99]
  uint8_t buf[t81::canonfs_io::kWireSize];
  t81::canonfs_io::encode_ref(ref, buf);

  // Decode → back
  CanonRef got = t81::canonfs_io::decode_ref(buf);

  // Check roundtrip
  assert(got.permissions == (CANON_PERM_READ | CANON_PERM_WRITE));
  assert(got.expires_at == 0x1122334455667788ull);
  assert(got.target.to_string() == "b81:deadbeef0123456789");

  // Permission helper
  assert(t81::canonfs_io::permissions_allow(got.permissions, CANON_PERM_READ));
  assert(!t81::canonfs_io::permissions_allow(got.permissions, CANON_PERM_ADMIN));

  std::cout << "canonfs_io ok\n";
  return 0;
}
