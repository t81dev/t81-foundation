#include "test_sig_util.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

using t81::test::sig_mix;
using t81::test::sig_mix_string;

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "vm_workload_determinism_tiers_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

// Alias kept for call-site readability within this file
inline std::uint64_t mix(std::uint64_t seed, std::uint64_t value) { return sig_mix(seed, value); }

std::uint64_t signature_for_program(const t81::tisc::Program& p, bool* ok) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(5000);
  *ok = run.has_value();
  if (!run.has_value()) return 0;

  const auto& st = vm->state();
  std::uint64_t sig = 1469598103934665603ULL;
  for (std::size_t i = 0; i < 48; ++i) {
    sig = mix(sig, static_cast<std::uint64_t>(st.contexts[0].registers[i]));
    sig = mix(sig, static_cast<std::uint64_t>(st.contexts[0].register_tags[i]));
  }
  sig = mix(sig, st.contexts[0].pc);
  sig = mix(sig, st.contexts[0].sp);
  sig = mix(sig, st.trace.size());
  sig = mix(sig, st.axion_log.size());
  for (const auto& e : st.trace) {
    sig = mix(sig, e.pc);
    sig = mix(sig, static_cast<std::uint64_t>(e.opcode));
    sig = mix(sig, e.trap.has_value() ? static_cast<std::uint64_t>(e.trap.value()) : 0ULL);
  }
  for (const auto& line : st.printed_output) {
    for (unsigned char c : line) sig = mix(sig, c);
  }
  for (const auto& ev : st.axion_log) {
    sig = mix(sig, static_cast<std::uint64_t>(ev.opcode));
    sig = mix(sig, static_cast<std::uint64_t>(ev.tag));
    sig = mix(sig, static_cast<std::uint64_t>(ev.value));
    for (unsigned char c : ev.verdict.reason) sig = mix(sig, c);
  }
  return sig;
}

// Workload program absorbed from vm_workload_determinism_test.cpp.
// Uses same loop structure plus stack/option/result path that the standalone test covered.
t81::tisc::Program workload_program() {
  using t81::tisc::Opcode;
  t81::tisc::Program p;
  p.insns.push_back({Opcode::LoadImm, 0, 0, 0});
  p.insns.push_back({Opcode::LoadImm, 1, 1, 0});
  p.insns.push_back({Opcode::LoadImm, 2, 81, 0});
  p.insns.push_back({Opcode::Add, 0, 0, 1});
  p.insns.push_back({Opcode::Less, 3, 0, 2});
  p.insns.push_back({Opcode::JumpIfNotZero, 3, 3, 0});
  p.insns.push_back({Opcode::Store, 140, 0, 0});
  p.insns.push_back({Opcode::Load, 4, 140, 0});
  p.insns.push_back({Opcode::Push, 4, 0, 0});
  p.insns.push_back({Opcode::Pop, 5, 0, 0});
  p.insns.push_back({Opcode::MakeOptionSome, 6, 5, 0});
  p.insns.push_back({Opcode::OptionUnwrap, 7, 6, 0});
  p.insns.push_back({Opcode::MakeResultOk, 8, 7, 0});
  p.insns.push_back({Opcode::ResultUnwrapOk, 9, 8, 0});
  p.insns.push_back({Opcode::Print, 9, 0, 0});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program micro_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 2, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Mul, 43, 42, 41});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program meso_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 42, 81, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 40, 40, 41});   // pc=3
  p.insns.push_back({t81::tisc::Opcode::Less, 43, 40, 42});  // pc=4
  p.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 3, 43, 0});
  p.insns.push_back({t81::tisc::Opcode::Store, 140, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::Load, 44, 140, 0});
  p.insns.push_back({t81::tisc::Opcode::Print, 44, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program mixed_program() {
  t81::tisc::Program p;
  p.axion_policy_text = "(policy (tier 1) (max-instructions 512) (max-stack 64))";
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 42, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeOptionSome, 41, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::OptionUnwrap, 42, 41, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeResultOk, 43, 42, 0});
  p.insns.push_back({t81::tisc::Opcode::ResultUnwrapOk, 44, 43, 0});
  p.insns.push_back({t81::tisc::Opcode::Push, 44, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Pop, 45, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Store, 160, 45, 0});
  p.insns.push_back({t81::tisc::Opcode::Load, 46, 160, 0});
  p.insns.push_back({t81::tisc::Opcode::Print, 46, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program policy_heavy_program() {
  t81::tisc::Program p;
  p.axion_policy_text =
      "(policy (tier 1) (max-instructions 2048) (max-stack 128) (max-meta-writes 64))";
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 12, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 5, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Push, 42, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Pop, 43, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::AxReport, 43, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program tensor_access_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 4, 0});
  p.insns.push_back({t81::tisc::Opcode::TNew, 41, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 42, 2, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 43, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 41, 42, 43});
  p.insns.push_back({t81::tisc::Opcode::TGet, 44, 41, 42});
  p.insns.push_back({t81::tisc::Opcode::Print, 44, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

bool validate_tier(const std::string& tier_id, const t81::tisc::Program& p, std::ofstream& out) {
  bool ok = false;
  const std::uint64_t baseline = signature_for_program(p, &ok);
  if (!expect(ok, tier_id + ": baseline run failed")) return false;
  out << tier_id << " baseline=" << baseline << "\n";
  for (int i = 0; i < 5; ++i) {
    bool run_ok = false;
    const std::uint64_t sig = signature_for_program(p, &run_ok);
    if (!expect(run_ok, tier_id + ": repeated run failed")) return false;
    if (!expect(sig == baseline, tier_id + ": signature drift")) return false;
    out << tier_id << " run" << i << "=" << sig << "\n";
  }
  return true;
}

}  // namespace

int main() {
  std::filesystem::create_directories("artifacts");
  std::ofstream log("artifacts/vm_workload_determinism_signatures.log", std::ios::trunc);
  if (!expect(static_cast<bool>(log), "failed to open signature artifact log")) return 1;

  if (!validate_tier("workload", workload_program(), log)) return 1;
  if (!validate_tier("micro", micro_program(), log)) return 1;
  if (!validate_tier("meso", meso_program(), log)) return 1;
  if (!validate_tier("mixed", mixed_program(), log)) return 1;
  if (!validate_tier("policy-heavy", policy_heavy_program(), log)) return 1;
  if (!validate_tier("tensor-access", tensor_access_program(), log)) return 1;

  log.flush();
  return 0;
}
