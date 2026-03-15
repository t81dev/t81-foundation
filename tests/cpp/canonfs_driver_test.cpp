#include <iostream>
#include <string>
#include <t81/canonfs/canon_driver.hpp>
#include <t81/tracing/canonhash.hpp>
#include <vector>

int main() {
  using namespace t81::canonfs;
  auto expect = [](bool cond, const char* msg) -> bool {
    if (!cond) {
      std::cerr << "canonfs_driver_test failure: " << msg << "\n";
      return false;
    }
    return true;
  };

  [[maybe_unused]] auto driver = make_in_memory_driver();
  const std::string payload = "hello-canonfs";
  [[maybe_unused]] auto ref_res =
      driver->write_object(ObjectType::RawBlock,
                           std::span<const std::byte>(
                               reinterpret_cast<const std::byte*>(payload.data()), payload.size()));
  if (!expect(ref_res.has_value(), "write_object failed")) return 1;
  [[maybe_unused]] auto ref = ref_res.value();
  [[maybe_unused]] auto read = driver->read_object_bytes(ref);
  if (!expect(read.has_value(), "read_object_bytes failed")) return 1;
  std::string back(reinterpret_cast<const char*>(read.value().data()), read.value().size());
  if (!expect(back == payload, "read payload mismatch")) return 1;

  // Rewriting the same bytes should yield the same hash (content addressable)
  [[maybe_unused]] auto ref_res2 =
      driver->write_object(ObjectType::RawBlock,
                           std::span<const std::byte>(
                               reinterpret_cast<const std::byte*>(payload.data()), payload.size()));
  if (!expect(ref_res2.has_value(), "second write_object failed")) return 1;
  if (!expect(ref_res2.value().hash == ref.hash, "content-addressable hash mismatch")) return 1;

  // Capability enforcement: publish only for ref, then access succeeds; unknown should fail.
  CapabilityGrant grant{ref, {"userA", "pk-userA"}, CANON_PERM_READ | CANON_PERM_WRITE, {}, 0, {}};
  [[maybe_unused]] auto cap_res = driver->publish_capability(grant);
  if (!expect(cap_res.has_value(), "publish_capability failed")) return 1;
  [[maybe_unused]] auto read_authed = driver->read_object_bytes(ref);
  if (!expect(read_authed.has_value(), "authorized read failed")) return 1;

  CanonRef no_cap{
      ref.hash};  // same hash, but cap exists; still allowed because capability was published.
  [[maybe_unused]] auto read_again = driver->read_object_bytes(no_cap);
  if (!expect(read_again.has_value(), "read with existing capability failed")) return 1;

  CanonRef bogus{CanonHash{t81::hash::hash_string("bogus")}};
  [[maybe_unused]] auto miss_cap = driver->read_object_bytes(bogus);
  if (!expect(!miss_cap.has_value(), "bogus read unexpectedly succeeded")) return 1;
  if (!expect(miss_cap.error() == Error::CapabilityError, "bogus read error mismatch")) return 1;

  return 0;
}
