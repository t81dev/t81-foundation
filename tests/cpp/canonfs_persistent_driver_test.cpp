#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "t81/canonfs/axion_hook.hpp"
#include "t81/canonfs/canon_driver.hpp"

std::vector<std::byte> make_bytes(std::string_view str) {
  std::vector<std::byte> bytes;
  bytes.reserve(str.size());
  for (const char c : str) {
    bytes.push_back(static_cast<std::byte>(c));
  }
  return bytes;
}

int main() {
  using namespace t81::canonfs;

  reset_axion_trace();

  std::filesystem::path root =
      std::filesystem::temp_directory_path() / "canonfs-persistent-test";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  auto driver = make_persistent_driver(root);
  driver->set_axion_hook(make_axion_policy_hook(R"(
    (policy
      (tier 1)
      (require-axion-event (reason "meta slot axion event segment=meta"))
      (require-axion-event (reason "action=Write"))
      (require-axion-event (reason "action=Read")))
  )"));

  const std::string payload = "persistent payload";
  auto write_bytes = make_bytes(payload);
  auto write_res = driver->write_object(
      ObjectType::Blob,
      std::span<const std::byte>(write_bytes.data(), write_bytes.size()));
  if (!write_res.has_value()) return 1;

  auto read_res = driver->read_object_bytes(write_res.value());
  if (!read_res.has_value()) return 1;

  driver.reset();

  auto driver2 = make_persistent_driver(root);
  driver2->set_axion_hook(make_axion_policy_hook(R"(
    (policy
      (tier 1)
      (require-axion-event (reason "meta slot axion event segment=meta"))
      (require-axion-event (reason "action=Read")))
  )"));

  auto read_again = driver2->read_object_bytes(write_res.value());
  if (!read_again.has_value()) return 1;

  if (read_again.value() != read_res.value()) return 1;

  std::filesystem::remove_all(root);
  return 0;
}
