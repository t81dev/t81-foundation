#include <cassert>
#include <filesystem>
#include <iostream>
#include <vector>
#include "t81/canonfs/axion_hook.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/tracing/canonhash.hpp"

int main() {
  std::cout << "--- CanonFS Axion Trace Persistence Demo ---\n";

  std::filesystem::path root = "demo_canonfs";
  std::filesystem::remove_all(root);

  auto driver = t81::canonfs::make_persistent_driver(root);
  auto hook = t81::canonfs::make_axion_policy_hook("(policy (tier 1))");
  driver->set_axion_hook(hook);

  // 1. Write an object
  std::vector<std::byte> data;
  data.push_back(std::byte{0xDE});
  data.push_back(std::byte{0xAD});
  data.push_back(std::byte{0xBE});
  data.push_back(std::byte{0xEF});

  auto write_res = driver->write_object(t81::canonfs::ObjectType::RawBlock, data);
  if (!write_res.has_value()) {
    std::cerr << "Write failed!\n";
    return 1;
  }
  std::cout << "Wrote object: " << write_res.value().hash.h.to_string() << "\n";

  // 2. Read it back
  auto read_res = driver->read_object_bytes(write_res.value());
  if (!read_res.has_value()) {
    std::cerr << "Read failed!\n";
    return 1;
  }
  assert(read_res.value() == data);
  std::cout << "Read object back successfully.\n";

  // 3. Inspect Axion Trace
  const auto& trace = t81::canonfs::axion_trace();
  std::cout << "\nCaptured Axion Trace for CanonFS operations:\n";
  for (const auto& entry : trace) {
    std::cout << "  [Trace] " << entry << "\n";
  }

  bool found_write = false;
  bool found_read = false;
  for (const auto& entry : trace) {
    if (entry.find("action=Write") != std::string::npos &&
        entry.find("segment=meta") != std::string::npos)
      found_write = true;
    if (entry.find("action=Read") != std::string::npos &&
        entry.find("segment=meta") != std::string::npos)
      found_read = true;
  }

  if (found_write && found_read) {
    std::cout << "\nSUCCESS: CanonFS emitted required Axion trace strings.\n";
  } else {
    std::cerr << "\nFAILURE: Missing required trace strings!\n";
    return 1;
  }

  std::filesystem::remove_all(root);
  return 0;
}
