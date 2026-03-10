// experimental/ternaryos/tests/context_switch_test.cpp
//
// Unit tests for TISC context save / restore / yield.

#include "../sched/context_switch.hpp"

#include <cstdio>

using namespace t81::ternaryos::sched;

static int g_pass = 0;
static int g_fail = 0;

static bool check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
  return cond;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

static t81::vm::ThreadContext make_thread(std::size_t pc, std::int64_t r0,
                                      std::int64_t r1 = 0) {
  t81::vm::ThreadContext t{};
  t.pc             = pc;
  t.sp             = 0x800;
  t.registers[0]   = r0;
  t.registers[1]   = r1;
  t.register_tags[0] = t81::vm::ValueTag::Int;
  t.register_tags[1] = t81::vm::ValueTag::Int;
  t.flags.zero     = (r0 == 0);
  t.flags.negative = (r0 < 0);
  t.flags.positive = (r0 > 0);
  t.call_depth     = 2;
  t.stack_frames   = {{10, 20}, {30, 40}};
  t.halted         = false;
  t.active         = true;
  return t;
}

// ─── Tests ───────────────────────────────────────────────────────────────────

static void test_context_save() {
  std::printf("\n[CS-1] context_save captures full thread state\n");
  auto thread = make_thread(/*pc=*/42, /*r0=*/99, /*r1=*/-7);

  TiscContext ctx;
  ctx.tid   = 3;
  ctx.label = "worker";
  context_save(thread, ctx);

  check(ctx.pc             == 42,                     "pc captured");
  check(ctx.sp             == 0x800,                  "sp captured");
  check(ctx.registers[0]   == 99,                     "r0 captured");
  check(ctx.registers[1]   == -7,                     "r1 captured");
  check(ctx.register_tags[0] == t81::vm::ValueTag::Int,   "tag r0 captured");
  check(ctx.flags.positive == true,                   "positive flag captured");
  check(ctx.flags.negative == false,                  "negative flag clear");
  check(ctx.call_depth     == 2,                      "call_depth captured");
  check(ctx.stack_frames.size() == 2,                 "stack_frames captured");
  check(ctx.stack_frames[1]     == std::make_pair<std::int64_t,std::int64_t>(30, 40),
                                                      "stack_frame[1] value");
  check(ctx.halted         == false,                  "halted captured");
  check(ctx.active         == true,                   "active captured");
  // Preserved fields.
  check(ctx.tid   == 3,        "tid preserved");
  check(ctx.label == "worker", "label preserved");
  // After save the snapshot is Ready (off-CPU).
  check(ctx.state == ThreadState::Ready, "state → Ready after save");
}

static void test_context_restore() {
  std::printf("\n[CS-2] context_restore writes back to ThreadContext\n");
  TiscContext ctx{};
  ctx.pc           = 100;
  ctx.sp           = 0x400;
  ctx.registers[5] = 555;
  ctx.register_tags[5] = t81::vm::ValueTag::BigIntHandle;
  ctx.flags.zero   = true;
  ctx.call_depth   = 1;
  ctx.stack_frames = {{7, 8}};
  ctx.halted       = false;
  ctx.active       = true;

  t81::vm::ThreadContext tctx{};
  context_restore(ctx, tctx);

  check(tctx.pc           == 100,                          "pc restored");
  check(tctx.sp           == 0x400,                        "sp restored");
  check(tctx.registers[5] == 555,                          "r5 restored");
  check(tctx.register_tags[5] == t81::vm::ValueTag::BigIntHandle, "tag r5 restored");
  check(tctx.flags.zero   == true,                         "zero flag restored");
  check(tctx.call_depth   == 1,                            "call_depth restored");
  check(tctx.stack_frames.size() == 1,                     "stack_frames restored");
  check(tctx.halted       == false,                        "halted restored");
  check(tctx.active       == true,                         "active restored");
}

static void test_round_trip() {
  std::printf("\n[CS-3] save → restore round-trip is bit-exact\n");
  auto original = make_thread(/*pc=*/777, /*r0=*/42);
  // Write a non-trivial pattern into all 243 registers.
  for (int i = 0; i < 243; ++i) {
    original.registers[i]     = static_cast<std::int64_t>(i * i - 100);
    original.register_tags[i] = (i % 2 == 0) ? t81::vm::ValueTag::Int
                                              : t81::vm::ValueTag::TensorHandle;
  }

  TiscContext snapshot{};
  context_save(original, snapshot);

  t81::vm::ThreadContext restored{};
  context_restore(snapshot, restored);

  bool regs_match = (restored.registers == original.registers);
  bool tags_match = (restored.register_tags == original.register_tags);
  check(regs_match, "all 243 registers round-trip bit-exact");
  check(tags_match, "all 243 tags round-trip bit-exact");
  check(restored.pc         == original.pc,         "pc round-trips");
  check(restored.sp         == original.sp,         "sp round-trips");
  check(restored.call_depth == original.call_depth, "call_depth round-trips");
}

static void test_context_yield() {
  std::printf("\n[CS-4] context_yield swaps two threads\n");
  // Thread A: pc=10, r0=1
  auto thread_a = make_thread(10, 1);
  // Pre-saved context for thread B: pc=200, r0=99
  TiscContext ctx_b{};
  {
    auto thread_b = make_thread(200, 99);
    context_save(thread_b, ctx_b);
  }

  // Yield from A → B. out_a receives A's snapshot; thread_a now runs B.
  TiscContext out_a{};
  out_a.tid = 1;
  context_yield(thread_a, out_a, ctx_b);

  // thread_a should now reflect B's state.
  check(thread_a.pc        == 200, "thread_a.pc == B's pc after yield");
  check(thread_a.registers[0] == 99, "thread_a.r0 == B's r0 after yield");

  // out_a should hold A's original state.
  check(out_a.pc        == 10, "out_a.pc == A's original pc");
  check(out_a.registers[0] == 1, "out_a.r0 == A's original r0");
  check(out_a.state == ThreadState::Ready, "A's snapshot is Ready");
  check(out_a.tid == 1, "A's tid preserved in snapshot");
}

static void test_halted_thread_round_trip() {
  std::printf("\n[CS-5] Halted thread context saves and restores correctly\n");
  t81::vm::ThreadContext halted{};
  halted.pc     = 999;
  halted.halted = true;
  halted.active = false;

  TiscContext snap{};
  context_save(halted, snap);
  check(snap.halted == true,  "halted flag saved");
  check(snap.active == false, "active flag saved");

  t81::vm::ThreadContext restored{};
  context_restore(snap, restored);
  check(restored.halted == true,  "halted flag restored");
  check(restored.active == false, "active flag restored");
  check(restored.pc == 999,       "pc restored for halted thread");
}

static void test_thread_state_string() {
  std::printf("\n[CS-6] ThreadState::state_string()\n");
  TiscContext ctx{};
  ctx.state = ThreadState::Sleeping;
  check(ctx.state_string() == "Sleeping", "Sleeping string");
  ctx.state = ThreadState::Ready;
  check(ctx.state_string() == "Ready",    "Ready string");
  ctx.state = ThreadState::Running;
  check(ctx.state_string() == "Running",  "Running string");
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== TISC context-switch tests ===\n");

  test_context_save();
  test_context_restore();
  test_round_trip();
  test_context_yield();
  test_halted_thread_round_trip();
  test_thread_state_string();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
