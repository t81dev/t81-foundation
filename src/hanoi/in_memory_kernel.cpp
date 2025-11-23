#include "t81/hanoi/kernel.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "t81/axion/engine.hpp"

namespace t81::hanoi {
namespace {
struct Snapshot {
  t81::canonfs::CanonRef root;
};

class InMemoryKernel : public Kernel {
 public:
  explicit InMemoryKernel(t81::canonfs::Driver& driver) : driver_(driver) {
    current_root_ = t81::canonfs::CanonRef{t81::canonfs::CanonHash{"root"}};
    snapshots_[current_root_.hash] = Snapshot{current_root_};
  }

  Result<SnapshotRef> fork_snapshot(const SnapshotRef& base) override {
    if (!snapshots_.count(base.hash)) return Error::CanonMismatch;
    SnapshotRef child{t81::canonfs::CanonHash{base.hash + "-fork"}};
    snapshots_[child.hash] = Snapshot{child};
    return child;
  }

  Result<SnapshotRef> commit_snapshot(const SnapshotRef& snapshot) override {
    if (!snapshots_.count(snapshot.hash)) return Error::CanonMismatch;
    current_root_ = snapshot;
    return snapshot;
  }

  Result<void> switch_root(const SnapshotRef& snapshot) override {
    if (!snapshots_.count(snapshot.hash)) return Error::CanonMismatch;
    current_root_ = snapshot;
    return {};
  }

  Result<Pid> spawn(const SnapshotRef& snapshot) override {
    if (!snapshots_.count(snapshot.hash)) return Error::CanonMismatch;
    return static_cast<Pid>(++next_pid_);
  }

  Result<std::vector<std::byte>> read_object(const t81::canonfs::CanonRef& ref) override {
    auto bytes = driver_.read_object_bytes(ref);
    if (!bytes) return Error::CapabilityMissing;
    return bytes.value();
  }

  Result<void> grant_cap(const t81::canonfs::CapabilityGrant& grant) override {
    auto res = driver_.publish_capability(grant);
    if (!res) return Error::CapabilityRevoked;
    return {};
  }

  Result<void> revoke_cap(const t81::canonfs::CanonRef& ref) override {
    auto res = driver_.revoke_capability(ref);
    if (!res) return Error::CapabilityRevoked;
    return {};
  }

  Result<void> yield_tick() override { return {}; }

  Result<RegionHandle> map_region(std::size_t bytes) override {
    RegionHandle handle{next_region_++};
    regions_[handle.id] = std::vector<std::byte>(bytes);
    return handle;
  }

  Result<void> parity_repair(const t81::canonfs::CanonRef& ref) override {
    auto res = driver_.parity_repair_subtree(ref);
    if (!res) return Error::RepairError;
    return {};
  }

  Result<void> halt() override {
    halted_ = true;
    return {};
  }

 private:
  t81::canonfs::Driver& driver_;
  std::map<t81::canonfs::CanonHash, Snapshot> snapshots_;
  SnapshotRef current_root_;
  std::map<std::uint64_t, std::vector<std::byte>> regions_;
  std::uint64_t next_region_{1};
  std::uint64_t next_pid_{0};
  bool halted_{false};
};
}  // namespace

std::unique_ptr<Kernel> make_in_memory_kernel(t81::canonfs::Driver& driver) {
  return std::make_unique<InMemoryKernel>(driver);
}
}  // namespace t81::hanoi

