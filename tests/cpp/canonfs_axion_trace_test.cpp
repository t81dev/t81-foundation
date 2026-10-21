#include <algorithm>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "t81/canonfs/axion_hook.hpp"
#include "t81/canonfs/canon_driver.hpp"

int main() {
  using namespace t81::canonfs;

  reset_axion_trace();

  std::filesystem::path workdir = std::filesystem::temp_directory_path() / "canonfs-axion-trace";
  std::filesystem::remove_all(workdir);
  std::filesystem::create_directories(workdir);
  [[maybe_unused]] auto driver = make_persistent_driver(workdir);
  driver->set_axion_hook(make_axion_policy_hook(R"(
    (policy
      (tier 1)
      (require-axion-event (reason "meta slot axion event segment=meta addr="))
      (require-axion-event (reason "action=Write")))
  )"));

  const std::string payload = "axion-canonfs";
  std::vector<std::byte> bytes(payload.size());
  for (size_t i = 0; i < payload.size(); ++i) {
    bytes[i] = static_cast<std::byte>(payload[i]);
  }

  auto write_res = driver->write_object(ObjectType::RawBlock,
                                        std::span<const std::byte>(bytes.data(), bytes.size()));
  if (!write_res.has_value()) {
    std::cerr << "canonfs_axion_trace_test failure: write_object failed\n";
    std::filesystem::remove_all(workdir);
    return 1;
  }

  // Each operation must satisfy its own policy before touching storage.
  driver->set_axion_hook(make_axion_policy_hook(R"(
    (policy (tier 1)
      (require-axion-event (reason "meta slot axion event segment=meta"))
      (require-axion-event (reason "action=Read"))))"));

  [[maybe_unused]] auto read_res = driver->read_object_bytes(write_res.value());
  if (!read_res.has_value()) {
    std::cerr << "canonfs_axion_trace_test failure: read_object_bytes failed\n";
    std::filesystem::remove_all(workdir);
    return 1;
  }

  const auto& trace = axion_trace();
  if (!std::any_of(trace.begin(), trace.end(), [](auto& entry) {
        return entry.find("meta slot axion event segment=meta addr=") != std::string::npos;
      })) {
    return 1;
  }
  if (!std::any_of(trace.begin(), trace.end(),
                   [](auto& entry) { return entry.find("action=Write") != std::string::npos; })) {
    return 1;
  }
  if (!std::any_of(trace.begin(), trace.end(),
                   [](auto& entry) { return entry.find("action=Read") != std::string::npos; })) {
    return 1;
  }

  std::filesystem::remove_all(workdir);
  return 0;
}
