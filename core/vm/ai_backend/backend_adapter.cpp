// core/vm/ai_backend/backend_adapter.cpp
//
// T81 VM backend adapter — RFC-0032 Phase 4 (C-02).
// Promoted from: experiments/ai/llm_backend/backend_adapter.cpp
//
// Violations removed from the experimental file:
//   • LlamaCppBackend  — all llama.cpp dispatch stubs deleted
//   • OnnxRuntimeBackend — all onnx_runtime dispatch stubs deleted
//   • std::this_thread::sleep_for() calls — removed (non-deterministic timing)
//   • std::chrono timing fields in InferenceResult — removed
//   • nlohmann/json dependency — removed
//
// Dispatch path: T81VmBackend builds a synthetic TISC program [<opcode>, Halt],
// attaches an Axion AIHookEngine, and runs through the deterministic T81 VM
// interpreter.  RFC-0032 §8.2 canonical event strings are emitted by the
// AIHookEngine before each opcode executes.

#include "t81/vm/ai_backend/backend_adapter.hpp"

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/vm.hpp"

namespace t81::vm::ai_backend {

// Pack two register indices into the int32 encoding expected by
// decode_ai_packed_reg_pair() in vm.cpp:
//   packed = first | (second << 8)
static constexpr std::int32_t pack_reg(int first, int second) noexcept {
  return static_cast<std::int32_t>(
      (static_cast<std::uint32_t>(first)  & 0xFFu) |
      ((static_cast<std::uint32_t>(second) & 0xFFu) << 8u));
}

// ── Constructor ───────────────────────────────────────────────────────────────

T81VmBackend::T81VmBackend(t81::axion::Policy policy)
    : policy_(std::move(policy)) {}

// ── Internal dispatch ─────────────────────────────────────────────────────────

AiDispatchResult T81VmBackend::run_ai_opcode(
    const t81::tisc::Insn& insn,
    std::int64_t r1, bool r1_tensor,
    std::int64_t r2, bool r2_tensor,
    std::int64_t r3, bool r3_tensor,
    const std::string& /*shape_hint*/) {

  // ── Build synthetic program [<opcode>, Halt] ──────────────────────────────
  t81::tisc::Program prog;
  prog.insns.push_back(insn);
  prog.insns.push_back(t81::tisc::Insn{t81::tisc::Opcode::Halt, 0, 0, 0});

  // ── Attach Axion AIHookEngine ─────────────────────────────────────────────
  // AIHookEngine wraps a PolicyEngine that enforces policy_ (RFC-0032 §8.2).
  // The VM calls engine.evaluate() before every instruction, so the hook
  // fires before the AI opcode executes.
  auto inner  = std::make_unique<t81::axion::PolicyEngine>(policy_);
  auto hook   = std::make_unique<t81::axion::AIHookEngine>(std::move(inner));
  auto vm     = t81::vm::make_interpreter_vm(std::move(hook));

  // ── Load program and set operand registers ────────────────────────────────
  vm->load_program(prog);

  const auto tensor_tag = ValueTag::TensorHandle;
  const auto int_tag    = ValueTag::Int;
  vm->set_register(1, r1, r1_tensor ? tensor_tag : int_tag);
  vm->set_register(2, r2, r2_tensor ? tensor_tag : int_tag);
  vm->set_register(3, r3, r3_tensor ? tensor_tag : int_tag);

  // ── Execute ───────────────────────────────────────────────────────────────
  auto run_result = vm->run_to_halt();

  AiDispatchResult result;
  if (run_result.has_value()) {
    // Ran to Halt without trap — read result from R0.
    result.ok            = true;
    result.output_handle = vm->state().contexts[0].registers[0];
  } else {
    result.ok   = false;
    result.trap = run_result.error();

    if (result.trap == Trap::SecurityFault) {
      // Extract the Axion deny reason from the last axion_log entry.
      const auto& log = vm->state().axion_log;
      if (!log.empty()) {
        result.deny_reason = log.back().verdict.reason;
      }
    } else {
      result.deny_reason = to_string(result.trap);
    }
  }
  return result;
}

// ── Public dispatch API ───────────────────────────────────────────────────────

AiDispatchResult T81VmBackend::dispatch_attn(
    std::int64_t q_handle, std::int64_t k_handle, std::int64_t v_handle,
    const std::string& shape_hint) {
  // ATTN  a=RD(0), b=R_Q(1), c=pack(R_K=2, R_V=3)
  t81::tisc::Insn insn;
  insn.opcode = t81::tisc::Opcode::ATTN;
  insn.a      = 0;
  insn.b      = 1;
  insn.c      = pack_reg(2, 3);
  return run_ai_opcode(insn,
      q_handle, /*tensor=*/true,
      k_handle, /*tensor=*/true,
      v_handle, /*tensor=*/true,
      shape_hint);
}

AiDispatchResult T81VmBackend::dispatch_qmatmul(
    std::int64_t act_handle, std::int64_t wt_handle, std::int64_t scale_handle,
    const std::string& /*wt_hash*/) {
  // QMATMUL  a=RD(0), b=R_ACT(1), c=pack(R_WT=2, R_SCALE=3)
  t81::tisc::Insn insn;
  insn.opcode = t81::tisc::Opcode::QMATMUL;
  insn.a      = 0;
  insn.b      = 1;
  insn.c      = pack_reg(2, 3);
  return run_ai_opcode(insn,
      act_handle,   /*tensor=*/true,
      wt_handle,    /*tensor=*/true,
      scale_handle, /*tensor=*/false,
      "");
}

AiDispatchResult T81VmBackend::dispatch_embed(
    std::int64_t table_handle, std::int64_t index_handle) {
  // EMBED  a=RD(0), b=R_TABLE(1), c=R_IDX(2)  (not packed — three separate regs)
  t81::tisc::Insn insn;
  insn.opcode = t81::tisc::Opcode::EMBED;
  insn.a      = 0;
  insn.b      = 1;
  insn.c      = 2;
  return run_ai_opcode(insn,
      table_handle, /*tensor=*/true,
      index_handle, /*tensor=*/false,
      0,            /*r3 unused*/false,
      "");
}

AiDispatchResult T81VmBackend::dispatch_wload(
    std::int64_t src_handle, std::int64_t policy_handle) {
  // WLOAD  a=RD(0), b=R_SRC(1), c=R_POLICY(2)
  t81::tisc::Insn insn;
  insn.opcode = t81::tisc::Opcode::WLOAD;
  insn.a      = 0;
  insn.b      = 1;
  insn.c      = 2;
  return run_ai_opcode(insn,
      src_handle,    /*tensor=*/true,
      policy_handle, /*tensor=*/false,
      0,             /*r3 unused*/false,
      "");
}

}  // namespace t81::vm::ai_backend
