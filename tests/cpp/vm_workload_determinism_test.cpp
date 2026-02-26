#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "vm_workload_determinism_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

void mix_u64(std::uint64_t value, std::uint64_t* state) {
  *state ^= value + 0x9e3779b97f4a7c15ULL + (*state << 6U) + (*state >> 2U);
}

void mix_string(const std::string& value, std::uint64_t* state) {
  for (unsigned char c : value) {
    mix_u64(static_cast<std::uint64_t>(c), state);
  }
}

t81::tisc::Program make_workload_program() {
  using t81::tisc::Opcode;
  t81::tisc::Program p;

  // Loop body: r0 += 1 until r0 == r2.
  p.insns.push_back({Opcode::LoadImm, 0, 0, 0});        // counter
  p.insns.push_back({Opcode::LoadImm, 1, 1, 0});        // step
  p.insns.push_back({Opcode::LoadImm, 2, 81, 0});       // loop limit
  p.insns.push_back({Opcode::Add, 0, 0, 1});            // pc=3
  p.insns.push_back({Opcode::Less, 3, 0, 2});           // pc=4
  p.insns.push_back({Opcode::JumpIfNotZero, 3, 3, 0});  // pc=5 -> 3

  // Memory + stack + option/result path.
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

std::uint64_t run_signature(const t81::tisc::Program& p, bool* ok) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(2000);
  *ok = run.has_value();
  if (!run.has_value()) return 0;

  const auto& st = vm->state();
  std::uint64_t sig = 1469598103934665603ULL;

  for (std::size_t i = 0; i < 32; ++i) {
    mix_u64(static_cast<std::uint64_t>(st.contexts[0].registers[i]), &sig);
    mix_u64(static_cast<std::uint64_t>(st.contexts[0].register_tags[i]), &sig);
  }
  mix_u64(st.contexts[0].pc, &sig);
  mix_u64(st.contexts[0].sp, &sig);
  mix_u64(static_cast<std::uint64_t>(st.halted), &sig);
  mix_u64(st.gc_cycles, &sig);

  const std::size_t mem_limit = st.memory.size() < 128 ? st.memory.size() : 128;
  for (std::size_t i = 0; i < mem_limit; ++i) {
    mix_u64(static_cast<std::uint64_t>(st.memory[i]), &sig);
    mix_u64(static_cast<std::uint64_t>(st.memory_tags[i]), &sig);
  }

  for (const auto& trace : st.trace) {
    mix_u64(trace.pc, &sig);
    mix_u64(static_cast<std::uint64_t>(trace.opcode), &sig);
    mix_u64(static_cast<std::uint64_t>(trace.trap.has_value()), &sig);
    if (trace.trap.has_value()) {
      mix_u64(static_cast<std::uint64_t>(trace.trap.value()), &sig);
    }
  }

  for (const auto& event : st.axion_log) {
    mix_u64(static_cast<std::uint64_t>(event.opcode), &sig);
    mix_u64(static_cast<std::uint64_t>(event.tag), &sig);
    mix_u64(static_cast<std::uint64_t>(event.value), &sig);
    mix_u64(static_cast<std::uint64_t>(event.verdict.kind), &sig);
    mix_string(event.verdict.reason, &sig);
  }
  for (const auto& line : st.printed_output) {
    mix_string(line, &sig);
  }

  return sig;
}

}  // namespace

int main() {
  const auto program = make_workload_program();
  bool ok = false;
  const std::uint64_t baseline = run_signature(program, &ok);
  if (!expect(ok, "baseline run failed")) return 1;

  for (int i = 0; i < 7; ++i) {
    bool run_ok = false;
    std::uint64_t sig = run_signature(program, &run_ok);
    if (!expect(run_ok, "repeated run failed")) return 1;
    if (!expect(sig == baseline, "workload signature drift across identical runs")) return 1;
  }

  return 0;
}
