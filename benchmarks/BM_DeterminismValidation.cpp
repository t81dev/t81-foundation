// benchmarks/BM_DeterminismValidation.cpp
//
// RFC-00A2 — Determinism Validation Benchmark
//
// Measures the T81 VM's bit-exact reproducibility guarantee by running a
// fixed TISC program N times and verifying that all runs produce identical
// CanonHash81 hashes of the final VM state.
//
// Benchmark counters:
//   unique_hashes       — distinct CanonHash81 values observed (must be 1)
//   determinism_score   — 1.0 iff all hashes identical, else fraction correct
//   ops_per_run         — TISC instructions in the workload program

#include <benchmark/benchmark.h>

#include <array>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/vm.hpp"

namespace {

using t81::tisc::Insn;
using t81::tisc::Opcode;
using t81::tisc::Program;

// ── Workload programs ─────────────────────────────────────────────────────────

// A small fixed-point integer arithmetic program.
// r0 = 81; r1 = 27; r2 = r0 + r1 = 108; r3 = 3; r4 = r2 * r3 = 324
// Deterministically produces r4 = 324 every run.
Program make_arith_workload() {
  Program p;
  p.insns.push_back({Opcode::LoadImm, 0, 81, 0});   // r0 = 81
  p.insns.push_back({Opcode::LoadImm, 1, 27, 0});   // r1 = 27
  p.insns.push_back({Opcode::Add,     2, 0, 1});    // r2 = r0 + r1
  p.insns.push_back({Opcode::LoadImm, 3, 3, 0});    // r3 = 3
  p.insns.push_back({Opcode::Mul,     4, 2, 3});    // r4 = r2 * r3
  p.insns.push_back({Opcode::Halt,    0, 0, 0});
  return p;
}

// A longer arithmetic chain — 40 LoadImm+Add pairs — to exercise more
// of the dispatch loop.
Program make_arith_chain_workload() {
  Program p;
  // Seed r0 = 1
  p.insns.push_back({Opcode::LoadImm, 0, 1, 0});
  // 40 iterations: r0 += i  (i = 1..40)
  for (int i = 1; i <= 40; ++i) {
    p.insns.push_back({Opcode::LoadImm, 1, i, 0});  // r1 = i
    p.insns.push_back({Opcode::Add,     0, 0, 1});  // r0 += r1
  }
  p.insns.push_back({Opcode::Halt, 0, 0, 0});
  return p;
}

// ── State serialization ───────────────────────────────────────────────────────

// Serialise the first 8 registers + halted flag to a reproducible string.
// Using only 8 registers keeps serialisation fast while still covering the
// outputs written by the workload programs above.
std::string serialize_vm_state(const t81::vm::State& st) {
  std::ostringstream oss;
  if (!st.contexts.empty()) {
    const auto& ctx = st.contexts[0];
    for (int i = 0; i < 8; ++i) {
      oss << ctx.registers[i] << ',';
    }
    oss << ctx.flags.zero << ',' << ctx.flags.negative << ',' << ctx.flags.positive;
  }
  oss << '|' << (st.halted ? '1' : '0');
  // Include printed output so any Print-opcode output is part of the hash.
  for (const auto& line : st.printed_output) {
    oss << '|' << line;
  }
  return oss.str();
}

// ── Benchmark helpers ─────────────────────────────────────────────────────────

// Run `program` N times; return (unique_hash_count, total_runs).
std::pair<int, int> measure_determinism(const Program& program, int n) {
  std::unordered_map<std::string, int> seen;
  for (int i = 0; i < n; ++i) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt(10000);
    if (!result.has_value()) {
      // Trap: count as a distinct "hash" — forces determinism_score < 1.
      seen["TRAP"]++;
      continue;
    }
    const std::string serial = serialize_vm_state(vm->state());
    auto h = t81::hash::hash_string(serial);
    seen[h.to_string()]++;
  }
  return {static_cast<int>(seen.size()), n};
}

// ── Benchmarks ────────────────────────────────────────────────────────────────

// Measures throughput of a single VM run (no hashing).
static void BM_VMRun_Arith(benchmark::State& state) {
  const Program prog = make_arith_workload();
  const int n_ops = static_cast<int>(prog.insns.size());

  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(prog);
    auto r = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(r);
  }
  state.SetItemsProcessed(state.iterations() * n_ops);
  state.counters["ops_per_run"] = static_cast<double>(n_ops);
  state.SetLabel("deterministic arithmetic workload");
}
BENCHMARK(BM_VMRun_Arith)->Repetitions(3);

// Runs the arithmetic chain and measures single-run throughput.
static void BM_VMRun_ArithChain(benchmark::State& state) {
  const Program prog = make_arith_chain_workload();
  const int n_ops = static_cast<int>(prog.insns.size());

  for (auto _ : state) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(prog);
    auto r = vm->run_to_halt(10000);
    benchmark::DoNotOptimize(r);
  }
  state.SetItemsProcessed(state.iterations() * n_ops);
  state.counters["ops_per_run"] = static_cast<double>(n_ops);
  state.SetLabel("deterministic arithmetic chain (40 add ops)");
}
BENCHMARK(BM_VMRun_ArithChain)->Repetitions(3);

// Validates determinism: runs the workload N times, counts unique hashes.
// determinism_score must be 1.0; unique_hashes must be 1.
static void BM_DeterminismValidation_Arith(benchmark::State& state) {
  const int num_runs = static_cast<int>(state.range(0));
  const Program prog = make_arith_workload();

  int unique_hashes = 0;
  int total = 0;
  for (auto _ : state) {
    auto [u, t] = measure_determinism(prog, num_runs);
    unique_hashes = u;
    total = t;
    benchmark::DoNotOptimize(unique_hashes);
  }
  const double score =
      (unique_hashes == 1 && total > 0) ? 1.0 : (1.0 - (unique_hashes - 1.0) / total);
  state.counters["unique_hashes"]     = static_cast<double>(unique_hashes);
  state.counters["determinism_score"] = score;
  state.counters["num_runs"]          = static_cast<double>(num_runs);
  state.SetLabel("CanonHash81 cross-run consistency");
}
BENCHMARK(BM_DeterminismValidation_Arith)->Arg(10)->Arg(50)->Repetitions(2);

// Same determinism check on the longer arithmetic chain.
static void BM_DeterminismValidation_ArithChain(benchmark::State& state) {
  const int num_runs = static_cast<int>(state.range(0));
  const Program prog = make_arith_chain_workload();

  int unique_hashes = 0;
  int total = 0;
  for (auto _ : state) {
    auto [u, t] = measure_determinism(prog, num_runs);
    unique_hashes = u;
    total = t;
    benchmark::DoNotOptimize(unique_hashes);
  }
  const double score =
      (unique_hashes == 1 && total > 0) ? 1.0 : (1.0 - (unique_hashes - 1.0) / total);
  state.counters["unique_hashes"]     = static_cast<double>(unique_hashes);
  state.counters["determinism_score"] = score;
  state.counters["num_runs"]          = static_cast<double>(num_runs);
  state.SetLabel("CanonHash81 chain cross-run consistency");
}
BENCHMARK(BM_DeterminismValidation_ArithChain)->Arg(10)->Arg(50)->Repetitions(2);

}  // namespace
