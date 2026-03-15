// experimental/dpe/tests/thread_pool_test.cpp
//
// RFC-DPE-0006 acceptance tests: Bounded Thread Pool for Epoch Execution.
//
// Acceptance criteria:
//   [DPE-06-01]  A 2-worker pool correctly executes an epoch level with 4
//                independent tasks — all 4 tasks complete, results correct.
//   [DPE-06-02]  An epoch run with a 2-worker pool produces the same
//                EpochHash as the same epoch run with unbounded dispatch.
//   [DPE-06-03]  A 1-worker pool serialises tasks correctly.
//   [DPE-06-04]  DpeThreadPool destructor cleanly joins all workers.

#include "../task_graph.hpp"
#include "../task_runner.hpp"
#include "../epoch_commit.hpp"
#include "../thread_pool.hpp"
#include "t81/isa/program.hpp"
#include "t81/isa/opcodes.hpp"

#include <atomic>
#include <cstdio>

using namespace t81::dpe;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) {
    std::printf("  PASS  %s\n", label);
    ++g_pass;
  } else {
    std::printf("  FAIL  %s\n", label);
    ++g_fail;
  }
}

static std::int64_t first_word(const DeltaRecord& rec) {
  std::int64_t w = 0;
  __builtin_memcpy(&w, rec.value.data(), sizeof(w));
  return w;
}

// ── [DPE-06-01] 4 independent tasks, 2-worker pool ───────────────────────────
//
// Four tasks each write a distinct value to a distinct page.
// Pool has 2 workers — tasks 3 & 4 must wait for workers to free up.
// All 4 committed values must be present after the run.

static void test_pool_more_tasks_than_workers() {
  std::printf("\n[DPE-06-01] 4 tasks submitted to 2-worker pool — all complete\n");

  // Pages at word addresses 512, 768, 1024, 1280.
  static constexpr uint64_t kPages[4] = {512, 768, 1024, 1280};
  static constexpr std::int64_t kVals[4] = {11, 22, 33, 44};

  std::vector<t81::tisc::Program> programs(4);
  for (int i = 0; i < 4; ++i) {
    programs[i].insns = {
      {t81::tisc::Opcode::LoadImm, 1, static_cast<std::int32_t>(kVals[i])},
      {t81::tisc::Opcode::Store,   static_cast<std::int32_t>(kPages[i]), 1},
      {t81::tisc::Opcode::Halt},
    };
  }

  EpochGraph epoch; epoch.epoch_id = 300;
  for (int i = 0; i < 4; ++i) {
    TaskDescriptor t; t.epoch_id = 300; t.task_seq = static_cast<uint64_t>(i);
    t.output_regions.push_back(OutputRegion{kPages[i], 1, true});
    epoch.tasks.push_back(t);
  }

  // Run all 4 tasks via 2-worker pool.
  DpeThreadPool pool(2);
  check(pool.worker_count() == 2, "06-01a: pool created with 2 workers");

  DpeTaskRunner runner;
  std::vector<DpeTaskResult> results(4);

  for (int i = 0; i < 4; ++i) {
    const int idx = i;
    bool submitted = pool.submit([&runner, &epoch, &programs, &results, idx]() {
      results[idx] = runner.run_direct(epoch.tasks[idx], programs[idx]);
    });
    check(submitted, "06-01b: task submitted successfully");
  }
  pool.wait_idle();

  for (int i = 0; i < 4; ++i) {
    check(results[i].halted, "06-01c: task halted");
    check(!results[i].delta_records.empty(), "06-01d: DeltaRecord emitted");
    if (!results[i].delta_records.empty()) {
      check(first_word(results[i].delta_records[0]) == kVals[i],
            "06-01e: correct value committed");
    }
  }
}

// ── [DPE-06-02] EpochHash: pool dispatch == unbounded dispatch ────────────────

static void test_pool_epoch_hash_matches_unbounded() {
  std::printf("\n[DPE-06-02] EpochHash with 2-worker pool == EpochHash with unbounded dispatch\n");

  static constexpr uint64_t kPageA = 512;
  static constexpr uint64_t kPageB = 768;
  static constexpr uint64_t kPageC = 1024;

  auto make_program = [](std::int64_t val, uint64_t page) {
    t81::tisc::Program p;
    p.insns = {
      {t81::tisc::Opcode::LoadImm, 1, static_cast<std::int32_t>(val)},
      {t81::tisc::Opcode::Store,   static_cast<std::int32_t>(page), 1},
      {t81::tisc::Opcode::Halt},
    };
    return p;
  };

  EpochGraph epoch; epoch.epoch_id = 310;
  for (int i = 0; i < 3; ++i) {
    TaskDescriptor t; t.epoch_id = 310; t.task_seq = static_cast<uint64_t>(i);
    t.output_regions.push_back(OutputRegion{(i == 0 ? kPageA : i == 1 ? kPageB : kPageC), 1, true});
    epoch.tasks.push_back(t);
  }

  const std::vector<t81::tisc::Program> programs = {
    make_program(7,  kPageA),
    make_program(13, kPageB),
    make_program(21, kPageC),
  };

  DpeTaskRunner runner;

  // Run via 2-worker pool.
  std::vector<DpeTaskResult> pool_results(3);
  {
    DpeThreadPool pool(2);
    for (int i = 0; i < 3; ++i) {
      const int idx = i;
      pool.submit([&runner, &epoch, &programs, &pool_results, idx]() {
        pool_results[idx] = runner.run_direct(epoch.tasks[idx], programs[idx]);
      });
    }
    pool.wait_idle();
  }

  // Run via direct (unbounded / no pool).
  std::vector<DpeTaskResult> direct_results(3);
  for (int i = 0; i < 3; ++i) {
    direct_results[i] = runner.run_direct(epoch.tasks[i], programs[i]);
  }

  // Build TaskDeltaSets for both runs and commit.
  auto make_sets = [&](const std::vector<DpeTaskResult>& res) {
    std::vector<TaskDeltaSet> sets;
    for (int i = 0; i < 3; ++i) {
      TaskDeltaSet ds;
      ds.id      = compute_task_id(epoch.tasks[i]);
      ds.faulted = !res[i].halted;
      ds.records = res[i].delta_records;
      sets.push_back(std::move(ds));
    }
    return sets;
  };

  const auto commit_pool   = commit_epoch(epoch, make_sets(pool_results));
  const auto commit_direct = commit_epoch(epoch, make_sets(direct_results));

  check(commit_pool.ok() && commit_direct.ok(),
        "06-02a: both commits succeeded");
  if (commit_pool.ok() && commit_direct.ok()) {
    check(commit_pool.epoch_hash.bytes == commit_direct.epoch_hash.bytes,
          "[DPE-06-02] EpochHash identical: pool dispatch == unbounded dispatch");
  }
}

// ── [DPE-06-03] 1-worker pool serialises tasks correctly ─────────────────────

static void test_single_worker_pool() {
  std::printf("\n[DPE-06-03] 1-worker pool serialises 3 independent tasks correctly\n");

  static constexpr uint64_t kPages[3] = {512, 768, 1024};
  static constexpr std::int64_t kVals[3] = {5, 10, 15};

  DpeTaskRunner runner;
  EpochGraph epoch; epoch.epoch_id = 320;
  std::vector<t81::tisc::Program> programs(3);

  for (int i = 0; i < 3; ++i) {
    TaskDescriptor t; t.epoch_id = 320; t.task_seq = static_cast<uint64_t>(i);
    t.output_regions.push_back(OutputRegion{kPages[i], 1, true});
    epoch.tasks.push_back(t);
    programs[i].insns = {
      {t81::tisc::Opcode::LoadImm, 1, static_cast<std::int32_t>(kVals[i])},
      {t81::tisc::Opcode::Store,   static_cast<std::int32_t>(kPages[i]), 1},
      {t81::tisc::Opcode::Halt},
    };
  }

  DpeThreadPool pool(1);
  check(pool.worker_count() == 1, "06-03a: 1-worker pool created");

  std::vector<DpeTaskResult> results(3);
  for (int i = 0; i < 3; ++i) {
    const int idx = i;
    pool.submit([&runner, &epoch, &programs, &results, idx]() {
      results[idx] = runner.run_direct(epoch.tasks[idx], programs[idx]);
    });
  }
  pool.wait_idle();

  for (int i = 0; i < 3; ++i) {
    check(results[i].halted, "06-03b: task halted");
    if (!results[i].delta_records.empty()) {
      check(first_word(results[i].delta_records[0]) == kVals[i],
            "06-03c: correct value committed via 1-worker pool");
    }
  }
}

// ── [DPE-06-04] Destructor cleanly shuts down workers ────────────────────────

static void test_pool_destructor_clean_shutdown() {
  std::printf("\n[DPE-06-04] DpeThreadPool destructor cleanly joins workers\n");

  // Create a pool with 4 workers, submit no tasks, let it go out of scope.
  {
    DpeThreadPool pool(4);
    check(pool.worker_count() == 4, "06-04a: 4-worker pool created");
    // Destructor called here — must not crash or hang.
  }
  check(true, "06-04b: pool destroyed without crash");

  // Create a pool, submit a task, wait for it, then destroy.
  std::atomic<bool> ran{false};
  {
    DpeThreadPool pool(2);
    pool.submit([&ran]() { ran.store(true); });
    pool.wait_idle();
    check(ran.load(), "06-04c: submitted task ran before destructor");
    // Destructor joins workers.
  }
  check(true, "06-04d: pool with completed tasks destroyed without crash");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== DPE Thread Pool tests (RFC-DPE-0006 [DPE-06-01..04]) ===\n");

  test_pool_more_tasks_than_workers();
  test_pool_epoch_hash_matches_unbounded();
  test_single_worker_pool();
  test_pool_destructor_clean_shutdown();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
