#include "t81/canonfs/canon_driver.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace t81::canonfs {
namespace {
class InMemoryDriver : public Driver {
 public:
  Result<CanonRef> write_object(ObjectType, std::span<const std::byte> bytes) override {
    CanonRef ref{CanonHash{next_hash()}};
    objects_[ref.hash] = std::vector<std::byte>(bytes.begin(), bytes.end());
    return ref;
  }

  Result<std::vector<std::byte>> read_object_bytes(const CanonRef& ref) override {
    auto it = objects_.find(ref.hash);
    if (it == objects_.end()) return Error::NotFound;
    return it->second;
  }

  Result<void> publish_capability(const CapabilityGrant& grant) override {
    capabilities_[grant.target.hash] = grant.subject;
    return {};
  }

  Result<void> revoke_capability(const CanonRef& ref) override {
    capabilities_.erase(ref.hash);
    return {};
  }

  Result<void> parity_repair_subtree(const CanonRef& ref) override {
    // Placeholder: mark parity repair succeeded. TODO: spec/canonfs-spec.md repair rules.
    if (!objects_.count(ref.hash)) return Error::NotFound;
    return {};
  }

 private:
  std::string next_hash() {
    // TODO: implement canonical hash (spec/canonfs-spec.md §hash). Placeholder counter-based.
    return "mem-" + std::to_string(counter_++);
  }

  std::map<CanonHash, std::vector<std::byte>> objects_;
  std::map<CanonHash, std::string> capabilities_;
  std::size_t counter_{0};
};
}  // namespace

std::unique_ptr<Driver> make_in_memory_driver() { return std::make_unique<InMemoryDriver>(); }
}  // namespace t81::canonfs

