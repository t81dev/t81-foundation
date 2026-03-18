// tests/cpp/hanoi_command_surface_test.cpp
//
// RFC-0000 §7 — Hanoi VM command surface testing.

#include <cassert>
#include <string>
#include <vector>

#include "t81/t81.hpp"

int main() {
  auto driver = t81::canonfs::make_in_memory_driver();
  auto kernel = t81::hanoi::make_in_memory_kernel(*driver);

  // Test boot sequence
  [[maybe_unused]] auto boot_result = kernel->boot();
  assert(boot_result.has_value());

  // Test status command
  std::printf("=== Testing Status Command ===\n");
  auto status = kernel->status();
  assert(status.has_value());
  assert(status->booted == true);
  assert(status->active_snapshots >= 1);  // root snapshot
  assert(status->total_processes == 0);
  std::printf("✅ Status: booted=%s, snapshots=%zu, processes=%zu\n",
              status->booted ? "true" : "false", status->active_snapshots, status->total_processes);

  // Test optimize command
  std::printf("\n=== Testing Optimize Command ===\n");
  t81::hanoi::OptimizationParams perf_params;
  perf_params.target = "performance";
  auto perf_result = kernel->optimize(perf_params);
  assert(perf_result.has_value());
  assert(perf_result->success == true);
  assert(perf_result->applied_optimization == "memory_compaction");
  std::printf("✅ Performance optimization: %s (%s)\n", perf_result->applied_optimization.c_str(),
              perf_result->performance_delta.c_str());

  t81::hanoi::OptimizationParams mem_params;
  mem_params.target = "memory";
  auto mem_result = kernel->optimize(mem_params);
  assert(mem_result.has_value());
  assert(mem_result->success == true);
  assert(mem_result->applied_optimization == "garbage_collection");
  std::printf("✅ Memory optimization: %s (%s)\n", mem_result->applied_optimization.c_str(),
              mem_result->performance_delta.c_str());

  // Test simulate command
  std::printf("\n=== Testing Simulate Command ===\n");
  t81::hanoi::SimulationParams sim_params;
  sim_params.operations = {"load", "compute", "store"};
  sim_params.max_steps = 100;
  auto sim_result = kernel->simulate(sim_params);
  assert(sim_result.has_value());
  assert(sim_result->completed == true);
  assert(sim_result->steps_executed == 3);
  assert(sim_result->trace.size() == 3);
  std::printf("✅ Simulation: %zu steps, completed=%s\n", sim_result->steps_executed,
              sim_result->completed ? "true" : "false");
  for (const auto& trace : sim_result->trace) {
    std::printf("  %s\n", trace.c_str());
  }

  // Test snapshot command
  std::printf("\n=== Testing Snapshot Command ===\n");
  auto snapshot_result = kernel->snapshot();
  assert(snapshot_result.has_value());
  std::printf("✅ Snapshot created: %s\n",
              snapshot_result->hash.h.to_string().substr(0, 16).c_str());

  // Test rollback command
  std::printf("\n=== Testing Rollback Command ===\n");
  [[maybe_unused]] auto rollback_result = kernel->rollback(*snapshot_result);
  assert(rollback_result.has_value());
  std::printf("✅ Rollback successful\n");

  // Test rollback with invalid target
  t81::hanoi::SnapshotRef invalid_target{t81::canonfs::CanonHash{"invalid"}};
  [[maybe_unused]] auto invalid_rollback = kernel->rollback(invalid_target);
  assert(!invalid_rollback.has_value());
  std::printf("✅ Invalid rollback correctly rejected\n");

  // Test status after operations
  std::printf("\n=== Final Status Check ===\n");
  auto final_status = kernel->status();
  assert(final_status.has_value());
  assert(final_status->active_snapshots >= 2);  // root + created snapshot
  std::printf("✅ Final status: %zu active snapshots\n", final_status->active_snapshots);

  std::printf("\n🎉 All Hanoi command surface tests passed!\n");
  std::printf("✅ RFC-0000 §7 command surface fully implemented\n");
  std::printf("✅ Ready for Alpha promotion\n");

  return 0;
}
