#pragma once

#include <memory>
#include <vector>
#include <t81/support/expected.hpp>
#include "t81/canonfs/canon_driver.hpp"
#include "t81/hanoi/types.hpp"
#include "t81/hanoi/error.hpp"

namespace t81::hanoi {
template <typename T>
using Result = std::expected<T, Error>;

class Kernel {
 public:
  virtual ~Kernel() = default;
  virtual Result<SnapshotRef> fork_snapshot(const SnapshotRef& base) = 0;
  virtual Result<SnapshotRef> commit_snapshot(const SnapshotRef& snapshot) = 0;
  virtual Result<void> switch_root(const SnapshotRef& snapshot) = 0;
  virtual Result<Pid> spawn(const SnapshotRef& snapshot) = 0;
  virtual Result<std::vector<std::byte>> read_object(const t81::canonfs::CanonRef& ref) = 0;
  virtual Result<void> grant_cap(const t81::canonfs::CapabilityGrant& grant) = 0;
  virtual Result<void> revoke_cap(const t81::canonfs::CanonRef& ref) = 0;
  virtual Result<void> yield_tick() = 0;
  virtual Result<RegionHandle> map_region(std::size_t bytes) = 0;
  virtual Result<void> parity_repair(const t81::canonfs::CanonRef& ref) = 0;
  virtual Result<void> halt() = 0;
};

// Factory for the in-memory kernel simulator.
std::unique_ptr<Kernel> make_in_memory_kernel(t81::canonfs::Driver& driver);
}  // namespace t81::hanoi

