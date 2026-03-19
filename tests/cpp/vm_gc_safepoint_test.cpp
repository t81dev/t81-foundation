// tests/cpp/vm_gc_safepoint_test.cpp
//
// RFC-0006 §2.3 + §2.4 — Deterministic GC graduation tests
//
// Verified criteria:
//   [RFC-0006-§2.3-a]  GcSafepoint opcode triggers a GC cycle (gc_cycles increments)
//   [RFC-0006-§2.3-b]  GcSafepoint is a valid, named opcode in kAllOpcodes
//   [RFC-0006-§2.3-c]  Interval-based trigger fires (gc_cycles > 0 after kGcInterval insns)
//   [RFC-0006-§2.4-a]  Axion policy Deny on GC cycle → SecurityFault trap

#include "t81/axion/engine.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <cstdio>
#include <string_view>

using namespace t81::tisc;
using t81::vm::Trap;

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

// ── Selective deny engine: only denies requests whose syscall == kGcCycle ─────

namespace {
class GcDenyEngine : public t81::axion::Engine {
 public:
  t81::axion::Verdict evaluate(const t81::axion::SyscallContext& ctx) override {
    if (ctx.syscall == t81::axion::reasons::kGcCycle) {
      return {t81::axion::VerdictKind::Deny, "gc-deny-test"};
    }
    return {t81::axion::VerdictKind::Allow, "pass"};
  }
};
}  // namespace

// ── [RFC-0006-§2.3-b] GcSafepoint opcode is valid and named ──────────────────

static void test_gcsafepoint_opcode_is_named() {
  std::printf("\n[RFC-0006-§2.3-b] GcSafepoint opcode is valid and named\n");

  const std::string_view name = opcode_name(Opcode::GcSafepoint);
  check(name == "GcSafepoint", "[RFC-0006-§2.3-b] opcode_name(GcSafepoint) == \"GcSafepoint\"");

  // Verify it appears in kAllOpcodes
  bool found = false;
  for (Opcode op : kAllOpcodes) {
    if (op == Opcode::GcSafepoint) {
      found = true;
      break;
    }
  }
  check(found, "[RFC-0006-§2.3-b] GcSafepoint is in kAllOpcodes");
}

// ── [RFC-0006-§2.3-a] GcSafepoint opcode triggers a GC cycle ─────────────────
//
// Program:
//   LoadImm r1, 1
//   GcSafepoint          ← explicit safepoint
//   Halt

static void test_gcsafepoint_increments_gc_cycles() {
  std::printf("\n[RFC-0006-§2.3-a] GcSafepoint opcode triggers a GC cycle\n");

  Program prog;
  prog.insns = {
      {Opcode::LoadImm, 1, 1, 0},
      {Opcode::GcSafepoint, 0, 0, 0},
      {Opcode::Halt, 0, 0, 0},
  };

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto res = vm->run_to_halt(1000);

  check(res.has_value(), "[RFC-0006-§2.3-a] program runs without error");
  check(vm->state().gc_cycles >= 1,
        "[RFC-0006-§2.3-a] gc_cycles >= 1 after explicit GcSafepoint");
}

// ── [RFC-0006-§2.3-c] Interval-based trigger fires ───────────────────────────
//
// Run more than kGcInterval (64) instructions and confirm gc_cycles > 0.
// We use a simple counted loop.

static void test_interval_trigger_fires() {
  std::printf("\n[RFC-0006-§2.3-c] Interval-based GC trigger fires after 64+ instructions\n");

  // Loop 100 iterations: LoadImm(3) + loop body(3) × 100 = ~303 instructions
  Program prog;
  prog.insns = {
      {Opcode::LoadImm, 1, 0,   0},  // r1 = 0
      {Opcode::LoadImm, 2, 1,   0},  // r2 = 1 (step)
      {Opcode::LoadImm, 3, 100, 0},  // r3 = 100 (count)
      // loop at pc=3:
      {Opcode::Add, 1, 1, 2},                  // r1 += 1
      {Opcode::Sub, 3, 3, 2},                  // r3 -= 1
      {Opcode::JumpIfNotZero, 3, 3, 0},        // if r3 != 0 goto pc=3
      {Opcode::Halt, 0, 0, 0},
  };

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto res = vm->run_to_halt(10000);

  check(res.has_value(), "[RFC-0006-§2.3-c] long-running program completes without error");
  check(vm->state().gc_cycles > 0,
        "[RFC-0006-§2.3-c] gc_cycles > 0 after interval-based trigger");
}

// ── [RFC-0006-§2.4-a] Axion Deny on GC cycle → SecurityFault ─────────────────

static void test_gc_axion_deny_causes_security_fault() {
  std::printf("\n[RFC-0006-§2.4-a] Axion Deny on GC cycle produces SecurityFault\n");

  // A single GcSafepoint with the deny engine active should immediately trap.
  Program prog;
  prog.insns = {
      {Opcode::LoadImm, 1, 1, 0},
      {Opcode::GcSafepoint, 0, 0, 0},
      {Opcode::Halt, 0, 0, 0},
  };

  auto vm = t81::vm::make_interpreter_vm(std::make_unique<GcDenyEngine>());
  vm->load_program(prog);
  auto res = vm->run_to_halt(1000);

  check(!res.has_value(), "[RFC-0006-§2.4-a] run_to_halt returns error (policy denied GC)");
  if (!res.has_value()) {
    check(res.error() == Trap::SecurityFault,
          "[RFC-0006-§2.4-a] trap kind is SecurityFault");
  }
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== GC Safepoint Tests (RFC-0006 §2.3 + §2.4) ===\n");

  test_gcsafepoint_opcode_is_named();
  test_gcsafepoint_increments_gc_cycles();
  test_interval_trigger_fires();
  test_gc_axion_deny_causes_security_fault();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
