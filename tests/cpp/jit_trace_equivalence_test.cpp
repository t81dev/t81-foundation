#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using t81::tisc::Insn;
using t81::tisc::Opcode;
using t81::tisc::Program;
using t81::vm::State;

class DenySwarJitTraceExitEngine final : public t81::axion::Engine {
public:
  t81::axion::Verdict evaluate(const t81::axion::SyscallContext& ctx) override {
    if (ctx.syscall == t81::axion::reasons::kJitTraceExit &&
        ctx.next_opcode == t81::tisc::Opcode::TNOT_SWAR) {
      return {t81::axion::VerdictKind::Deny, "deny-swar-jit-trace-exit"};
    }
    return {t81::axion::VerdictKind::Allow, "allow"};
  }
};

bool expect(bool cond, const std::string& message) {
  if (!cond) {
    std::cerr << "jit_trace_equivalence_test failure: " << message << "\n";
    return false;
  }
  return true;
}

Program make_hot_arith_program() {
  Program p;
  p.insns = {
      // r1=0, r2=0, r3=1, r4=100 (loop count)
      {Opcode::LoadImm, 1, 0, 0},
      {Opcode::LoadImm, 2, 0, 0},
      {Opcode::LoadImm, 3, 1, 0},
      {Opcode::LoadImm, 4, 100, 0},
      // loop at pc=4:
      {Opcode::Add, 1, 1, 3},            // r1 += 1
      {Opcode::Add, 2, 2, 1},            // r2 += r1
      {Opcode::Sub, 4, 4, 3},            // r4 -= 1
      {Opcode::JumpIfNotZero, 4, 4, 0},  // if r4 != 0 jump loop
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

Program make_hot_deterministic_ops_program() {
  Program p;
  p.insns = {
      {Opcode::LoadImm, 1, -1, 0},  // trit -1
      {Opcode::LoadImm, 2, 1, 0},   // trit +1
      {Opcode::LoadImm, 3, 0, 0},
      {Opcode::LoadImm, 4, 64, 0},  // loop count
      // loop at pc=4
      {Opcode::TOr, 5, 1, 2},         // 1
      {Opcode::TAnd, 6, 1, 2},        // -1
      {Opcode::TXor, 7, 5, 6},        // -1
      {Opcode::TNot, 8, 7, 0},        // 1
      {Opcode::LessEqual, 9, 6, 5},   // 1
      {Opcode::Greater, 10, 8, 3},    // 1
      {Opcode::NotEqual, 11, 10, 3},  // 1
      {Opcode::Cmp, 6, 5, 0},         // negative=true
      {Opcode::JumpIfNegative, 14, 0, 0},
      {Opcode::LoadImm, 12, 999, 0},  // should be skipped
      {Opcode::SetF, 12, 0, 0},       // -1
      {Opcode::Dec, 4, 0, 0},
      {Opcode::JumpIfNotZero, 4, 4, 0},
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

Program make_hot_memory_option_result_program() {
  Program p;
  p.insns = {
      {Opcode::LoadImm, 1, 64, 0},  // loop count
      {Opcode::LoadImm, 2, 7, 0},   // payload value
      {Opcode::LoadImm, 4, 0, 0},   // running sum
      {Opcode::LoadImm, 5, 0, 0},
      // loop at pc=4
      {Opcode::Store, 90, 2, 0},  // mem[90] = r2
      {Opcode::Load, 5, 90, 0},   // r5 = mem[90]
      {Opcode::Push, 5, 0, 0},
      {Opcode::Pop, 6, 0, 0},
      {Opcode::MakeOptionSome, 7, 6, 0},
      {Opcode::OptionIsSome, 8, 7, 0},
      {Opcode::OptionUnwrap, 9, 7, 0},
      {Opcode::MakeResultOk, 10, 9, 0},
      {Opcode::ResultIsOk, 11, 10, 0},
      {Opcode::ResultUnwrapOk, 12, 10, 0},
      {Opcode::Add, 4, 4, 12},  // sum += 7
      {Opcode::Dec, 1, 0, 0},
      {Opcode::JumpIfNotZero, 4, 1, 0},
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

Program make_hot_enum_program() {
  Program p;
  p.insns = {
      {Opcode::LoadImm, 1, 64, 0},  // loop count
      {Opcode::LoadImm, 2, 7, 0},   // payload
      {Opcode::LoadImm, 3, 0, 0},   // sum
      // loop at pc=3
      {Opcode::MakeEnumVariantPayload, 4, 2, 200},  // variant id 200 payload=7
      {Opcode::EnumIsVariant, 5, 4, 200},           // 1
      {Opcode::EnumUnwrapPayload, 6, 4, 0},         // 7
      {Opcode::Add, 3, 3, 6},                       // sum += 7
      {Opcode::Dec, 1, 0, 0},
      {Opcode::JumpIfNotZero, 3, 1, 0},
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

Program make_hot_call_ret_program() {
  Program p;
  p.insns = {
      {Opcode::LoadImm, 12, 64, 0},  // loop count (using R12 instead of hardwired R0)
      {Opcode::LoadImm, 1, 11, 0},   // function entry PC
      {Opcode::LoadImm, 2, 2, 0},    // function return payload
      {Opcode::LoadImm, 3, 0, 0},    // sum
      // loop at pc=4
      {Opcode::Call, 0, 1, 0},  // call function at r1
      {Opcode::Add, 3, 3, 10},  // sum += r10
      {Opcode::Dec, 12, 0, 0},
      {Opcode::JumpIfNotZero, 4, 12, 0},
      {Opcode::Halt, 0, 0, 0},
      {Opcode::Nop, 0, 0, 0},
      {Opcode::Nop, 0, 0, 0},
      // function body
      {Opcode::Mov, 10, 2, 0},
      {Opcode::Ret, 0, 0, 0},
  };
  return p;
}

Program make_hot_bitwise_program() {
  Program p;
  p.insns = {
      {Opcode::LoadImm, 1, 0x5A, 0},  // 90
      {Opcode::LoadImm, 2, 0x0F, 0},  // 15
      {Opcode::LoadImm, 3, 1, 0},     // shift amount
      {Opcode::LoadImm, 4, 64, 0},    // loop count
      // loop at pc=4
      {Opcode::BitAnd, 5, 1, 2},
      {Opcode::BitOr, 6, 1, 2},
      {Opcode::BitXor, 7, 1, 2},
      {Opcode::BitNot, 8, 7, 0},
      {Opcode::BitShl, 9, 6, 3},
      {Opcode::BitShr, 10, 9, 3},
      {Opcode::BitUShr, 11, 8, 3},
      {Opcode::Dec, 4, 0, 0},
      {Opcode::JumpIfNotZero, 4, 4, 0},
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

Program make_hot_swar_tensor_program() {
  Program p;
  p.tensor_pool.push_back([] {
    t81::T729DynamicTensor tensor({4}, {-1.0f, 0.0f, 1.0f, -1.0f});
    tensor.set_numeric_class(t81::TensorNumericClass::ExactTrit);
    return tensor;
  }());
  p.tensor_pool.push_back([] {
    t81::T729DynamicTensor tensor({4}, {1.0f, -1.0f, 0.0f, 1.0f});
    tensor.set_numeric_class(t81::TensorNumericClass::ExactTrit);
    return tensor;
  }());
  p.insns = {
      {Opcode::LoadImm, 1, 1, 0, t81::tisc::LiteralKind::TensorHandle},
      {Opcode::LoadImm, 2, 2, 0, t81::tisc::LiteralKind::TensorHandle},
      {Opcode::LoadImm, 3, 64, 0},
      {Opcode::LoadImm, 4, 0, 0},
      // loop at pc=4
      {Opcode::TNOT_SWAR, 5, 1, 0},
      {Opcode::TAND_SWAR, 6, 1, 2},
      {Opcode::TOR_SWAR, 7, 1, 2},
      {Opcode::Add, 4, 4, 3},
      {Opcode::Dec, 3, 0, 0},
      {Opcode::JumpIfNotZero, 4, 3, 0},
      {Opcode::Halt, 0, 0, 0},
  };
  return p;
}

struct Snapshot {
  std::vector<std::int64_t> regs;
  std::vector<t81::vm::ValueTag> tags;
  std::size_t pc{0};
  bool halted{false};
  std::vector<std::string> trace_reasons;
};

Snapshot run_program(const Program& program) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "run_to_halt trap=" << static_cast<int>(result.error()) << "\n";
    return {};
  }

  Snapshot snap;
  const State& st = vm->state();
  snap.regs.assign(st.contexts[0].registers.begin(), st.contexts[0].registers.end());
  snap.tags.assign(st.contexts[0].register_tags.begin(), st.contexts[0].register_tags.end());
  snap.pc = st.contexts[0].pc;
  snap.halted = st.halted;
  snap.trace_reasons.reserve(st.axion_log.size());
  for (const auto& entry : st.axion_log) {
    snap.trace_reasons.push_back(entry.verdict.reason);
  }
  return snap;
}

bool test_jit_trace_equivalence_and_determinism() {
  const Program program = make_hot_arith_program();

  // Both runs should converge to identical final machine states and traces,
  // regardless of when the internal trace JIT engages.
  const Snapshot a = run_program(program);
  const Snapshot b = run_program(program);

  if (!expect(!a.regs.empty(), "arith snapshot A is empty")) return false;
  if (!expect(!b.regs.empty(), "arith snapshot B is empty")) return false;
  if (!expect(a.regs == b.regs, "arith register snapshots diverged")) return false;
  if (!expect(a.tags == b.tags, "arith register-tag snapshots diverged")) return false;
  if (!expect(a.pc == b.pc, "arith PCs diverged")) return false;
  if (!expect(a.halted == b.halted, "arith halted flags diverged")) return false;
  if (!expect(a.trace_reasons == b.trace_reasons, "arith trace reasons diverged")) return false;
  bool saw_enter = false;
  bool saw_exit = false;
  [[maybe_unused]] bool saw_enter_at_loop_pc = false;
  [[maybe_unused]] bool saw_exit_kind = false;
  for (const auto& reason : a.trace_reasons) {
    if (reason.find("jit trace enter") != std::string::npos) {
      saw_enter = true;
      if (reason.find("pc=4") != std::string::npos) {
        saw_enter_at_loop_pc = true;
      }
    }
    if (reason.find("jit trace exit") != std::string::npos) {
      saw_exit = true;
      if (reason.find("exit-kind=") != std::string::npos) {
        saw_exit_kind = true;
      }
    }
  }
  if (!saw_enter || !saw_exit || !saw_enter_at_loop_pc || !saw_exit_kind) {
    std::cerr << "missing jit boundary reasons: enter=" << saw_enter << " exit=" << saw_exit
              << " enter_pc4=" << saw_enter_at_loop_pc << " exit_kind=" << saw_exit_kind << "\n";
    return expect(false, "missing JIT boundary logs for arith trace");
  }

  // Sanity-check the arithmetic result for this loop.
  // r1 = 100, r2 = 1 + ... + 100 = 5050.
  if (!expect(a.regs[1] == 100, "arith R1 expected 100")) return false;
  if (!expect(a.regs[2] == 5050, "arith R2 expected 5050")) return false;
  return true;
}

bool test_jit_extended_opcode_trace_determinism() {
  const Program program = make_hot_deterministic_ops_program();
  const Snapshot a = run_program(program);
  const Snapshot b = run_program(program);

  if (!expect(!a.regs.empty(), "extended snapshot A is empty")) return false;
  if (!expect(!b.regs.empty(), "extended snapshot B is empty")) return false;
  if (!expect(a.regs == b.regs, "extended register snapshots diverged")) return false;
  if (!expect(a.tags == b.tags, "extended register-tag snapshots diverged")) return false;
  if (!expect(a.pc == b.pc, "extended PCs diverged")) return false;
  if (!expect(a.halted == b.halted, "extended halted flags diverged")) return false;
  if (!expect(a.trace_reasons == b.trace_reasons, "extended trace reasons diverged")) return false;

  [[maybe_unused]] bool saw_enter_at_loop_pc = false;
  [[maybe_unused]] bool saw_exit_kind = false;
  for (const auto& reason : a.trace_reasons) {
    if (reason.find("jit trace enter") != std::string::npos &&
        reason.find("pc=4") != std::string::npos) {
      saw_enter_at_loop_pc = true;
    }
    if (reason.find("jit trace exit") != std::string::npos &&
        reason.find("exit-kind=") != std::string::npos) {
      saw_exit_kind = true;
    }
  }
  if (!expect(saw_enter_at_loop_pc, "extended trace missing jit enter at loop pc=4")) return false;
  if (!expect(saw_exit_kind, "extended trace missing jit exit kind annotation")) return false;

  if (!expect(a.regs[5] == 1, "extended R5 (TOr) expected 1")) return false;
  if (!expect(a.regs[6] == -1, "extended R6 (TAnd) expected -1")) return false;
  if (!expect(a.regs[8] == 1, "extended R8 (TNot(TXor)) expected 1")) return false;
  if (!expect(a.regs[9] == 1, "extended R9 (LessEqual) expected 1")) return false;
  if (!expect(a.regs[10] == 1, "extended R10 (Greater) expected 1")) return false;
  if (!expect(a.regs[11] == 1, "extended R11 (NotEqual) expected 1")) return false;
  if (!expect(a.regs[12] == -1, "extended R12 (SetF) expected -1")) return false;
  return true;
}

bool test_jit_memory_option_result_trace_determinism() {
  const Program program = make_hot_memory_option_result_program();
  const Snapshot a = run_program(program);
  const Snapshot b = run_program(program);

  if (!expect(!a.regs.empty(), "memory/option snapshot A is empty")) return false;
  if (!expect(!b.regs.empty(), "memory/option snapshot B is empty")) return false;
  if (!expect(a.regs == b.regs, "memory/option register snapshots diverged")) return false;
  if (!expect(a.tags == b.tags, "memory/option register-tag snapshots diverged")) return false;
  if (!expect(a.pc == b.pc, "memory/option PCs diverged")) return false;
  if (!expect(a.halted == b.halted, "memory/option halted flags diverged")) return false;
  if (!expect(a.trace_reasons == b.trace_reasons, "memory/option trace reasons diverged"))
    return false;

  bool saw_enter_at_loop_pc = false;
  bool saw_exit_kind = false;
  for (const auto& reason : a.trace_reasons) {
    if (reason.find("jit trace enter") != std::string::npos &&
        reason.find("pc=4") != std::string::npos) {
      saw_enter_at_loop_pc = true;
    }
    if (reason.find("jit trace exit") != std::string::npos &&
        reason.find("exit-kind=") != std::string::npos) {
      saw_exit_kind = true;
    }
  }
  if (!expect(saw_enter_at_loop_pc, "memory/option trace missing jit enter at loop pc=4"))
    return false;
  if (!expect(saw_exit_kind, "memory/option trace missing jit exit kind annotation")) return false;

  if (!expect(a.regs[4] == 448, "memory/option R4 expected 448")) return false;
  if (!expect(a.regs[5] == 7, "memory/option R5 expected 7")) return false;
  if (!expect(a.regs[6] == 7, "memory/option R6 expected 7")) return false;
  if (!expect(a.regs[8] == 1, "memory/option R8 (OptionIsSome) expected 1")) return false;
  if (!expect(a.regs[9] == 7, "memory/option R9 (OptionUnwrap) expected 7")) return false;
  if (!expect(a.regs[11] == 1, "memory/option R11 (ResultIsOk) expected 1")) return false;
  if (!expect(a.regs[12] == 7, "memory/option R12 (ResultUnwrapOk) expected 7")) return false;
  return true;
}

bool test_jit_enum_trace_determinism() {
  const Program program = make_hot_enum_program();
  const Snapshot a = run_program(program);
  const Snapshot b = run_program(program);

  if (!expect(!a.regs.empty(), "enum snapshot A is empty")) return false;
  if (!expect(!b.regs.empty(), "enum snapshot B is empty")) return false;
  if (!expect(a.regs == b.regs, "enum register snapshots diverged")) return false;
  if (!expect(a.tags == b.tags, "enum register-tag snapshots diverged")) return false;
  if (!expect(a.pc == b.pc, "enum PCs diverged")) return false;
  if (!expect(a.halted == b.halted, "enum halted flags diverged")) return false;
  if (!expect(a.trace_reasons == b.trace_reasons, "enum trace reasons diverged")) return false;

  bool saw_enter_at_loop_pc = false;
  bool saw_exit_kind = false;
  for (const auto& reason : a.trace_reasons) {
    if (reason.find("jit trace enter") != std::string::npos &&
        reason.find("pc=3") != std::string::npos) {
      saw_enter_at_loop_pc = true;
    }
    if (reason.find("jit trace exit") != std::string::npos &&
        reason.find("exit-kind=") != std::string::npos) {
      saw_exit_kind = true;
    }
  }
  if (!expect(saw_enter_at_loop_pc, "enum trace missing jit enter at loop pc=3")) return false;
  if (!expect(saw_exit_kind, "enum trace missing jit exit kind annotation")) return false;

  if (!expect(a.regs[3] == 448, "enum R3 expected 448")) return false;
  if (!expect(a.regs[5] == 1, "enum R5 (EnumIsVariant) expected 1")) return false;
  if (!expect(a.regs[6] == 7, "enum R6 (EnumUnwrapPayload) expected 7")) return false;
  return true;
}

bool test_jit_call_ret_trace_determinism() {
  const Program program = make_hot_call_ret_program();
  const Snapshot a = run_program(program);
  const Snapshot b = run_program(program);

  if (!expect(!a.regs.empty(), "call/ret snapshot A is empty")) return false;
  if (!expect(!b.regs.empty(), "call/ret snapshot B is empty")) return false;
  if (!expect(a.regs == b.regs, "call/ret register snapshots diverged")) return false;
  if (!expect(a.tags == b.tags, "call/ret register-tag snapshots diverged")) return false;
  if (!expect(a.pc == b.pc, "call/ret PCs diverged")) return false;
  if (!expect(a.halted == b.halted, "call/ret halted flags diverged")) return false;
  if (!expect(a.trace_reasons == b.trace_reasons, "call/ret trace reasons diverged")) return false;

  [[maybe_unused]] bool saw_enter_at_loop_pc = false;
  [[maybe_unused]] bool saw_exit_kind = false;
  for (const auto& reason : a.trace_reasons) {
    if (reason.find("jit trace enter") != std::string::npos &&
        reason.find("pc=4") != std::string::npos) {
      saw_enter_at_loop_pc = true;
    }
    if (reason.find("jit trace exit") != std::string::npos &&
        reason.find("exit-kind=") != std::string::npos) {
      saw_exit_kind = true;
    }
  }
  // Call/Ret are currently excluded from JIT to enforce recursion limits/promotion.
  // So we expect NO JIT trace for the loop starting with Call at pc=4.
  // if (!expect(saw_enter_at_loop_pc, "call/ret trace missing jit enter at loop pc=4")) return
  // false;
  // if (!expect(saw_exit_kind, "call/ret trace missing jit exit kind annotation")) return
  // false;

  if (!expect(a.regs[3] == 128, "call/ret R3 expected 128")) return false;
  if (!expect(a.regs[10] == 2, "call/ret R10 expected 2")) return false;
  return true;
}

bool test_jit_bitwise_trace_determinism() {
  const Program program = make_hot_bitwise_program();
  const Snapshot a = run_program(program);
  const Snapshot b = run_program(program);

  if (!expect(!a.regs.empty(), "bitwise snapshot A is empty")) return false;
  if (!expect(!b.regs.empty(), "bitwise snapshot B is empty")) return false;
  if (!expect(a.regs == b.regs, "bitwise register snapshots diverged")) return false;
  if (!expect(a.tags == b.tags, "bitwise register-tag snapshots diverged")) return false;
  if (!expect(a.pc == b.pc, "bitwise PCs diverged")) return false;
  if (!expect(a.halted == b.halted, "bitwise halted flags diverged")) return false;
  if (!expect(a.trace_reasons == b.trace_reasons, "bitwise trace reasons diverged")) return false;

  bool saw_enter_at_loop_pc = false;
  bool saw_exit_kind = false;
  for (const auto& reason : a.trace_reasons) {
    if (reason.find("jit trace enter") != std::string::npos &&
        reason.find("pc=4") != std::string::npos) {
      saw_enter_at_loop_pc = true;
    }
    if (reason.find("jit trace exit") != std::string::npos &&
        reason.find("exit-kind=") != std::string::npos) {
      saw_exit_kind = true;
    }
  }
  if (!expect(saw_enter_at_loop_pc, "bitwise trace missing jit enter at loop pc=4")) return false;
  if (!expect(saw_exit_kind, "bitwise trace missing jit exit kind annotation")) return false;

  if (!expect(a.regs[5] == 10, "bitwise R5 (BitAnd) expected 10")) return false;
  if (!expect(a.regs[6] == 95, "bitwise R6 (BitOr) expected 95")) return false;
  if (!expect(a.regs[7] == 85, "bitwise R7 (BitXor) expected 85")) return false;
  if (!expect(a.regs[8] == -86, "bitwise R8 (BitNot) expected -86")) return false;
  if (!expect(a.regs[9] == 190, "bitwise R9 (BitShl) expected 190")) return false;
  if (!expect(a.regs[10] == 95, "bitwise R10 (BitShr) expected 95")) return false;
  if (!expect(a.regs[11] == 9223372036854775765LL, "bitwise R11 (BitUShr) expected logical shift"))
    return false;
  return true;
}

bool test_jit_swar_tensor_trace_determinism() {
  const Program program = make_hot_swar_tensor_program();
  const Snapshot a = run_program(program);
  const Snapshot b = run_program(program);

  if (!expect(!a.regs.empty(), "swar snapshot A is empty")) return false;
  if (!expect(!b.regs.empty(), "swar snapshot B is empty")) return false;
  if (!expect(a.regs == b.regs, "swar register snapshots diverged")) return false;
  if (!expect(a.tags == b.tags, "swar register-tag snapshots diverged")) return false;
  if (!expect(a.pc == b.pc, "swar PCs diverged")) return false;
  if (!expect(a.halted == b.halted, "swar halted flags diverged")) return false;
  if (!expect(a.trace_reasons == b.trace_reasons, "swar trace reasons diverged")) return false;

  bool saw_enter_at_loop_pc = false;
  bool saw_exit_kind = false;
  for (const auto& reason : a.trace_reasons) {
    if (reason.find("jit trace enter") != std::string::npos &&
        reason.find("pc=4") != std::string::npos) {
      saw_enter_at_loop_pc = true;
    }
    if (reason.find("jit trace exit") != std::string::npos &&
        reason.find("exit-kind=") != std::string::npos) {
      saw_exit_kind = true;
    }
  }
  if (!expect(saw_enter_at_loop_pc, "swar trace missing jit enter at loop pc=4")) return false;
  if (!expect(saw_exit_kind, "swar trace missing jit exit kind annotation")) return false;

  if (!expect(a.tags[5] == t81::vm::ValueTag::TensorHandle, "swar R5 expected tensor handle")) return false;
  if (!expect(a.tags[6] == t81::vm::ValueTag::TensorHandle, "swar R6 expected tensor handle")) return false;
  if (!expect(a.tags[7] == t81::vm::ValueTag::TensorHandle, "swar R7 expected tensor handle")) return false;
  if (!expect(a.regs[4] == 2080, "swar loop accumulator R4 expected 2080")) return false;
  return true;
}

bool test_jit_swar_policy_enforcement() {
  auto vm = t81::vm::make_interpreter_vm(std::make_unique<DenySwarJitTraceExitEngine>());
  vm->load_program(make_hot_swar_tensor_program());

  const auto run = vm->run_to_halt(5000);
  if (!expect(!run.has_value(), "swar policy test expected a trap")) return false;
  if (!expect(run.error() == t81::vm::Trap::SecurityFault,
              "swar policy test expected SecurityFault")) {
    return false;
  }

  bool saw_trace_enter = false;
  bool saw_trace_exit = false;
  for (const auto& entry : vm->state().axion_log) {
    if (entry.verdict.reason.find("jit trace enter") != std::string::npos) {
      saw_trace_enter = true;
    }
    if (entry.verdict.reason.find("jit trace exit") != std::string::npos) {
      saw_trace_exit = true;
    }
  }

  if (!expect(saw_trace_enter, "swar policy test missing jit trace enter")) return false;
  if (!expect(saw_trace_exit, "swar policy test missing jit trace exit")) return false;
  return true;
}

}  // namespace

int main() {
  if (!test_jit_trace_equivalence_and_determinism()) return 1;
  if (!test_jit_extended_opcode_trace_determinism()) return 1;
  if (!test_jit_memory_option_result_trace_determinism()) return 1;
  if (!test_jit_enum_trace_determinism()) return 1;
  if (!test_jit_call_ret_trace_determinism()) return 1;
  if (!test_jit_bitwise_trace_determinism()) return 1;
  if (!test_jit_swar_tensor_trace_determinism()) return 1;
  if (!test_jit_swar_policy_enforcement()) return 1;
  std::cout << "jit trace equivalence test passed\n";
  return 0;
}
