#include <cstddef>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>

#include "t81/canonfs/canon_driver.hpp"

namespace {
bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "canonfs_identity_contract_test failure: " << message << "\n";
    return false;
  }
  return true;
}

std::span<const std::byte> bytes_of(const std::string& text) {
  return std::span<const std::byte>(reinterpret_cast<const std::byte*>(text.data()), text.size());
}
}  // namespace

int main() {
  using namespace t81::canonfs;

  const std::string payload = "canonfs-identity-contract";

  {
    auto driver = make_in_memory_driver();
    auto raw_ref = driver->write_object(ObjectType::RawBlock, bytes_of(payload));
    auto tensor_ref = driver->write_object(ObjectType::CanonTensor, bytes_of(payload));
    if (!expect(raw_ref.has_value(), "in-memory raw write failed")) return 1;
    if (!expect(tensor_ref.has_value(), "in-memory tensor write failed")) return 1;
    if (!expect(raw_ref->hash == tensor_ref->hash,
                "in-memory driver identity unexpectedly depends on ObjectType")) {
      return 1;
    }
  }

  {
    const auto root = std::filesystem::temp_directory_path() / "canonfs-identity-contract-test";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (!expect(!ec, "persistent test root create failed")) return 1;

    auto driver = make_persistent_driver(root);
    auto raw_ref = driver->write_object(ObjectType::RawBlock, bytes_of(payload));
    auto tensor_ref = driver->write_object(ObjectType::CanonTensor, bytes_of(payload));
    if (!expect(raw_ref.has_value(), "persistent raw write failed")) return 1;
    if (!expect(tensor_ref.has_value(), "persistent tensor write failed")) return 1;
    if (!expect(raw_ref->hash == tensor_ref->hash,
                "persistent driver identity unexpectedly depends on ObjectType")) {
      return 1;
    }

    std::filesystem::remove_all(root, ec);
    if (!expect(!ec, "persistent test root cleanup failed")) return 1;
  }

  return 0;
}
