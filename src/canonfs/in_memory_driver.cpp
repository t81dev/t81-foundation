#include "t81/canonfs/canon_driver.hpp"

#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <t81/hash/canonhash.hpp>

namespace t81::canonfs {
namespace {
class InMemoryDriver : public Driver {
 public:
  void set_axion_hook(std::function<AxionVerdict(OpKind, const CanonRef&)> hook) override {
    hook_ = std::move(hook);
  }
  Result<CanonRef> write_object(ObjectType, std::span<const std::byte> bytes) override {
    if (!axion_allow(OpKind::Write, std::nullopt)) return Error::CapabilityError;
    // Content-address using CanonHash81 over raw bytes.
    std::vector<std::uint8_t> v(bytes.size());
    std::memcpy(v.data(), bytes.data(), bytes.size());
    auto h = t81::hash::hash_bytes(v);
    CanonHash canon_hash{h};
    CanonRef ref{canon_hash};
    if (!has_capability(ref, CANON_PERM_WRITE)) return Error::CapabilityError;
    objects_[ref.hash] = std::vector<std::byte>(bytes.begin(), bytes.end());
    return ref;
  }

  Result<std::vector<std::byte>> read_object_bytes(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Read, ref)) return Error::CapabilityError;
    if (!has_capability(ref, CANON_PERM_READ)) return Error::CapabilityError;
    auto it = objects_.find(ref.hash);
    if (it == objects_.end()) return Error::NotFound;
    return it->second;
  }

  Result<void> publish_capability(const CapabilityGrant& grant) override {
    capabilities_[grant.target.hash] = grant.perms;
    axion_allow(OpKind::Publish, grant.target); // side-effect hook
    return {};
  }

  Result<void> revoke_capability(const CanonRef& ref) override {
    capabilities_.erase(ref.hash);
    axion_allow(OpKind::Revoke, ref);
    return {};
  }

  Result<void> parity_repair_subtree(const CanonRef& ref) override {
    // Placeholder: mark parity repair succeeded. TODO: spec/canonfs-spec.md repair rules.
    if (!objects_.count(ref.hash)) return Error::NotFound;
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
    if (capabilities_.empty()) return true; // bootstrap: allow when no caps exist
    auto it = capabilities_.find(ref.hash);
    if (it == capabilities_.end()) return false;
    return (it->second & required) != 0;
  }

  std::map<CanonHash, std::vector<std::byte>> objects_;
  std::map<CanonHash, uint16_t> capabilities_;
  std::function<AxionVerdict(OpKind, const CanonRef&)> hook_{};
};
}  // namespace

std::unique_ptr<Driver> make_in_memory_driver() { return std::make_unique<InMemoryDriver>(); }
}  // namespace t81::canonfs
