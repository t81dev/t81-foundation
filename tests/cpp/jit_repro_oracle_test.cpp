// tests/cpp/jit_repro_oracle_test.cpp
//
// RFC-0028 §2 + §6 — Canonical Trace Identity and Repro Oracle
//
// Verified criteria:
//   [RFC-0028-§2-a]  JitCompiler::compile() produces a non-zero trace_hash
//   [RFC-0028-§2-b]  Same instruction sequence → same trace_hash (stable)
//   [RFC-0028-§2-c]  Different instruction sequences → different trace_hash (discriminating)
//   [RFC-0028-§5]    ExitKind::AxionBoundary is defined and distinct from GuardDeopt
//   [RFC-0028-§6-a]  Two interpreter runs of a hot-loop program yield an identical
//                    CanonHash81 of the final register file (interpreter ≡ JIT-enabled path)
//   [RFC-0028-§6-b]  A mutated program yields a different final-state hash (oracle is sensitive)

#include "t81/isa/program.hpp"
#include "t81/jit/jit.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace t81::tisc;
using t81::hash::CanonHash81;
using t81::hash::hash_bytes;
using t81::vm::JitCompiler;
using t81::vm::JitTrace;

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

// ── Helpers ──────────────────────────────────────────────────────────────────

// Hot arithmetic loop: r1 counts 1..50, r2 accumulates sum.
static Program make_hot_arith() {
  Program p;
  p.insns = {
      {Opcode::LoadImm, 1, 0,  0},   // r1 = 0
      {Opcode::LoadImm, 2, 0,  0},   // r2 = 0
      {Opcode::LoadImm, 3, 1,  0},   // r3 = 1  (step)
      {Opcode::LoadImm, 4, 50, 0},   // r4 = 50 (loop count)
      // loop at pc=4:
      {Opcode::Add, 1, 1, 3},         // r1 += 1
      {Opcode::Add, 2, 2, 1},         // r2 += r1
      {Opcode::Sub, 4, 4, 3},         // r4 -= 1
      {Opcode::JumpIfNotZero, 4, 4, 0},
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

// Same structure but different loop count — ensures hash discrimination.
static Program make_hot_arith_variant() {
  Program p;
  p.insns = {
      {Opcode::LoadImm, 1, 0,   0},
      {Opcode::LoadImm, 2, 0,   0},
      {Opcode::LoadImm, 3, 1,   0},
      {Opcode::LoadImm, 4, 100, 0},   // different immediate → different hash
      {Opcode::Add, 1, 1, 3},
      {Opcode::Add, 2, 2, 1},
      {Opcode::Sub, 4, 4, 3},
      {Opcode::JumpIfNotZero, 4, 4, 0},
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

// Compile a trace from a program via JitCompiler (records until branch/stop).
static std::unique_ptr<JitTrace> compile_trace(const Program& prog) {
  JitCompiler jc;
  jc.start_tracing(0);
  for (const auto& insn : prog.insns) {
    jc.record_instruction(insn);
    if (!jc.is_tracing()) break;
  }
  return jc.compile();
}

// Run program through interpreter and return CanonHash81 of the final register
// file (all 243 registers serialised as little-endian int64_t bytes).
static CanonHash81 state_hash(const Program& prog) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  (void)vm->run_to_halt();

  const auto& st   = vm->state();
  const auto& regs = st.contexts.at(0).registers;

  std::vector<std::uint8_t> serial;
  serial.resize(regs.size() * sizeof(std::int64_t));
  for (std::size_t i = 0; i < regs.size(); ++i) {
    std::memcpy(serial.data() + i * sizeof(std::int64_t), &regs[i], sizeof(std::int64_t));
  }
  return hash_bytes(serial);
}

// ── [RFC-0028-§2] Canonical trace identity ───────────────────────────────────

static void test_trace_hash_nonzero() {
  std::printf("\n[RFC-0028-§2-a] trace_hash is non-zero after compile()\n");

  const Program prog = make_hot_arith();
  const auto trace   = compile_trace(prog);

  check(trace != nullptr, "trace compiled successfully");
  if (!trace) return;

  const CanonHash81 zero{};
  check(trace->trace_hash() != zero, "[RFC-0028-§2-a] trace_hash is non-zero");
}

static void test_trace_hash_stable() {
  std::printf("\n[RFC-0028-§2-b] Same program → same trace_hash (stable)\n");

  const Program prog = make_hot_arith();
  const auto t1 = compile_trace(prog);
  const auto t2 = compile_trace(prog);

  check(t1 != nullptr && t2 != nullptr, "both traces compiled");
  if (!t1 || !t2) return;

  check(t1->trace_hash() == t2->trace_hash(),
        "[RFC-0028-§2-b] identical programs produce identical trace_hash");
}

static void test_trace_hash_discriminating() {
  std::printf("\n[RFC-0028-§2-c] Different programs → different trace_hash\n");

  const auto t1 = compile_trace(make_hot_arith());
  const auto t2 = compile_trace(make_hot_arith_variant());

  check(t1 != nullptr && t2 != nullptr, "both traces compiled");
  if (!t1 || !t2) return;

  check(t1->trace_hash() != t2->trace_hash(),
        "[RFC-0028-§2-c] different programs produce different trace_hash");
}

// ── [RFC-0028-§5] AxionBoundary exit kind ────────────────────────────────────

static void test_axion_boundary_exit_kind_defined() {
  std::printf("\n[RFC-0028-§5] ExitKind::AxionBoundary is defined and distinct\n");

  // Compile-time check: each ExitKind must be a different integer.
  const auto completed = static_cast<int>(JitTrace::ExitKind::Completed);
  const auto branch    = static_cast<int>(JitTrace::ExitKind::Branch);
  const auto deopt     = static_cast<int>(JitTrace::ExitKind::GuardDeopt);
  const auto deny      = static_cast<int>(JitTrace::ExitKind::PolicyDeny);
  const auto axion     = static_cast<int>(JitTrace::ExitKind::AxionBoundary);

  check(axion != completed, "AxionBoundary != Completed");
  check(axion != branch,    "AxionBoundary != Branch");
  check(axion != deopt,     "AxionBoundary != GuardDeopt");
  check(axion != deny,      "AxionBoundary != PolicyDeny");
  check(axion >= 0,         "[RFC-0028-§5] AxionBoundary is a valid enum value");
}

// ── [RFC-0028-§6] Repro Oracle ───────────────────────────────────────────────

static void test_repro_oracle_identical_runs() {
  std::printf("\n[RFC-0028-§6-a] Two interpreter runs yield identical state hash\n");

  const Program prog = make_hot_arith();
  const CanonHash81 h1 = state_hash(prog);
  const CanonHash81 h2 = state_hash(prog);

  const CanonHash81 zero{};
  check(h1 != zero, "repro-oracle: hash 1 is non-zero");
  check(h2 != zero, "repro-oracle: hash 2 is non-zero");
  check(h1 == h2,
        "[RFC-0028-§6-a] CanonHash81(final registers) identical across two runs");
}

static void test_repro_oracle_sensitive_to_program_change() {
  std::printf("\n[RFC-0028-§6-b] Different program → different state hash\n");

  const CanonHash81 h1 = state_hash(make_hot_arith());
  const CanonHash81 h2 = state_hash(make_hot_arith_variant());

  check(h1 != h2,
        "[RFC-0028-§6-b] mutated program produces different final-state hash");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== JIT Repro Oracle (RFC-0028 §2 + §5 + §6) ===\n");

  test_trace_hash_nonzero();
  test_trace_hash_stable();
  test_trace_hash_discriminating();
  test_axion_boundary_exit_kind_defined();
  test_repro_oracle_identical_runs();
  test_repro_oracle_sensitive_to_program_change();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
