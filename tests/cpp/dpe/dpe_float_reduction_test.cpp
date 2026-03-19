// tests/cpp/dpe/dpe_float_reduction_test.cpp
//
// RFC-DPE-0003 [DPE-03-05]: T81Float operations inside tasks use the strict
// deterministic path; hash divergence from non-canonical float is detectable.
//
// Acceptance criteria covered:
//   [DPE-03-05-a]  A DeltaRecord word tagged FloatHandle contains the IEEE 754
//                  double representation of the float value — not a transient
//                  pool handle index.
//   [DPE-03-05-b]  Two independent executions of the same epoch produce
//                  identical EpochHash (float determinism).
//   [DPE-03-05-c]  A float value round-trips through DeltaRecord → snapshot →
//                  successor task execution; the successor reads back the same
//                  double value that the predecessor stored.
//   [DPE-03-05-d]  Two tasks that load the same float literal from pools
//                  allocated in different orders emit identical DeltaRecord
//                  bytes for that float word (content-addressable, no handle
//                  index leakage).

#include "t81/dpe/epoch_commit.hpp"
#include "t81/dpe/task_graph.hpp"
#include "t81/dpe/task_runner.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

#include <cstdio>
#include <cstring>

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

// ── Address layout ────────────────────────────────────────────────────────────
//
// For a 3-instruction program (LoadImm, Store, Halt):
//   code:  [0, 3)
//   stack: [3, 259)
//   heap:  [259, …)
//
// Word 512 (0x200) is safely in the heap region.
// Word 1024 (0x400) is used as the successor output page in the round-trip
// test so T1 writes to a fresh address, guaranteeing a detectable change.

static constexpr uint64_t kFloatPageTVA  = 512;
static constexpr uint64_t kFloat2PageTVA = 1024;

// Extract the double stored at word 0 of a DeltaRecord.
static double first_float(const DeltaRecord& rec) {
  double v = 0.0;
  std::memcpy(&v, rec.value.data(), sizeof(v));
  return v;
}

// Check whether word 0 of a DeltaRecord is tagged FloatHandle (tag value 3,
// matching ValueTag::FloatHandle in t81/vm/state.hpp).
static bool first_word_is_float(const DeltaRecord& rec) {
  // ValueTag::FloatHandle == 3 per the enum in state.hpp
  return rec.word_tags[0] == 3;
}

// ── Build a minimal float-producing TISC program ──────────────────────────────
//
//   float_pool = {value}
//   LoadImm R1, 0   (FloatHandle) — loads float_pool[0]
//   Store   mem[kFloatPageTVA], R1
//   Halt

static t81::tisc::Program make_float_store_program(double value) {
  t81::tisc::Program prog;
  prog.float_pool = {value};
  // state_.floats = program_.float_pool, and float handles are 1-based
  // (float_ptr(h) returns &state_.floats[h-1]).  Pool[0] → handle 1.
  prog.insns = {
    {t81::tisc::Opcode::LoadImm,
     /*a=*/1, /*b=*/1, /*c=*/0,
     t81::tisc::LiteralKind::FloatHandle},
    {t81::tisc::Opcode::Store,
     static_cast<std::int32_t>(kFloatPageTVA), /*b=*/1},
    {t81::tisc::Opcode::Halt},
  };
  return prog;
}

// Build a successor program that reads a float from one page and writes it to
// a different page, proving the value propagated correctly through the snapshot.
//
//   Load  R1, mem[kFloatPageTVA]
//   Store mem[kFloat2PageTVA], R1   ← writes to a fresh page
//   Halt

static t81::tisc::Program make_float_passthrough_program() {
  t81::tisc::Program prog;
  prog.insns = {
    {t81::tisc::Opcode::Load,
     /*a=*/1, /*b=*/static_cast<std::int64_t>(kFloatPageTVA)},
    {t81::tisc::Opcode::Store,
     static_cast<std::int32_t>(kFloat2PageTVA), /*b=*/1},
    {t81::tisc::Opcode::Halt},
  };
  return prog;
}

// ── [DPE-03-05-a] FloatHandle delta word contains canonical double bytes ──────

static void test_float_delta_contains_canonical_bytes() {
  std::printf("\n[DPE-03-05-a] DeltaRecord word is FloatHandle-tagged with canonical double\n");

  const double kValue = 3.14159265358979;
  auto prog = make_float_store_program(kValue);

  TaskDescriptor t0;
  t0.epoch_id = 305;
  t0.task_seq = 0;
  t0.output_regions.push_back(OutputRegion{kFloatPageTVA, 1, true});

  DpeTaskRunner runner;
  const DpeTaskResult result = runner.run_direct(t0, prog);

  check(result.halted,                     "[DPE-03-05-a] task halted");
  check(!result.delta_records.empty(),     "[DPE-03-05-a] delta record emitted");

  if (!result.delta_records.empty()) {
    const DeltaRecord& rec = result.delta_records[0];

    check(first_word_is_float(rec),
          "[DPE-03-05-a] word_tags[0] == FloatHandle (3)");

    const double stored = first_float(rec);
    check(stored == kValue,
          "[DPE-03-05-a] delta bytes decode to original double value");

    // Verify it is NOT equal to the handle index (which would be 1 for the
    // first float in a fresh pool — obviously != 3.14).
    double handle_as_double = 0.0;
    const std::int64_t handle_idx = 1;
    std::memcpy(&handle_as_double, &handle_idx, sizeof(handle_idx));
    check(stored != handle_as_double,
          "[DPE-03-05-a] delta bytes are NOT the raw handle index");
  }
}

// ── [DPE-03-05-b] Identical epochs produce identical EpochHash ────────────────

static void test_float_epoch_hash_determinism() {
  std::printf("\n[DPE-03-05-b] Two runs of the same float epoch yield identical EpochHash\n");

  const double kValue = 2.718281828459045;
  auto prog = make_float_store_program(kValue);

  TaskDescriptor t0;
  t0.epoch_id = 305;
  t0.task_seq = 0;
  t0.output_regions.push_back(OutputRegion{kFloatPageTVA, 1, true});

  EpochGraph epoch;
  epoch.epoch_id = 305;
  epoch.tasks    = {t0};

  // Run #1
  DpeTaskRunner runner1;
  const DpeTaskResult r1 = runner1.run_direct(t0, prog);
  TaskDeltaSet ds1{compute_task_id(t0), false, r1.delta_records};
  const EpochCommitResult cr1 = commit_epoch(epoch, {ds1});

  // Run #2 (independent VM instance)
  DpeTaskRunner runner2;
  const DpeTaskResult r2 = runner2.run_direct(t0, prog);
  TaskDeltaSet ds2{compute_task_id(t0), false, r2.delta_records};
  const EpochCommitResult cr2 = commit_epoch(epoch, {ds2});

  check(cr1.ok(),  "[DPE-03-05-b] run1 commit ok");
  check(cr2.ok(),  "[DPE-03-05-b] run2 commit ok");
  check(cr1.epoch_hash == cr2.epoch_hash,
        "[DPE-03-05-b] EpochHash identical across two runs");
}

// ── [DPE-03-05-c] Float round-trips through predecessor snapshot ──────────────

static void test_float_predecessor_roundtrip() {
  std::printf("\n[DPE-03-05-c] Float value propagates intact through T0 → T1 snapshot\n");

  const double kValue = 1.41421356237;

  // T0: loads kValue, stores to page
  auto prog_t0 = make_float_store_program(kValue);

  // T1: reads the page (float), stores it back unchanged
  auto prog_t1 = make_float_passthrough_program();

  TaskDescriptor t0;
  t0.epoch_id = 305;
  t0.task_seq = 0;
  t0.output_regions.push_back(OutputRegion{kFloatPageTVA, 1, true});

  const TaskId t0_pid = program_identity(t0);

  TaskDescriptor t1;
  t1.epoch_id = 305;
  t1.task_seq = 1;
  t1.output_regions.push_back(OutputRegion{kFloat2PageTVA, 1, true});  // writes to fresh page
  t1.dep_task_ids.push_back(t0_pid);

  DpeTaskRunner runner;

  // Execute T0
  const DpeTaskResult r0 = runner.run_direct(t0, prog_t0);
  check(r0.halted,                 "[DPE-03-05-c] T0 halted");
  check(!r0.delta_records.empty(), "[DPE-03-05-c] T0 emitted delta");

  // Propagate T0's delta as T1's input snapshot
  DpeTaskInputSnapshot snap;
  for (const auto& rec : r0.delta_records) {
    snap.pages.emplace(rec.tva, DpePageSnapshot{rec.value, rec.word_tags});
  }

  // Execute T1 with snapshot
  const DpeTaskResult r1 = runner.run_direct(t1, prog_t1, snap);
  check(r1.halted,                 "[DPE-03-05-c] T1 halted");
  check(!r1.delta_records.empty(), "[DPE-03-05-c] T1 emitted delta");

  if (!r1.delta_records.empty()) {
    check(first_word_is_float(r1.delta_records[0]),
          "[DPE-03-05-c] T1 delta word is FloatHandle-tagged");
    check(first_float(r1.delta_records[0]) == kValue,
          "[DPE-03-05-c] T1 delta contains original float value (lossless round-trip)");
  }
}

// ── [DPE-03-05-d] Same float value in differently-ordered pools → same bytes ──

static void test_float_content_addressable() {
  std::printf("\n[DPE-03-05-d] Same float value from different pool positions → same delta bytes\n");

  const double kValue = 0.5;

  // Program A: float_pool = {kValue} — pool[0], handle = 1
  t81::tisc::Program prog_a;
  prog_a.float_pool = {kValue};
  prog_a.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 1, 0, t81::tisc::LiteralKind::FloatHandle},
    {t81::tisc::Opcode::Store, static_cast<std::int32_t>(kFloatPageTVA), 1},
    {t81::tisc::Opcode::Halt},
  };

  // Program B: float_pool = {99.0, kValue} — kValue is pool[1], handle = 2
  t81::tisc::Program prog_b;
  prog_b.float_pool = {99.0, kValue};
  prog_b.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 2, 0, t81::tisc::LiteralKind::FloatHandle},
    {t81::tisc::Opcode::Store, static_cast<std::int32_t>(kFloatPageTVA), 1},
    {t81::tisc::Opcode::Halt},
  };

  TaskDescriptor td;
  td.epoch_id = 305;
  td.task_seq = 0;
  td.output_regions.push_back(OutputRegion{kFloatPageTVA, 1, true});

  DpeTaskRunner runner;
  const DpeTaskResult ra = runner.run_direct(td, prog_a);
  const DpeTaskResult rb = runner.run_direct(td, prog_b);

  check(ra.halted && rb.halted,              "[DPE-03-05-d] both tasks halted");
  check(!ra.delta_records.empty() && !rb.delta_records.empty(),
        "[DPE-03-05-d] both emitted deltas");

  if (!ra.delta_records.empty() && !rb.delta_records.empty()) {
    check(first_word_is_float(ra.delta_records[0]),
          "[DPE-03-05-d] prog_a word is FloatHandle-tagged");
    check(first_word_is_float(rb.delta_records[0]),
          "[DPE-03-05-d] prog_b word is FloatHandle-tagged");
    check(first_float(ra.delta_records[0]) == kValue,
          "[DPE-03-05-d] prog_a delta value == kValue");
    check(first_float(rb.delta_records[0]) == kValue,
          "[DPE-03-05-d] prog_b delta value == kValue");
    // The first 8 bytes of the page must be identical (both encode kValue).
    check(ra.delta_records[0].value[0] == rb.delta_records[0].value[0] &&
          ra.delta_records[0].value[1] == rb.delta_records[0].value[1] &&
          ra.delta_records[0].value[2] == rb.delta_records[0].value[2] &&
          ra.delta_records[0].value[3] == rb.delta_records[0].value[3] &&
          ra.delta_records[0].value[4] == rb.delta_records[0].value[4] &&
          ra.delta_records[0].value[5] == rb.delta_records[0].value[5] &&
          ra.delta_records[0].value[6] == rb.delta_records[0].value[6] &&
          ra.delta_records[0].value[7] == rb.delta_records[0].value[7],
          "[DPE-03-05-d] delta bytes identical regardless of pool position");
  }
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== DPE Float Determinism Test [DPE-03-05] ===\n");

  test_float_delta_contains_canonical_bytes();
  test_float_epoch_hash_determinism();
  test_float_predecessor_roundtrip();
  test_float_content_addressable();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
