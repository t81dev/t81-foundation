#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/storage_key.hpp"
#include "test_runtime_check.hpp"

namespace fs = std::filesystem;
using namespace t81::canonfs;

static void write_file(const fs::path& path, const std::string& text) {
  std::ofstream out(path, std::ios::binary);
  out << text;
  out.close();
  T81_TEST_CHECK(!out.fail());
}

static void expect_rejected(const fs::path& root) {
  bool rejected = false;
  try {
    auto driver = make_persistent_driver(root);
  } catch (const std::runtime_error&) {
    rejected = true;
  }
  T81_TEST_CHECK(rejected);
}

int main() {
  // Golden codec vector, independent of the production hash function.
  CanonHash sequence{};
  for (std::size_t i = 0; i < sequence.h.bytes.size(); ++i)
    sequence.h.bytes[i] = static_cast<std::uint8_t>(i);
  T81_TEST_CHECK(storage_key_v1(sequence) ==
                 "h1-000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
  for (unsigned i = 0; i < 256; ++i) {
    CanonHash hash{};
    hash.h.bytes.fill(static_cast<std::uint8_t>(i));
    const auto key = storage_key_v1(hash);
    T81_TEST_CHECK(key.size() == 67);
    T81_TEST_CHECK(key.find_first_of("<>:\"/\\|?* .") == std::string::npos);
    T81_TEST_CHECK(parse_storage_key_v1(key) == hash);
    if (i) T81_TEST_CHECK(key != storage_key_v1(CanonHash{}));
  }
  T81_TEST_CHECK(!parse_storage_key_v1("h1-00"));
  auto invalid = storage_key_v1(sequence);
  invalid[4] = 'A';
  T81_TEST_CHECK(!parse_storage_key_v1(invalid));
  invalid[4] = '/';
  T81_TEST_CHECK(!parse_storage_key_v1(invalid));

  const auto root = fs::temp_directory_path() / "canonfs-portable-format-test";
  fs::remove_all(root);
  auto persistent = make_persistent_driver(root);
  auto memory = make_in_memory_driver();
  const std::vector<std::pair<std::string, std::string>> vectors = {
      // Python hashlib.sha3_512(payload).digest()[:32].hex(), not candidate-generated.
      {"canonfs-read-verify-payload",
       "c678fce92c9a2b0123a432748bac351265159b942fed116abb2f3ddb90db1a67"},
      {"canonfs-identity-contract",
       "eb517f3ce0404a54bba5139e9006a34b4fc3397a6767277f73eda535b8a85c6e"}};
  for (const auto& [payload, expected_hex] : vectors) {
    const auto bytes = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(payload.data()), payload.size());
    auto first = persistent->write_object(ObjectType::RawBlock, bytes);
    auto repeated = persistent->write_object(ObjectType::RawBlock, bytes);
    auto independent_driver = memory->write_object(ObjectType::RawBlock, bytes);
    T81_TEST_CHECK(first && repeated && independent_driver);
    T81_TEST_CHECK(first->hash == repeated->hash && first->hash == independent_driver->hash);
    T81_TEST_CHECK(storage_key_v1(first->hash) == "h1-" + expected_hex);
    T81_TEST_CHECK(first->hash.h.to_string().find_first_of("<>") != std::string::npos);
    T81_TEST_CHECK(fs::exists(root / "objects" / ("h1-" + expected_hex + ".blk")));
    persistent.reset();
    persistent = make_persistent_driver(root);
    auto read = persistent->read_object_bytes(*first);
    T81_TEST_CHECK(read && *read == std::vector<std::byte>(bytes.begin(), bytes.end()));
    CapabilityGrant grant{};
    grant.target = *first;
    grant.perms = 3;
    T81_TEST_CHECK(persistent->publish_capability(grant));
    T81_TEST_CHECK(fs::exists(root / "caps" / ("h1-" + expected_hex + ".cap")));
    persistent.reset();
    persistent = make_persistent_driver(root);
    T81_TEST_CHECK(persistent->read_object_bytes(*first));
    // Restore bootstrap mode for the next independent payload fixture.
    T81_TEST_CHECK(persistent->revoke_capability(*first));
    persistent.reset();
    persistent = make_persistent_driver(root);
  }
  persistent.reset();
  write_file(root / "caps" / "legacy.cap", "3");
  expect_rejected(root);  // Never ignore old policy names in an otherwise marked store.
  fs::remove_all(root);
  fs::create_directories(root / "objects");
  write_file(root / "objects" / "legacy.blk", "preserve-me");
  expect_rejected(root);
  T81_TEST_CHECK(fs::file_size(root / "objects" / "legacy.blk") == 11);
  T81_TEST_CHECK(!fs::exists(root / kStorageFormatFile));
  fs::remove_all(root);
  fs::create_directories(root);
  for (const auto& marker : {std::string("unknown\n"), std::string(kStorageFormatV1) + "extra",
                             std::string("t81-canonfs-hex-")}) {
    write_file(root / kStorageFormatFile, marker);
    expect_rejected(root);
  }
  fs::remove_all(root);
  std::cout << "portable storage codec, independent hashes, reopen, capabilities, migration "
               "boundary: PASS\n";
}
