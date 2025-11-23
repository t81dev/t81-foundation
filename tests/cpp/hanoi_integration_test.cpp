#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "t81/t81.hpp"

int main() {
  auto driver = t81::canonfs::make_in_memory_driver();
  auto kernel = t81::hanoi::make_in_memory_kernel(*driver);

  t81::hanoi::SnapshotRef base{t81::canonfs::CanonHash{"root"}};
  auto forked = kernel->fork_snapshot(base);
  assert(forked.has_value());

  std::string payload = "hello";
  std::vector<std::byte> bytes(payload.size());
  std::memcpy(bytes.data(), payload.data(), payload.size());

  auto obj = driver->write_object(t81::canonfs::ObjectType::Blob, bytes);
  assert(obj.has_value());

  auto committed = kernel->commit_snapshot(forked.value());
  assert(committed.has_value());
  auto switched = kernel->switch_root(committed.value());
  assert(switched.has_value());

  auto read_back = kernel->read_object(obj.value());
  assert(read_back.has_value());
  auto& buffer = read_back.value();
  std::string roundtrip(reinterpret_cast<char*>(buffer.data()), buffer.size());
  assert(roundtrip == payload);

  return 0;
}

