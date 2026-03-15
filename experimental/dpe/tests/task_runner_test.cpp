// experimental/dpe/tests/task_runner_test.cpp
//
// RFC-DPE-0002 conformance tests requiring real TISC VM integration.
//
// Acceptance criteria covered:
//   [DPE-02-05]  A single-task epoch with no output regions produces
//                identical register results to direct TISC VM execution.
//
// Additional coverage:
//   [DPE-runner-01]  DpeTaskRunner emits DeltaRecords for pages that change
//                    within declared output regions.
//   [DPE-runner-02]  DpeTaskRunner does NOT emit DeltaRecords for pages that
//                    are declared but not written.

#include "../task_runner.hpp"
#include "t81/vm/vm.hpp"
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

// ── [DPE-02-05] Single-task epoch ≡ direct VM execution ──────────────────────

static void test_single_task_epoch_matches_direct_execution() {
  std::printf("\n[DPE-02-05] Single-task epoch with no output regions matches direct TISC execution\n");

  // Program: LoadImm R5 = 42; Halt
  t81::tisc::Program program;
  program.insns = {
    {t81::tisc::Opcode::LoadImm, 5, 42},
    {t81::tisc::Opcode::Halt},
  };

  // ── Direct VM ─────────────────────────────────────────────────────────────
  auto direct_vm = t81::vm::make_interpreter_vm();
  direct_vm->load_program(program);
  (void)direct_vm->run_to_halt();
  const auto& direct_state = direct_vm->state();

  check(direct_state.halted, "direct: VM halted");
  const std::int64_t direct_r5 =
      direct_state.contexts.empty() ? -1 : direct_state.contexts[0].registers[5];
  check(direct_r5 == 42, "direct: R5 == 42 after LoadImm");

  // ── DpeTaskRunner (no output regions) ────────────────────────────────────
  TaskDescriptor task;
  task.epoch_id = 50;
  task.task_seq = 0;
  // output_regions intentionally empty

  DpeTaskRunner runner;
  const DpeTaskResult dpe = runner.run_direct(task, program);

  check(dpe.halted, "dpe: task halted");
  check(dpe.final_registers[5] == 42, "dpe: R5 == 42 after LoadImm");
  check(dpe.delta_records.empty(),
        "dpe: no delta records for task with no output regions");

  // ── Equivalence ───────────────────────────────────────────────────────────
  bool regs_match = true;
  if (!direct_state.contexts.empty()) {
    for (int i = 0; i < 243; ++i) {
      if (direct_state.contexts[0].registers[i] != dpe.final_registers[i]) {
        regs_match = false;
        break;
      }
    }
  }
  check(regs_match,
        "[DPE-02-05] DpeTaskRunner final registers == direct VM final registers");
}

// ── [DPE-02-05] Variant: two registers, no output regions ────────────────────

static void test_multi_register_program_equivalence() {
  std::printf("\n[DPE-02-05-b] Multi-register program: DpeTaskRunner == direct VM\n");

  // Program: LoadImm R1=10, LoadImm R2=20, Add R3=R1+R2, Halt
  t81::tisc::Program program;
  program.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 10},
    {t81::tisc::Opcode::LoadImm, 2, 20},
    {t81::tisc::Opcode::Add,     3, 1, 2},   // R3 = R1 + R2
    {t81::tisc::Opcode::Halt},
  };

  // Direct VM
  auto direct_vm = t81::vm::make_interpreter_vm();
  direct_vm->load_program(program);
  (void)direct_vm->run_to_halt();
  const auto& ds = direct_vm->state();

  check(ds.halted, "multi-reg direct: VM halted");
  const std::int64_t direct_r3 =
      ds.contexts.empty() ? -1 : ds.contexts[0].registers[3];
  check(direct_r3 == 30, "multi-reg direct: R3 == 30");

  // DpeTaskRunner
  TaskDescriptor task;
  task.epoch_id = 51;
  task.task_seq = 0;

  DpeTaskRunner runner;
  const DpeTaskResult dpe = runner.run_direct(task, program);

  check(dpe.halted, "multi-reg dpe: task halted");
  check(dpe.final_registers[3] == 30, "multi-reg dpe: R3 == 30");

  bool regs_match = true;
  if (!ds.contexts.empty()) {
    for (int i = 0; i < 243; ++i) {
      if (ds.contexts[0].registers[i] != dpe.final_registers[i]) {
        regs_match = false;
        break;
      }
    }
  }
  check(regs_match, "[DPE-02-05-b] multi-register equivalence holds");
}

// ── [DPE-runner-01] DeltaRecords emitted for written output-region pages ──────

static void test_delta_records_emitted_for_written_pages() {
  std::printf("\n[DPE-runner-01] DeltaRecords emitted for pages written within output regions\n");

  // The VM memory layout for an N-instruction program is:
  //   code:   [0, N)
  //   stack:  [N, N + kDefaultStackSize)   kDefaultStackSize = 256
  //   heap:   [N + 256, N + 256 + kDefaultHeapSize)
  //
  // For a 3-instruction program (LoadImm, Store, Halt):
  //   heap.start = 3 + 256 = 259
  //
  // Store only succeeds inside the heap/stack/tensor/meta segments, so we
  // must target a heap word.  We use word 259 as the address.
  static constexpr std::int32_t kHeapWord = 259;

  // Program: LoadImm R1=99, Store mem[259]=R1, Halt
  t81::tisc::Program program;
  program.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 99},
    {t81::tisc::Opcode::Store,   kHeapWord, 1},   // mem[259] = R1
    {t81::tisc::Opcode::Halt},
  };

  // Output region covers the page containing word 259.
  // base_tva = 259, page_count = 1 → words 259..770.
  static constexpr uint64_t kRegionBase = static_cast<uint64_t>(kHeapWord);
  TaskDescriptor task;
  task.epoch_id = 52;
  task.task_seq = 0;
  task.output_regions.push_back(OutputRegion{kRegionBase, 1, false});

  DpeTaskRunner runner;
  const DpeTaskResult dpe = runner.run_direct(task, program);

  check(dpe.halted, "runner-01: task halted");
  check(!dpe.delta_records.empty(),
        "runner-01: at least one DeltaRecord emitted after store to output region");
  if (!dpe.delta_records.empty()) {
    check(dpe.delta_records[0].tva == kRegionBase,
          "runner-01: DeltaRecord TVA == base of declared output region");
  }
}

// ── [DPE-runner-02] No DeltaRecord for declared-but-unwritten pages ───────────

static void test_no_delta_record_for_unwritten_declared_pages() {
  std::printf("\n[DPE-runner-02] No DeltaRecords for declared output region pages not written\n");

  // Program: LoadImm R5=7, Halt  (no Store → no memory write)
  t81::tisc::Program program;
  program.insns = {
    {t81::tisc::Opcode::LoadImm, 5, 7},
    {t81::tisc::Opcode::Halt},
  };

  // Declare an output region in the heap area — but the program never stores to it.
  // heap.start for a 2-instruction program (LoadImm, Halt) = 2 + 256 = 258.
  TaskDescriptor task;
  task.epoch_id = 53;
  task.task_seq = 0;
  task.output_regions.push_back(OutputRegion{258, 2, false});

  DpeTaskRunner runner;
  const DpeTaskResult dpe = runner.run_direct(task, program);

  check(dpe.halted, "runner-02: task halted");
  check(dpe.delta_records.empty(),
        "runner-02: no DeltaRecords when output region pages are not written");
  check(dpe.final_registers[5] == 7, "runner-02: R5 == 7");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== DPE Task Runner tests (RFC-DPE-0002 [DPE-02-05]) ===\n");

  test_single_task_epoch_matches_direct_execution();
  test_multi_register_program_equivalence();
  test_delta_records_emitted_for_written_pages();
  test_no_delta_record_for_unwritten_declared_pages();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
