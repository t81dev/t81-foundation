#include "t81/experimental/hanoi/kernel.hpp"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "t81/axion/ethics.hpp"

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

  // RFC-0000 §4: Ethics-first boot — evaluates Θ₁–Θ₉ against the boot context.
  // Failures trigger AXHALT semantics: returns AxionRejection and blocks spawn().
  Result<void> boot() override {
    t81::axion::SyscallContext ctx;
    ctx.snapshot = current_root_;
    ctx.caller   = "hanoi::boot";
    ctx.syscall  = "boot";
    for (int i = 1; i <= t81::axion::kEthicsPrincipleCount; ++i) {
      auto principle = static_cast<t81::axion::EthicsPrinciple>(i);
      auto verdict   = t81::axion::check_ethics(principle, ctx);
      if (verdict.kind == t81::axion::VerdictKind::Deny) {
        boot_failed_reason_ = verdict.reason;
        return Result<void>(t81::unexpect, Error::AxionRejection);
      }
    }
    booted_ = true;
    return {};
  }

  Result<SnapshotRef> fork_snapshot(const SnapshotRef& base) override {
    if (!snapshots_.count(base.hash)) {
      return Result<SnapshotRef>(t81::unexpect, Error::CanonMismatch);
    }
    // Derive a new hash deterministically by hashing the base hash string plus a fork marker.
    std::string fork_str = base.hash.h.to_string() + ".fork." + std::to_string(next_pid_);
    SnapshotRef child{t81::canonfs::CanonHash{t81::hash::hash_string(fork_str)}};
    snapshots_[child.hash] = Snapshot{child};
    return child;
  }

  Result<SnapshotRef> commit_snapshot(const SnapshotRef& snapshot) override {
    if (!snapshots_.count(snapshot.hash)) {
      return Result<SnapshotRef>(t81::unexpect, Error::CanonMismatch);
    }
    current_root_ = snapshot;
    return snapshot;
  }

  Result<void> switch_root(const SnapshotRef& snapshot) override {
    if (!snapshots_.count(snapshot.hash)) {
      return Result<void>(t81::unexpect, Error::CanonMismatch);
    }
    current_root_ = snapshot;
    return {};
  }

  Result<Pid> spawn(const SnapshotRef& snapshot) override {
    // RFC-0000 §4: spawn() requires a successful ethics-first boot().
    if (!booted_) {
      return Result<Pid>(t81::unexpect, Error::AxionRejection);
    }
    if (!snapshots_.count(snapshot.hash)) {
      return Result<Pid>(t81::unexpect, Error::CanonMismatch);
    }
    // RFC-0000 §4: Hanoi scheduler MUST limit active PIDs to 81 (3^4).
    if (active_pids_.size() >= kMaxSlots) {
      return Result<Pid>(t81::unexpect, Error::SchedulerFull);
    }
    Pid pid = static_cast<Pid>(++next_pid_);
    active_pids_.insert(pid);
    return pid;
  }

  Result<std::vector<std::byte>> read_object(const t81::canonfs::CanonRef& ref) override {
    auto bytes = driver_.read_object_bytes(ref);
    if (!bytes) return Result<std::vector<std::byte>>(t81::unexpect, Error::CapabilityMissing);
    return bytes.value();
  }

  Result<void> grant_cap(const t81::canonfs::CapabilityGrant& grant) override {
    auto res = driver_.publish_capability(grant);
    if (!res) return Result<void>(t81::unexpect, Error::CapabilityRevoked);
    return {};
  }

  Result<void> revoke_cap(const t81::canonfs::CanonRef& ref) override {
    auto res = driver_.revoke_capability(ref);
    if (!res) return Result<void>(t81::unexpect, Error::CapabilityRevoked);
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
    if (!res) return Result<void>(t81::unexpect, Error::RepairError);
    return {};
  }

  Result<void> halt() override {
    halted_ = true;
    return {};
  }
  
  // ─── Command Surface Implementation (RFC-0000 §7) ─────────────────────
  
  Result<KernelStatus> status() override {
    KernelStatus status;
    status.booted = booted_;
    status.active_snapshots = snapshots_.size();
    status.total_processes = active_pids_.size();
    status.current_root_hash = current_root_.hash.h.to_string();
    status.last_axion_verdict = boot_failed_reason_.empty() ? "Allow" : boot_failed_reason_;
    return status;
  }
  
  Result<OptimizationResult> optimize(const OptimizationParams& params) override {
    OptimizationResult result;
    
    // Simple optimization simulation
    if (params.target == "performance") {
      result.success = true;
      result.applied_optimization = "memory_compaction";
      result.performance_delta = "+5.2%";
    } else if (params.target == "memory") {
      result.success = true;
      result.applied_optimization = "garbage_collection";
      result.performance_delta = "-12.8% memory";
    } else {
      result.success = false;
      result.applied_optimization = "none";
      result.performance_delta = "0%";
    }
    
    return result;
  }
  
  Result<SimulationResult> simulate(const SimulationParams& params) override {
    SimulationResult result;
    
    // Simple simulation of operations
    for (const auto& op : params.operations) {
      if (result.steps_executed >= params.max_steps) {
        break;
      }
      
      result.trace.push_back("exec: " + op);
      result.steps_executed++;
    }
    
    result.completed = result.steps_executed == params.operations.size();
    
    // Compute final state hash (simplified)
    std::string state_str = "steps:" + std::to_string(result.steps_executed);
    result.final_state_hash = t81::hash::hash_string(state_str).to_string();
    
    return result;
  }
  
  Result<SnapshotRef> snapshot() override {
    // Create a new snapshot from current state
    std::string snapshot_str = current_root_.hash.h.to_string() + ".snapshot." + std::to_string(next_pid_);
    SnapshotRef new_snapshot{t81::canonfs::CanonHash{t81::hash::hash_string(snapshot_str)}};
    snapshots_[new_snapshot.hash] = Snapshot{new_snapshot};
    return new_snapshot;
  }
  
  Result<void> rollback(const SnapshotRef& target) override {
    if (!snapshots_.count(target.hash)) {
      return Result<void>(t81::unexpect, Error::CanonMismatch);
    }
    
    current_root_ = target;
    return {};
  }

private:
  // RFC-0000 §4: 81-slot deterministic scheduler (3^4 = 81).
  static constexpr std::size_t kMaxSlots = 81;

  t81::canonfs::Driver& driver_;
  std::map<t81::canonfs::CanonHash, Snapshot> snapshots_;
  SnapshotRef current_root_;
  std::map<std::uint64_t, std::vector<std::byte>> regions_;
  std::set<Pid> active_pids_;
  std::uint64_t next_region_{1};
  std::uint64_t next_pid_{0};
  bool halted_{false};
  bool booted_{false};
  std::string boot_failed_reason_;
};
}  // namespace

std::unique_ptr<Kernel> make_in_memory_kernel(t81::canonfs::Driver& driver) {
  return std::make_unique<InMemoryKernel>(driver);
}
}  // namespace t81::hanoi
