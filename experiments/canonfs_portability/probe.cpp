#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/storage_mapping.hpp"

std::string hex(std::span<const std::byte> data) {
  std::ostringstream out;
  for (auto b : data)
    out << std::hex << std::setfill('0') << std::setw(2) << std::to_integer<int>(b);
  return out.str();
}
int main(int argc, char** argv) {
  if (argc != 4) return 2;
  try {
    const bool portable = std::string(argv[1]) == "portable";
    if (!portable && std::string(argv[1]) != "legacy") return 2;
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) return 2;
    const std::string text{std::istreambuf_iterator<char>(input), {}};
    const auto bytes = std::as_bytes(std::span(text.data(), text.size()));
    const auto expected = t81::canonfs::CanonHash{t81::hash::hash_bytes(bytes)};
    const std::filesystem::path root = argv[3];
    if (std::filesystem::exists(root)) return 2;
    auto create = [&] {
      return portable ? t81::canonfs::make_portable_persistent_driver(root)
                      : t81::canonfs::make_persistent_driver(root);
    };
    auto driver = create();
    const auto result = driver->write_object(t81::canonfs::ObjectType::RawBlock, bytes);
    bool read_ok = false, duplicate_ok = false;
    if (result) {
      driver.reset();
      driver = create();
      const auto restored = driver->read_object_bytes(*result);
      read_ok = result->hash == expected && restored && hex(*restored) == hex(bytes);
      auto duplicate = driver->write_object(t81::canonfs::ObjectType::CanonTensor, bytes);
      duplicate_ok = duplicate && duplicate->hash == result->hash;
    }
    std::cout << "{\"written\":" << (result ? "true" : "false")
              << ",\"cold_read\":" << (read_ok ? "true" : "false")
              << ",\"duplicate_ok\":" << (duplicate_ok ? "true" : "false")
              << ",\"error\":" << (result ? 0 : static_cast<int>(result.error())) << ",\"key\":\""
              << t81::canonfs::portable_storage_key(expected) << "\",\"display\":\""
              << expected.h.to_string() << "\"}\n";
    return 0;
  } catch (const std::exception&) {
    return 3;
  }
}
