#include "t81/canonfs/canon_driver.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <istream>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <string>
#include <utility>
#include <vector>

#include "t81/hash/canonhash.hpp"

namespace t81::canonfs {
namespace {
std::filesystem::path objects_dir(const std::filesystem::path& root) {
  return root / "objects";
}

std::filesystem::path capabilities_dir(const std::filesystem::path& root) {
  return root / "caps";
}

std::filesystem::path parity_dir(const std::filesystem::path& root) {
  return root / "parity";
}

std::filesystem::path object_path(const std::filesystem::path& root,
                                  const CanonHash& hash) {
  return objects_dir(root) / (hash.h.to_string() + ".blk");
}

std::filesystem::path capability_path(const std::filesystem::path& root,
                                      const CanonHash& hash) {
  return capabilities_dir(root) / (hash.h.to_string() + ".cap");
}

std::optional<uint16_t> read_capability(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) return std::nullopt;
  std::ifstream in(path);
  if (!in) return std::nullopt;
  uint16_t value = 0;
  in >> value;
  if (!in) return std::nullopt;
  return value;
}

bool write_capability(const std::filesystem::path& path, uint16_t perms) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) return false;
  out << perms;
  return static_cast<bool>(out);
}

class PersistentDriver final : public Driver {
 public:
  explicit PersistentDriver(std::filesystem::path root)
      : root_(std::move(root)),
        objects_dir_(objects_dir(root_)),
        capabilities_dir_(capabilities_dir(root_)),
        parity_dir_(parity_dir(root_)) {
    std::error_code ec;
    std::filesystem::create_directories(objects_dir_, ec);
    std::filesystem::create_directories(capabilities_dir_, ec);
    std::filesystem::create_directories(parity_dir_, ec);
    if (ec) {
      throw std::runtime_error("CanonFS persistent driver mkdir failed: " +
                               ec.message());
    }
    has_capabilities_ = !std::filesystem::is_empty(capabilities_dir_);
  }

  void set_axion_hook(std::function<AxionVerdict(OpKind, const CanonRef&)> hook)
      override {
    hook_ = std::move(hook);
  }

  Result<CanonRef> write_object(ObjectType, std::span<const std::byte> bytes)
      override {
    std::vector<std::uint8_t> raw;
    raw.reserve(bytes.size());
    for (const auto b : bytes) {
      raw.push_back(std::to_integer<std::uint8_t>(b));
    }
    auto hashed = t81::hash::hash_bytes(raw);
    CanonRef ref{CanonHash{hashed}};
    if (!axion_allow(OpKind::Write, ref)) return Error::CapabilityError;
    if (!has_capability(ref.hash, CANON_PERM_WRITE)) return Error::CapabilityError;
    auto target = object_path(root_, ref.hash);
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    if (!out) return Error::DecodeError;
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (!out) return Error::DecodeError;
    return ref;
  }

  Result<std::vector<std::byte>> read_object_bytes(
      const CanonRef& ref) override {
    if (!axion_allow(OpKind::Read, ref)) return Error::CapabilityError;
    if (!has_capability(ref.hash, CANON_PERM_READ)) return Error::CapabilityError;
    auto target = object_path(root_, ref.hash);
    if (!std::filesystem::exists(target)) return Error::NotFound;
    std::ifstream in(target, std::ios::binary);
    if (!in) return Error::DecodeError;
    in.seekg(0, std::ios::end);
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<std::byte> result;
    if (size > 0) {
      result.resize(static_cast<std::size_t>(size));
      in.read(reinterpret_cast<char*>(result.data()), size);
      if (!in) return Error::DecodeError;
    }
    return result;
  }

  Result<void> publish_capability(const CapabilityGrant& grant) override {
    if (!axion_allow(OpKind::Publish, grant.target)) return Error::CapabilityError;
    auto target = capability_path(root_, grant.target.hash);
    if (!write_capability(target, grant.perms)) return Error::DecodeError;
    has_capabilities_ = true;
    return {};
  }

  Result<void> revoke_capability(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Revoke, ref)) return Error::CapabilityError;
    auto target = capability_path(root_, ref.hash);
    std::error_code ec;
    std::filesystem::remove(target, ec);
    if (ec) return Error::CapabilityError;
    has_capabilities_ = !std::filesystem::is_empty(capabilities_dir_);
    return {};
  }

  Result<void> parity_repair_subtree(const CanonRef& ref) override {
    auto target = object_path(root_, ref.hash);
    if (!std::filesystem::exists(target)) return Error::NotFound;
    return {};
  }

 private:
  bool axion_allow(OpKind kind, const CanonRef& ref) const {
    if (!hook_) return true;
    AxionVerdict v = hook_(kind, ref);
    return v.allow;
  }

  bool has_capability(const CanonHash& hash, uint16_t required) const {
    if (required == 0) return true;
    if (!has_capabilities_) return true;
    auto perms = read_capability(capability_path(root_, hash));
    if (!perms.has_value()) return false;
    return (perms.value() & required) != 0;
  }

  std::filesystem::path root_;
  std::filesystem::path objects_dir_;
  std::filesystem::path capabilities_dir_;
  std::filesystem::path parity_dir_;
  bool has_capabilities_{false};
  std::function<AxionVerdict(OpKind, const CanonRef&)> hook_{};
};

}  // namespace

std::unique_ptr<Driver> make_persistent_driver(std::filesystem::path root) {
  return std::make_unique<PersistentDriver>(std::move(root));
}

}  // namespace t81::canonfs
