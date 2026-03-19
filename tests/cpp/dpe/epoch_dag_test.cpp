// tests/cpp/dpe/epoch_dag_test.cpp
//
// RFC-DPE-0004 acceptance tests: DAG-Ordered Multi-Task Epoch Execution.
//
// Acceptance criteria:
//   [DPE-04-01]  T0 writes V to page P; T1's input snapshot contains V at P
//                before T1 runs.
//   [DPE-04-02]  T1 reads V, transforms it to V', and commits V'.
//                The final committed page at TVA P contains V', not V.
//   [DPE-04-03]  The same two-task chain submitted with tasks in reversed
//                array order (T1 first, T0 second) produces identical
//                committed state — proving execution follows the DAG.
//   [DPE-04-04]  An independent task (no deps) is unaffected by the
//                topological ordering change; its committed result equals
//                the pre-DPE-0004 single-task result.

#include "t81/dpe/task_graph.hpp"
#include "t81/dpe/task_runner.hpp"
#include "t81/isa/program.hpp"
#include "t81/isa/opcodes.hpp"

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

// ── Helper: extract the first int64 word from a DeltaRecord ──────────────────

static std::int64_t first_word(const DeltaRecord& rec) {
  std::int64_t w = 0;
  __builtin_memcpy(&w, rec.value.data(), sizeof(w));
  return w;
}

// ── Address layout ────────────────────────────────────────────────────────────
//
// For a 3-instruction program (LoadImm, Store, Halt):
//   code:  [0, 3)
//   stack: [3, 259)
//   heap:  [259, …)
//
// For a 5-instruction program (Load, LoadImm, Add, Store, Halt):
//   code:  [0, 5)
//   stack: [5, 261)
//   heap:  [261, …)
//
// Word 512 (0x200) is safely in the heap region for both programs.

static constexpr uint64_t kPageTVA = 512;

// Build program-identity TaskId helper.
static TaskId prog_id(const TaskDescriptor& t) {
  return program_identity(t);
}

// ── [DPE-04-01 / DPE-04-02] T0 → T1 chain: value flows and transforms ────────

static void test_dag_predecessor_value_flows_to_successor() {
  std::printf("\n[DPE-04-01/02] T0 writes 42 to page P; T1 reads P, adds 1, commits 43\n");

  // T0: LoadImm R1=42; Store mem[512]=R1; Halt
  t81::tisc::Program prog_t0;
  prog_t0.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 42},
    {t81::tisc::Opcode::Store,   static_cast<std::int32_t>(kPageTVA), 1},
    {t81::tisc::Opcode::Halt},
  };

  // T1: Load R2=mem[512]; LoadImm R3=1; Add R4=R2+R3; Store mem[512]=R4; Halt
  t81::tisc::Program prog_t1;
  prog_t1.insns = {
    {t81::tisc::Opcode::Load,    2, static_cast<std::int32_t>(kPageTVA)},
    {t81::tisc::Opcode::LoadImm, 3, 1},
    {t81::tisc::Opcode::Add,     4, 2, 3},
    {t81::tisc::Opcode::Store,   static_cast<std::int32_t>(kPageTVA), 4},
    {t81::tisc::Opcode::Halt},
  };

  // Build T0 descriptor (no deps — independent).
  TaskDescriptor t0;
  t0.epoch_id = 100;
  t0.task_seq = 0;
  t0.output_regions.push_back(OutputRegion{kPageTVA, 1, true});

  // T0's program-identity is its TaskId (deps already empty).
  const TaskId t0_pid = prog_id(t0);

  // Build T1 descriptor with T0 as predecessor.
  TaskDescriptor t1;
  t1.epoch_id = 100;
  t1.task_seq = 1;
  t1.output_regions.push_back(OutputRegion{kPageTVA, 1, true});
  t1.dep_task_ids.push_back(t0_pid);

  // Build epoch with T0 first, T1 second (natural order).
  EpochGraph epoch;
  epoch.epoch_id = 100;
  epoch.tasks = {t0, t1};

  // ── Topological sort should return [0, 1] (T0 before T1) ─────────────────
  const auto order = topological_sort_epoch(epoch);
  check(order.size() == 2,        "[DPE-04-01] topo sort returns 2 indices");
  check(!order.empty() && order[0] == 0, "[DPE-04-01] T0 precedes T1 in topo order");

  // ── Run T0 directly to verify it writes 42 ───────────────────────────────
  DpeTaskRunner runner;
  const DpeTaskResult r0 = runner.run_direct(t0, prog_t0);
  check(r0.halted,                          "[DPE-04-01] T0 halted");
  check(!r0.delta_records.empty(),          "[DPE-04-01] T0 emitted a DeltaRecord");
  if (!r0.delta_records.empty()) {
    check(first_word(r0.delta_records[0]) == 42,
          "[DPE-04-01] T0 DeltaRecord[0] first word == 42");
  }

  // ── Build input snapshot for T1 from T0's delta records ──────────────────
  DpeTaskInputSnapshot snap;
  for (const auto& rec : r0.delta_records) {
    snap.pages.emplace(rec.tva, DpePageSnapshot{rec.value, rec.word_tags});
  }

  // ── Run T1 with the input snapshot ───────────────────────────────────────
  const DpeTaskResult r1 = runner.run_direct(t1, prog_t1, snap);
  check(r1.halted,                          "[DPE-04-02] T1 halted");
  check(!r1.delta_records.empty(),          "[DPE-04-02] T1 emitted a DeltaRecord");
  if (!r1.delta_records.empty()) {
    check(first_word(r1.delta_records[0]) == 43,
          "[DPE-04-02] T1 commits 43 (42 + 1) to page P");
  }
}

// ── [DPE-04-03] Reversed array order produces identical committed state ────────

static void test_dag_array_order_irrelevant() {
  std::printf("\n[DPE-04-03] T1-first, T0-second array order → same topo execution as T0-first\n");

  // Same programs as above.
  t81::tisc::Program prog_t0;
  prog_t0.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 42},
    {t81::tisc::Opcode::Store,   static_cast<std::int32_t>(kPageTVA), 1},
    {t81::tisc::Opcode::Halt},
  };

  t81::tisc::Program prog_t1;
  prog_t1.insns = {
    {t81::tisc::Opcode::Load,    2, static_cast<std::int32_t>(kPageTVA)},
    {t81::tisc::Opcode::LoadImm, 3, 1},
    {t81::tisc::Opcode::Add,     4, 2, 3},
    {t81::tisc::Opcode::Store,   static_cast<std::int32_t>(kPageTVA), 4},
    {t81::tisc::Opcode::Halt},
  };

  // Build T0 (no deps).
  TaskDescriptor t0;
  t0.epoch_id = 101;
  t0.task_seq = 0;
  t0.output_regions.push_back(OutputRegion{kPageTVA, 1, true});
  const TaskId t0_pid = prog_id(t0);

  // Build T1 (dep on T0).
  TaskDescriptor t1;
  t1.epoch_id = 101;
  t1.task_seq = 1;
  t1.output_regions.push_back(OutputRegion{kPageTVA, 1, true});
  t1.dep_task_ids.push_back(t0_pid);

  // Epoch with REVERSED array order: T1 at index 0, T0 at index 1.
  EpochGraph epoch;
  epoch.epoch_id = 101;
  epoch.tasks = {t1, t0};  // reversed!

  // programs[] parallel to tasks[]: programs[0]=prog_t1, programs[1]=prog_t0
  const std::vector<t81::tisc::Program> programs = {prog_t1, prog_t0};

  // ── Topological sort should return [1, 0] (T0 at index 1 runs first) ─────
  const auto order = topological_sort_epoch(epoch);
  check(order.size() == 2,              "[DPE-04-03] topo sort returns 2 indices");
  // T0 is at array index 1; it has no deps so it runs first.
  check(!order.empty() && order[0] == 1, "[DPE-04-03] T0 (array[1]) runs first");
  check(order.size() >= 2 && order[1] == 0, "[DPE-04-03] T1 (array[0]) runs second");

  // ── Manually simulate the DAG execution (same as the kernel does) ─────────
  DpeTaskRunner runner;

  // Run T0 first (array index 1).
  const DpeTaskResult r0 = runner.run_direct(t0, programs[1]);
  check(r0.halted,               "[DPE-04-03] T0 halted");
  check(!r0.delta_records.empty(), "[DPE-04-03] T0 emitted a DeltaRecord");

  // Build snapshot for T1.
  DpeTaskInputSnapshot snap;
  for (const auto& rec : r0.delta_records) {
    snap.pages.emplace(rec.tva, DpePageSnapshot{rec.value, rec.word_tags});
  }

  // Run T1 with snapshot.
  const DpeTaskResult r1 = runner.run_direct(t1, programs[0], snap);
  check(r1.halted,               "[DPE-04-03] T1 halted");
  check(!r1.delta_records.empty(), "[DPE-04-03] T1 emitted a DeltaRecord");
  if (!r1.delta_records.empty()) {
    check(first_word(r1.delta_records[0]) == 43,
          "[DPE-04-03] Reversed-array epoch: committed page P == 43 (same as T0-first)");
  }
}

// ── [DPE-04-04] Independent task unaffected by topological ordering ───────────

static void test_independent_task_unaffected_by_topo_sort() {
  std::printf("\n[DPE-04-04] Independent task (no deps) produces correct result unaffected by topo sort\n");

  // Simple single-task: LoadImm R5=77; Halt.  No output regions, no deps.
  t81::tisc::Program program;
  program.insns = {
    {t81::tisc::Opcode::LoadImm, 5, 77},
    {t81::tisc::Opcode::Halt},
  };

  TaskDescriptor task;
  task.epoch_id = 102;
  task.task_seq = 0;
  // No output_regions, no dep_task_ids.

  EpochGraph epoch;
  epoch.epoch_id = 102;
  epoch.tasks    = {task};

  const auto order = topological_sort_epoch(epoch);
  check(order.size() == 1 && order[0] == 0,
        "[DPE-04-04] Single independent task: topo sort returns [0]");

  DpeTaskRunner runner;
  const DpeTaskResult res = runner.run_direct(task, program);
  check(res.halted,                      "[DPE-04-04] Independent task halted");
  check(res.final_registers[5] == 77,    "[DPE-04-04] R5 == 77 (unaffected)");
  check(res.delta_records.empty(),       "[DPE-04-04] No DeltaRecords (no output regions)");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== DPE Epoch DAG tests (RFC-DPE-0004 [DPE-04-01..04]) ===\n");

  test_dag_predecessor_value_flows_to_successor();
  test_dag_array_order_irrelevant();
  test_independent_task_unaffected_by_topo_sort();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
