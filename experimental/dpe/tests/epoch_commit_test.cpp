// experimental/dpe/tests/epoch_commit_test.cpp
//
// RFC-DPE-0003 conformance tests for the epoch canonical commit engine.
//
// Acceptance criteria covered:
//   [DPE-03-01]  Canonical commit produces identical committed state
//                regardless of task execution order across independent runs
//   [DPE-03-02]  Last-writer-in-canonical-order conflict resolution produces
//                the correct result for overlapping non-exclusive writes
//   [DPE-03-03]  Epoch abort on TaskFault leaves canonical state unchanged
//   [DPE-03-04]  EpochHash is identical across two independent executions
//                of the same epoch against the same input_snapshot

#include "../epoch_commit.hpp"

#include <cassert>
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

// ── Helpers ───────────────────────────────────────────────────────────────────

static constexpr uint64_t kPageA = 0x1000;  // page-aligned TVA for region A
static constexpr uint64_t kPageB = 0x5000;  // page-aligned TVA for region B

static TaskDescriptor make_task(uint64_t epoch_id, uint64_t task_seq,
                                 std::vector<OutputRegion> regions = {}) {
  TaskDescriptor t;
  t.epoch_id       = epoch_id;
  t.task_seq       = task_seq;
  t.output_regions = std::move(regions);
  return t;
}

// Build a page_data buffer filled with a single byte value.
static std::array<std::byte, kDpePageSize> make_page(uint8_t fill) {
  std::array<std::byte, kDpePageSize> buf{};
  buf.fill(static_cast<std::byte>(fill));
  return buf;
}

static EpochGraph make_epoch(uint64_t epoch_id,
                              std::vector<TaskDescriptor> tasks) {
  EpochGraph eg;
  eg.epoch_id = epoch_id;
  eg.tasks    = std::move(tasks);
  return eg;
}

// ── [DPE-03-01] Deterministic commit ordering ────────────────────────────────

static void test_commit_ordering_is_deterministic() {
  std::printf("\n[DPE-03-01] Canonical commit produces identical state regardless of submission order\n");

  // Two tasks: T0 writes page A with 0xAA, T1 writes page B with 0xBB.
  // Neither overlaps — committed state must contain both regardless of
  // which order we submit to commit_epoch.
  TaskDescriptor t0 = make_task(1, 0, {OutputRegion{kPageA, 1, false}});
  TaskDescriptor t1 = make_task(1, 1, {OutputRegion{kPageB, 1, false}});
  const TaskId id0 = compute_task_id(t0);
  const TaskId id1 = compute_task_id(t1);

  DeltaBuffer buf0(t0, id0);
  DeltaBuffer buf1(t1, id1);

  const auto page_aa = make_page(0xAA);
  const auto page_bb = make_page(0xBB);
  (void)buf0.write(kPageA, std::span<const std::byte>(page_aa));
  (void)buf1.write(kPageB, std::span<const std::byte>(page_bb));

  EpochGraph epoch = make_epoch(1, {t0, t1});

  using BufPairs = std::vector<std::pair<TaskId, const DeltaBuffer*>>;

  // Run 1: submit in (T0, T1) order.
  const auto r1 = commit_epoch(epoch, BufPairs{{id0, &buf0}, {id1, &buf1}});
  check(r1.ok(), "det-order: commit run 1 ok");
  check(r1.committed_pages.count(kPageA) == 1,
        "det-order: run 1 page A present");
  check(r1.committed_pages.count(kPageB) == 1,
        "det-order: run 1 page B present");

  // Run 2: submit in reversed (T1, T0) order.
  const auto r2 = commit_epoch(epoch, BufPairs{{id1, &buf1}, {id0, &buf0}});
  check(r2.ok(), "det-order: commit run 2 ok");

  // Committed pages must be identical regardless of submission order.
  check(r1.committed_pages == r2.committed_pages,
        "[DPE-03-01] committed_pages identical between submission orders");
  check(r1.epoch_hash == r2.epoch_hash,
        "[DPE-03-01] epoch_hash identical between submission orders");

  // Verify content: page A holds 0xAA, page B holds 0xBB.
  check(r1.committed_pages.at(kPageA)[0] == std::byte{0xAA},
        "det-order: page A content == 0xAA");
  check(r1.committed_pages.at(kPageB)[0] == std::byte{0xBB},
        "det-order: page B content == 0xBB");
}

// ── [DPE-03-02] Last-writer-in-canonical-order conflict resolution ────────────

static void test_last_writer_in_canonical_order_wins() {
  std::printf("\n[DPE-03-02] Last-writer-in-canonical-order wins for overlapping non-exclusive pages\n");

  // Two tasks both write to kPageA with different fill bytes.
  // The task with the HIGHER TaskId must win (its value survives in committed_pages).
  TaskDescriptor t0 = make_task(2, 0, {OutputRegion{kPageA, 1, false}});
  TaskDescriptor t1 = make_task(2, 1, {OutputRegion{kPageA, 1, false}});
  const TaskId id0 = compute_task_id(t0);
  const TaskId id1 = compute_task_id(t1);

  // Determine which TaskId is higher (commit applies lower first, higher last).
  const bool id0_higher = !(id0 < id1);  // id0 > id1 iff not (id0 < id1)
  const uint8_t lower_fill  = 0x11;
  const uint8_t higher_fill = 0xFF;

  DeltaBuffer buf0(t0, id0);
  DeltaBuffer buf1(t1, id1);
  const auto page_for_id0 = make_page(id0_higher ? higher_fill : lower_fill);
  const auto page_for_id1 = make_page(id0_higher ? lower_fill  : higher_fill);
  (void)buf0.write(kPageA, std::span<const std::byte>(page_for_id0));
  (void)buf1.write(kPageA, std::span<const std::byte>(page_for_id1));

  using BufPairs = std::vector<std::pair<TaskId, const DeltaBuffer*>>;
  EpochGraph epoch = make_epoch(2, {t0, t1});
  const auto r = commit_epoch(epoch, BufPairs{{id0, &buf0}, {id1, &buf1}});

  check(r.ok(), "last-writer: commit ok");
  check(r.committed_pages.count(kPageA) == 1, "last-writer: page A present");
  check(r.committed_pages.at(kPageA)[0] == std::byte{higher_fill},
        "[DPE-03-02] higher-TaskId task's page value survives in committed state");

  // The lower-TaskId task's value must NOT be present.
  check(r.committed_pages.at(kPageA)[0] != std::byte{lower_fill},
        "[DPE-03-02] lower-TaskId task's page value is overwritten");
}

// ── [DPE-03-03] Epoch abort on TaskFault ─────────────────────────────────────

static void test_epoch_aborts_on_task_fault() {
  std::printf("\n[DPE-03-03] Epoch abort on TaskFault leaves canonical state unchanged\n");

  // T0: healthy, writes to page A.
  TaskDescriptor t0 = make_task(3, 0, {OutputRegion{kPageA, 1, false}});
  const TaskId id0 = compute_task_id(t0);
  DeltaBuffer buf0(t0, id0);
  const auto page_aa = make_page(0xAA);
  (void)buf0.write(kPageA, std::span<const std::byte>(page_aa));
  check(!buf0.faulted(), "abort: buf0 is not faulted after in-region write");

  // T1: faulted — writes outside declared region (scratch_pages == 0).
  TaskDescriptor t1 = make_task(3, 1, {OutputRegion{kPageA, 1, false}});
  t1.scratch_pages = 0;
  const TaskId id1 = compute_task_id(t1);
  DeltaBuffer buf1(t1, id1);
  const auto bad_page = make_page(0xCC);
  (void)buf1.write(kPageB, std::span<const std::byte>(bad_page));  // outside declared region
  check(buf1.faulted(), "abort: buf1 is faulted after out-of-region write");

  using BufPairs = std::vector<std::pair<TaskId, const DeltaBuffer*>>;
  EpochGraph epoch = make_epoch(3, {t0, t1});
  const auto r = commit_epoch(epoch, BufPairs{{id0, &buf0}, {id1, &buf1}});

  check(!r.ok(), "abort: commit result is not ok");
  check(r.status == EpochCommitStatus::Aborted_TaskFault,
        "abort: status == Aborted_TaskFault");
  check(r.faulting_task_id.has_value(),
        "abort: faulting_task_id is set");
  check(r.faulting_task_id.value() == id1,
        "abort: faulting_task_id matches the faulted task");

  // Canonical state is unchanged: committed_pages must be empty (no partial commit).
  check(r.committed_pages.empty(),
        "[DPE-03-03] committed_pages is empty — canonical state unchanged on abort");

  // epoch_hash is zero-valued (not computed on abort).
  const t81::hash::CanonHash81 zero_hash{};
  check(r.epoch_hash == zero_hash,
        "abort: epoch_hash is zero-valued (not computed on abort)");
}

// ── [DPE-03-04] EpochHash reproducibility ────────────────────────────────────

static void test_epoch_hash_is_reproducible() {
  std::printf("\n[DPE-03-04] EpochHash is identical across two independent executions\n");

  TaskDescriptor t0 = make_task(4, 0, {OutputRegion{kPageA, 1, false}});
  TaskDescriptor t1 = make_task(4, 1, {OutputRegion{kPageB, 1, false}});
  const TaskId id0 = compute_task_id(t0);
  const TaskId id1 = compute_task_id(t1);

  // Independent execution 1.
  DeltaBuffer buf0a(t0, id0);
  DeltaBuffer buf1a(t1, id1);
  const auto page_aa = make_page(0xAA);
  const auto page_bb = make_page(0xBB);
  (void)buf0a.write(kPageA, std::span<const std::byte>(page_aa));
  (void)buf1a.write(kPageB, std::span<const std::byte>(page_bb));

  // Independent execution 2 (fresh DeltaBuffers, identical program).
  DeltaBuffer buf0b(t0, id0);
  DeltaBuffer buf1b(t1, id1);
  (void)buf0b.write(kPageA, std::span<const std::byte>(page_aa));
  (void)buf1b.write(kPageB, std::span<const std::byte>(page_bb));

  using BufPairs = std::vector<std::pair<TaskId, const DeltaBuffer*>>;
  EpochGraph epoch = make_epoch(4, {t0, t1});
  const auto r1 = commit_epoch(epoch, BufPairs{{id0, &buf0a}, {id1, &buf1a}});
  const auto r2 = commit_epoch(epoch, BufPairs{{id0, &buf0b}, {id1, &buf1b}});

  check(r1.ok(), "repro: run 1 ok");
  check(r2.ok(), "repro: run 2 ok");
  check(r1.epoch_hash == r2.epoch_hash,
        "[DPE-03-04] epoch_hash identical across two independent executions");
  check(r1.committed_pages == r2.committed_pages,
        "[DPE-03-04] committed_pages identical across two independent executions");
}

// ── [DPE-03-structural] EpochHash encodes epoch identity ─────────────────────

static void test_epoch_hash_encodes_epoch_identity() {
  std::printf("\n[DPE-03-structural] Different epoch_id produces different EpochHash\n");

  TaskDescriptor t0 = make_task(5, 0, {OutputRegion{kPageA, 1, false}});
  const TaskId id0 = compute_task_id(t0);
  const auto page_aa = make_page(0xAA);

  // Epoch 5 and epoch 6 — same task and delta, different epoch_id.
  DeltaBuffer buf5(t0, id0);
  (void)buf5.write(kPageA, std::span<const std::byte>(page_aa));

  TaskDescriptor t0b = make_task(6, 0, {OutputRegion{kPageA, 1, false}});
  const TaskId id0b = compute_task_id(t0b);
  DeltaBuffer buf6(t0b, id0b);
  (void)buf6.write(kPageA, std::span<const std::byte>(page_aa));

  EpochGraph epoch5 = make_epoch(5, {t0});
  EpochGraph epoch6 = make_epoch(6, {t0b});

  using BufPairs = std::vector<std::pair<TaskId, const DeltaBuffer*>>;
  const auto r5 = commit_epoch(epoch5, BufPairs{{id0,  &buf5}});
  const auto r6 = commit_epoch(epoch6, BufPairs{{id0b, &buf6}});

  check(r5.ok() && r6.ok(), "identity: both commits ok");
  check(r5.epoch_hash != r6.epoch_hash,
        "identity: different epoch_id → different EpochHash");
}

// ── [DPE-03-structural] Empty delta buffer produces valid EpochHash ───────────

static void test_empty_delta_buffer_produces_valid_hash() {
  std::printf("\n[DPE-03-structural] Task with no output regions produces valid EpochHash\n");

  // A task with no output regions produces an empty DeltaBuffer.
  // commit_epoch should succeed and produce a non-zero EpochHash.
  TaskDescriptor t0 = make_task(7, 0);  // no output_regions
  const TaskId id0 = compute_task_id(t0);
  DeltaBuffer buf0(t0, id0);  // empty — no writes

  using BufPairs = std::vector<std::pair<TaskId, const DeltaBuffer*>>;
  EpochGraph epoch = make_epoch(7, {t0});
  const auto r = commit_epoch(epoch, BufPairs{{id0, &buf0}});

  check(r.ok(), "empty-buf: commit ok");
  check(r.committed_pages.empty(), "empty-buf: no committed pages");

  // EpochHash must be deterministic even for an empty delta set.
  DeltaBuffer buf0b(t0, id0);
  const auto r2 = commit_epoch(epoch, BufPairs{{id0, &buf0b}});
  check(r.epoch_hash == r2.epoch_hash,
        "empty-buf: EpochHash reproducible for empty delta buffer");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== DPE Epoch Commit tests (RFC-DPE-0003) ===\n");

  test_commit_ordering_is_deterministic();
  test_last_writer_in_canonical_order_wins();
  test_epoch_aborts_on_task_fault();
  test_epoch_hash_is_reproducible();
  test_epoch_hash_encodes_epoch_identity();
  test_empty_delta_buffer_produces_valid_hash();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
