#include "t81/canonfs/canon_driver.hpp"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <t81/canonfs/rs_repair.hpp>
#include <t81/tracing/canonhash.hpp>
#include <vector>

namespace t81::canonfs {
namespace {
bool read_verify_enabled() {
  const char* raw = std::getenv("T81_CANONFS_READ_VERIFY");
  if (raw == nullptr) {
    return true;
  }
  std::string_view v(raw);
  return !(v == "0" || v == "false" || v == "FALSE" || v == "off" || v == "OFF");
}

class InMemoryDriver : public Driver {
public:
  void set_axion_hook(std::function<AxionVerdict(OpKind, const CanonRef&)> hook) override {
    hook_ = std::move(hook);
  }
  Result<CanonRef> write_object(ObjectType, std::span<const std::byte> bytes) override {
    if (!axion_allow(OpKind::Write, std::nullopt)) {
      return Result<CanonRef>(t81::unexpect, Error::CapabilityError);
    }
    // Content-address using CanonHash81 over raw bytes.
    auto h = t81::hash::hash_bytes(bytes);
    CanonHash canon_hash{h};
    CanonRef ref{canon_hash};
    if (!has_capability(ref, CANON_PERM_WRITE)) {
      return Result<CanonRef>(t81::unexpect, Error::CapabilityError);
    }
    auto& stored = objects_[ref.hash];
    stored.assign(bytes.begin(), bytes.end());
    parity_shards_[ref.hash] = ReedSolomonRepair::encode(stored);
    return ref;
  }

  Result<std::vector<std::byte>> read_object_bytes(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Read, ref)) {
      return Result<std::vector<std::byte>>(t81::unexpect, Error::CapabilityError);
    }
    if (!has_capability(ref, CANON_PERM_READ)) {
      return Result<std::vector<std::byte>>(t81::unexpect, Error::CapabilityError);
    }
    auto it = objects_.find(ref.hash);
    if (it == objects_.end()) {
      return Result<std::vector<std::byte>>(t81::unexpect, Error::NotFound);
    }
    if (read_verify_enabled()) {
      auto computed =
          t81::hash::hash_bytes(std::span<const std::byte>(it->second.data(), it->second.size()));
      if (computed != ref.hash.h) {
        return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
      }
    }
    return it->second;
  }

  Result<void> publish_capability(const CapabilityGrant& grant) override {
    if (!axion_allow(OpKind::Publish, grant.target)) {
      return Result<void>(t81::unexpect, Error::CapabilityError);
    }
    capabilities_[grant.target.hash] = grant.perms;
    return {};
  }

  Result<void> revoke_capability(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Revoke, ref)) {
      return Result<void>(t81::unexpect, Error::CapabilityError);
    }
    capabilities_.erase(ref.hash);
    return {};
  }

  Result<void> parity_repair_subtree(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Repair, ref)) {
      return Result<void>(t81::unexpect, Error::CapabilityError);
    }
    if (objects_.count(ref.hash)) return {};

    // Attempt repair from parity shards
    auto it = parity_shards_.find(ref.hash);
    if (it == parity_shards_.end()) {
      return Result<void>(t81::unexpect, Error::NotFound);
    }

    const auto& shards = it->second;
    std::vector<bool> available(5, true);  // Assume all 5 are there for this impl
    auto recovered = ReedSolomonRepair::repair(shards, available);

    if (recovered.empty()) {
      return Result<void>(t81::unexpect, Error::ParityFailure);
    }

    // Concatenate recovered data shards
    std::vector<std::byte> full_data;
    for (const auto& shard : recovered) {
      full_data.insert(full_data.end(), shard.begin(), shard.end());
    }

    objects_[ref.hash] = std::move(full_data);
    return {};
  }

private:
  bool axion_allow(OpKind kind, std::optional<CanonRef> ref) const {
    if (!hook_) return true;
    AxionVerdict v = hook_(kind, ref.value_or(CanonRef{CanonHash{}}));
    return v.allow;
  }

  bool has_capability(const CanonRef& ref, uint16_t required) const {
    if (required == 0) return true;
    if (capabilities_.empty()) return true;  // bootstrap: allow when no caps exist
    auto it = capabilities_.find(ref.hash);
    if (it == capabilities_.end()) return false;
    return (it->second & required) != 0;
  }

  std::map<CanonHash, std::vector<std::byte>> objects_;
  std::map<CanonHash, std::vector<std::vector<std::byte>>> parity_shards_;
  std::map<CanonHash, uint16_t> capabilities_;
  std::function<AxionVerdict(OpKind, const CanonRef&)> hook_{};
};
}  // namespace

std::unique_ptr<Driver> make_in_memory_driver() { return std::make_unique<InMemoryDriver>(); }
}  // namespace t81::canonfs

#ifdef _WIN32
#pragma warning(pop)
#endif
