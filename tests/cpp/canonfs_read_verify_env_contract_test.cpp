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
    std::cerr << "canonfs_read_verify_env_contract_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

void set_env(const char* value) {
#if defined(_WIN32)
  _putenv_s("T81_CANONFS_READ_VERIFY", value);
#else
  setenv("T81_CANONFS_READ_VERIFY", value, 1);
#endif
}

void unset_env() {
#if defined(_WIN32)
  _putenv_s("T81_CANONFS_READ_VERIFY", "");
#else
  unsetenv("T81_CANONFS_READ_VERIFY");
#endif
}

std::vector<std::byte> make_bytes(std::string_view text) {
  std::vector<std::byte> out;
  out.reserve(text.size());
  for (char c : text) out.push_back(static_cast<std::byte>(c));
  return out;
}

bool tamper_first_byte(const std::filesystem::path& root, const t81::canonfs::CanonRef& ref) {
  const auto object_path = root / "objects" / (ref.hash.h.to_string() + ".blk");
  std::fstream f(object_path, std::ios::in | std::ios::out | std::ios::binary);
  if (!f) return false;
  char first = '\0';
  f.read(&first, 1);
  if (!f) return false;
  f.seekp(0);
  const char flipped = static_cast<char>(first ^ 0x01);
  f.write(&flipped, 1);
  return static_cast<bool>(f);
}

}  // namespace

int main() {
  using namespace t81::canonfs;

  const auto root = std::filesystem::temp_directory_path() / "canonfs-read-verify-env-contract";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  unset_env();
  auto driver = make_persistent_driver(root);
  const auto payload = make_bytes("canonfs-read-verify-env-contract");
  auto write =
      driver->write_object(ObjectType::RawBlock,
                           std::span<const std::byte>(payload.data(), payload.size()));
  if (!expect(write.has_value(), "write_object failed")) return 1;
  driver.reset();

  if (!expect(tamper_first_byte(root, write.value()), "tamper failed")) return 1;

  // Unset -> default verify enabled -> DecodeError on tampered cold read.
  unset_env();
  auto d_unset = make_persistent_driver(root);
  auto r_unset = d_unset->read_object_bytes(write.value());
  if (!expect(!r_unset.has_value(), "unset env should fail tampered read")) return 1;
  if (!expect(r_unset.error() == Error::DecodeError, "unset env expects DecodeError")) return 1;

  // 0 / false / off variants disable verification.
  set_env("0");
  auto d_zero = make_persistent_driver(root);
  auto r_zero = d_zero->read_object_bytes(write.value());
  if (!expect(r_zero.has_value(), "env=0 should allow tampered read")) return 1;

  set_env("false");
  auto d_false = make_persistent_driver(root);
  auto r_false = d_false->read_object_bytes(write.value());
  if (!expect(r_false.has_value(), "env=false should allow tampered read")) return 1;

  set_env("OFF");
  auto d_off = make_persistent_driver(root);
  auto r_off = d_off->read_object_bytes(write.value());
  if (!expect(r_off.has_value(), "env=OFF should allow tampered read")) return 1;

  // Non-zero/non-false values keep verification enabled.
  set_env("1");
  auto d_one = make_persistent_driver(root);
  auto r_one = d_one->read_object_bytes(write.value());
  if (!expect(!r_one.has_value(), "env=1 should fail tampered read")) return 1;
  if (!expect(r_one.error() == Error::DecodeError, "env=1 expects DecodeError")) return 1;

  set_env("garbage");
  auto d_garbage = make_persistent_driver(root);
  auto r_garbage = d_garbage->read_object_bytes(write.value());
  if (!expect(!r_garbage.has_value(), "env=garbage should fail tampered read")) return 1;
  if (!expect(r_garbage.error() == Error::DecodeError, "env=garbage expects DecodeError")) return 1;

  unset_env();
  std::filesystem::remove_all(root);
  return 0;
}
