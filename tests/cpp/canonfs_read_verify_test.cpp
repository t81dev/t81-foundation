#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "t81/canonfs/canon_driver.hpp"

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "canonfs_read_verify_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

void set_read_verify_env(bool enabled) {
#if defined(_WIN32)
  _putenv_s("T81_CANONFS_READ_VERIFY", enabled ? "1" : "0");
#else
  setenv("T81_CANONFS_READ_VERIFY", enabled ? "1" : "0", 1);
#endif
}

std::vector<std::byte> make_bytes(std::string_view text) {
  std::vector<std::byte> out;
  out.reserve(text.size());
  for (char c : text) {
    out.push_back(static_cast<std::byte>(c));
  }
  return out;
}

}  // namespace

int main() {
  using namespace t81::canonfs;

  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "canonfs-read-verify-test";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  set_read_verify_env(true);

  auto driver = make_persistent_driver(root);
  const auto payload = make_bytes("canonfs-read-verify-payload");
  auto write_res = driver->write_object(ObjectType::RawBlock,
                                        std::span<const std::byte>(payload.data(), payload.size()));
  if (!expect(write_res.has_value(), "write_object failed")) return 1;

  const auto hash_text = write_res.value().hash.h.to_string();
  driver.reset();

  const auto object_path = root / "objects" / (hash_text + ".blk");
  std::fstream mut(object_path, std::ios::in | std::ios::out | std::ios::binary);
  if (!expect(static_cast<bool>(mut), "open object for tamper failed")) return 1;
  char first = '\0';
  mut.read(&first, 1);
  if (!expect(static_cast<bool>(mut), "read tamper byte failed")) return 1;
  mut.seekp(0);
  const char flipped = static_cast<char>(first ^ 0x01);
  mut.write(&flipped, 1);
  mut.close();

  auto verify_driver = make_persistent_driver(root);
  auto verify_read = verify_driver->read_object_bytes(write_res.value());
  if (!expect(!verify_read.has_value(), "read unexpectedly succeeded after tamper")) return 1;
  if (!expect(verify_read.error() == Error::DecodeError, "expected DecodeError on hash mismatch")) {
    return 1;
  }

  set_read_verify_env(false);
  auto non_verify_driver = make_persistent_driver(root);
  auto non_verify_read = non_verify_driver->read_object_bytes(write_res.value());
  if (!expect(non_verify_read.has_value(), "non-verify read should succeed")) return 1;
  if (!expect(non_verify_read.value() != payload, "tamper should alter payload bytes")) return 1;

  std::filesystem::remove_all(root);
  return 0;
}
