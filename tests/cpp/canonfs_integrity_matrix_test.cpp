#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "t81/canonfs/canon_driver.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "canonfs_integrity_matrix_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

void set_read_verify(bool enabled) {
#if defined(_WIN32)
  _putenv_s("T81_CANONFS_READ_VERIFY", enabled ? "1" : "0");
#else
  setenv("T81_CANONFS_READ_VERIFY", enabled ? "1" : "0", 1);
#endif
}

void unset_read_verify() {
#if defined(_WIN32)
  _putenv_s("T81_CANONFS_READ_VERIFY", "");
#else
  unsetenv("T81_CANONFS_READ_VERIFY");
#endif
}

std::vector<std::byte> to_bytes(const std::string& s) {
  std::vector<std::byte> out;
  out.reserve(s.size());
  for (char c : s) out.push_back(static_cast<std::byte>(c));
  return out;
}

bool tamper_object_file(const std::filesystem::path& root, const t81::canonfs::CanonRef& ref) {
  const auto path = root / "objects" / (ref.hash.h.to_string() + ".blk");
  std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
  if (!file) return false;
  char byte = 0;
  file.read(&byte, 1);
  if (!file) return false;
  file.seekp(0);
  byte ^= 0x01;
  file.write(&byte, 1);
  file.close();
  return true;
}

bool truncate_object_file(const std::filesystem::path& root, const t81::canonfs::CanonRef& ref,
                          std::size_t keep_bytes) {
  const auto path = root / "objects" / (ref.hash.h.to_string() + ".blk");
  if (!std::filesystem::exists(path)) return false;
  std::error_code ec;
  std::filesystem::resize_file(path, keep_bytes, ec);
  return !ec;
}

bool append_object_file(const std::filesystem::path& root, const t81::canonfs::CanonRef& ref) {
  const auto path = root / "objects" / (ref.hash.h.to_string() + ".blk");
  std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
  if (!file) return false;
  const char pad[2] = {0x55, 0x2A};
  file.write(pad, sizeof(pad));
  file.close();
  return true;
}

}  // namespace

int main() {
  using namespace t81::canonfs;

  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "canonfs-integrity-matrix-test";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  set_read_verify(true);
  auto persistent = make_persistent_driver(root);
  const auto payload = to_bytes("canonfs-integrity-matrix");
  auto write = persistent->write_object(ObjectType::RawBlock,
                                        std::span<const std::byte>(payload.data(), payload.size()));
  if (!expect(write.has_value(), "persistent write failed")) return 1;

  auto first_read = persistent->read_object_bytes(write.value());
  if (!expect(first_read.has_value(), "persistent first read failed")) return 1;
  if (!expect(first_read.value() == payload, "persistent first read payload mismatch")) return 1;

  if (!expect(tamper_object_file(root, write.value()), "tamper object file failed")) return 1;

  // Cache path: existing driver can serve cached copy; hash still verifies against cached bytes.
  auto cached_read = persistent->read_object_bytes(write.value());
  if (!expect(cached_read.has_value(), "cached read should succeed")) return 1;
  if (!expect(cached_read.value() == payload, "cached read should return pre-tamper bytes"))
    return 1;

  // Cold path: new driver must detect tampered bytes via read-verify.
  auto persistent_cold = make_persistent_driver(root);
  auto tampered_read = persistent_cold->read_object_bytes(write.value());
  if (!expect(!tampered_read.has_value(), "tampered cold read should fail")) return 1;
  if (!expect(tampered_read.error() == Error::DecodeError,
              "tampered cold read expects DecodeError")) {
    return 1;
  }

  set_read_verify(false);
  auto persistent_noverify = make_persistent_driver(root);
  auto nonverify_read = persistent_noverify->read_object_bytes(write.value());
  if (!expect(nonverify_read.has_value(), "no-verify read should succeed")) return 1;
  if (!expect(nonverify_read.value() != payload, "no-verify read should expose tampered bytes"))
    return 1;

  // Default path: read-verify should be enabled when env var is unset.
  unset_read_verify();
  auto persistent_default_verify = make_persistent_driver(root);
  auto default_verify_read = persistent_default_verify->read_object_bytes(write.value());
  if (!expect(!default_verify_read.has_value(), "default verify read should fail on tamper"))
    return 1;
  if (!expect(default_verify_read.error() == Error::DecodeError,
              "default verify read expects DecodeError")) {
    return 1;
  }

  // Truncation corruption case.
  set_read_verify(true);
  const std::filesystem::path trunc_root =
      std::filesystem::temp_directory_path() / "canonfs-integrity-matrix-truncate";
  std::filesystem::remove_all(trunc_root);
  std::filesystem::create_directories(trunc_root);
  auto trunc_driver = make_persistent_driver(trunc_root);
  auto trunc_write = trunc_driver->write_object(
      ObjectType::RawBlock, std::span<const std::byte>(payload.data(), payload.size()));
  if (!expect(trunc_write.has_value(), "truncate case write failed")) return 1;
  if (!expect(truncate_object_file(trunc_root, trunc_write.value(), 4), "truncate tamper failed"))
    return 1;
  auto trunc_cold = make_persistent_driver(trunc_root);
  auto trunc_read = trunc_cold->read_object_bytes(trunc_write.value());
  if (!expect(!trunc_read.has_value(), "truncate read should fail")) return 1;
  if (!expect(trunc_read.error() == Error::DecodeError, "truncate read expects DecodeError"))
    return 1;

  // Appended-tail corruption case.
  const std::filesystem::path append_root =
      std::filesystem::temp_directory_path() / "canonfs-integrity-matrix-append";
  std::filesystem::remove_all(append_root);
  std::filesystem::create_directories(append_root);
  auto append_driver = make_persistent_driver(append_root);
  auto append_write = append_driver->write_object(
      ObjectType::RawBlock, std::span<const std::byte>(payload.data(), payload.size()));
  if (!expect(append_write.has_value(), "append case write failed")) return 1;
  if (!expect(append_object_file(append_root, append_write.value()), "append tamper failed"))
    return 1;
  auto append_cold = make_persistent_driver(append_root);
  auto append_read = append_cold->read_object_bytes(append_write.value());
  if (!expect(!append_read.has_value(), "append read should fail")) return 1;
  if (!expect(append_read.error() == Error::DecodeError, "append read expects DecodeError"))
    return 1;

  auto repair_missing = persistent_noverify->parity_repair_subtree(write.value());
  if (!expect(repair_missing.has_value(), "repair existing object should be no-op success"))
    return 1;

  CanonRef bogus{CanonHash{t81::hash::hash_string("missing-object")}};
  auto repair_missing_bogus = persistent_noverify->parity_repair_subtree(bogus);
  if (!expect(!repair_missing_bogus.has_value(), "repair missing object should fail")) return 1;
  if (!expect(repair_missing_bogus.error() == Error::NotFound,
              "repair missing object expects NotFound")) {
    return 1;
  }

  auto inmem = make_in_memory_driver();
  auto inmem_missing_repair = inmem->parity_repair_subtree(bogus);
  if (!expect(!inmem_missing_repair.has_value(), "in-memory missing repair should fail")) return 1;
  if (!expect(inmem_missing_repair.error() == Error::NotFound,
              "in-memory missing repair expects NotFound")) {
    return 1;
  }

  std::filesystem::remove_all(trunc_root);
  std::filesystem::remove_all(append_root);
  std::filesystem::remove_all(root);
  return 0;
}
