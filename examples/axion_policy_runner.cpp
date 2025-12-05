#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

int main() {
  t81::tisc::Program program;
  program.tensor_pool.emplace_back(std::vector<int>{1}, std::vector<float>{3.14f});
  program.tensor_pool.emplace_back(std::vector<int>{1}, std::vector<float>{2.72f});

  t81::tisc::Insn load_tensor0{};
  load_tensor0.opcode = t81::tisc::Opcode::LoadImm;
  load_tensor0.a = 1;
  load_tensor0.b = 1;
  load_tensor0.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn load_tensor1{};
  load_tensor1.opcode = t81::tisc::Opcode::LoadImm;
  load_tensor1.a = 2;
  load_tensor1.b = 2;
  load_tensor1.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn vec_add{};
  vec_add.opcode = t81::tisc::Opcode::TVecAdd;
  vec_add.a = 3;
  vec_add.b = 1;
  vec_add.c = 2;

  t81::tisc::Insn stack_alloc{};
  stack_alloc.opcode = t81::tisc::Opcode::StackAlloc;
  stack_alloc.a = 4;
  stack_alloc.b = 16;

  t81::tisc::Insn heap_alloc{};
  heap_alloc.opcode = t81::tisc::Opcode::HeapAlloc;
  heap_alloc.a = 5;
  heap_alloc.b = 32;

  t81::tisc::Insn load_value{};
  load_value.opcode = t81::tisc::Opcode::LoadImm;
  load_value.a = 6;
  load_value.b = 123;

  t81::tisc::Insn ax_set{};
  ax_set.opcode = t81::tisc::Opcode::AxSet;
  ax_set.a = 5;  // guard a heap pointer
  ax_set.b = 6;

  t81::tisc::Insn ax_read{};
  ax_read.opcode = t81::tisc::Opcode::AxRead;
  ax_read.a = 7;
  ax_read.b = 4;  // read through the active stack frame

  t81::tisc::Insn heap_free{};
  heap_free.opcode = t81::tisc::Opcode::HeapFree;
  heap_free.a = 5;
  heap_free.b = 32;

  t81::tisc::Insn stack_free{};
  stack_free.opcode = t81::tisc::Opcode::StackFree;
  stack_free.a = 4;
  stack_free.b = 16;

  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;

  std::vector<t81::tisc::Insn> insns = {
      load_tensor0, load_tensor1, vec_add, stack_alloc, heap_alloc, load_value,
      ax_set, heap_free, stack_free, halt,
  };
  ax_read.b = static_cast<int>(insns.size() + 1);
  insns.insert(insns.begin() + 7, ax_read);
  program.insns = std::move(insns);

  program.axion_policy_text =
      "(policy (tier 1)"
      " (require-segment-event (segment stack) (action \"stack frame allocated\"))"
      " (require-segment-event (segment heap) (action \"heap block allocated\"))"
      " (require-segment-event (segment tensor) (action \"tensor slot allocated\"))"
      " (require-segment-event (segment meta) (action \"meta slot axion event\"))"
      " (require-segment-event (segment stack) (action \"AxRead guard\"))"
      " (require-segment-event (segment heap) (action \"AxSet guard\")))";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();

  std::cout << "Axion policy runner emitted the following verdict.reason strings:\n";
  for (const auto& entry : vm->state().axion_log) {
    std::cout << "  opcode=" << static_cast<int>(entry.opcode)
              << " tag=" << entry.tag
              << " reason=\"" << entry.verdict.reason << "\"\n";
  }

  auto contains_reason = [&](std::string_view substring) {
    for (const auto& entry : vm->state().axion_log) {
      if (entry.verdict.reason.find(substring) != std::string::npos) {
        return true;
      }
    }
    return false;
  };

  struct Requirement {
    const char* label;
    std::string_view substring;
  } requirements[] = {
      {"stack trace", "stack frame allocated stack addr="},
      {"heap trace", "heap block allocated heap addr="},
      {"tensor slot trace", "tensor slot allocated tensor addr="},
      {"meta slot trace", "meta slot axion event segment=meta addr="},
      {"AxRead guard", "AxRead guard segment=stack addr="},
      {"AxSet guard", "AxSet guard segment=heap addr="},
  };

  bool all_ok = true;
  for (const auto& req : requirements) {
    if (!contains_reason(req.substring)) {
      std::cerr << "Missing trace entry for " << req.label << "\n";
      all_ok = false;
    }
  }
  if (!result.has_value()) {
    std::cerr << "Axion policy runner trapped with code " << static_cast<int>(result.error()) << '\n';
    auto policy_opt = t81::axion::parse_policy(program.axion_policy_text);
    if (policy_opt.has_value()) {
      auto policy_owned = std::optional<t81::axion::Policy>{policy_opt.value()};
      auto engine = t81::axion::make_policy_engine(std::move(policy_owned));
      t81::axion::SyscallContext ctx;
      ctx.caller = "axion_policy_runner";
      ctx.syscall = "step";
      ctx.pc = vm->state().pc;
      ctx.next_opcode = t81::tisc::Opcode::Halt;
      ctx.trace_reasons.reserve(vm->state().axion_log.size());
      for (const auto& entry : vm->state().axion_log) {
        ctx.trace_reasons.push_back(entry.verdict.reason);
      }
      auto verdict = engine->evaluate(ctx);
      std::cerr << "Policy engine reports: " << verdict.reason << '\n';
    }
    return 1;
  }
  if (!all_ok) {
    return 1;
  }

  std::cout << "Axion segment requirements satisfied per RFC-0020/RFC-0009.\n";
  return 0;
}
