// include/t81/vm/ai_backend/backend_adapter.hpp
//
// T81 VM backend adapter — RFC-0032 Phase 4 (C-02).
//
// Dispatches AI-native inference opcodes (ATTN, QMATMUL, EMBED, WLOAD)
// through the deterministic T81 VM interpreter.  An Axion AIHookEngine is
// attached as the governance engine, emitting pre-execution RFC-0032 §8.2
// canonical event strings before each opcode.
//
//
// MUST NOT include <chrono>, <thread>, llama.cpp, or onnx_runtime headers.
// All inference results are integer/ternary-typed (no floating-point timing).

#pragma once

#include "t81/axion/policy.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/traps.hpp"

#include <cstdint>
#include <string>

namespace t81::vm::ai_backend {

// Result of a single AI opcode dispatch through the T81 VM.
// No timing fields — determinism requires integer/ternary-typed outputs only.
struct AiDispatchResult {
  bool ok{false};
  std::int64_t output_handle{0};  // R0 (dest register) on success; 0 on trap
  Trap trap{Trap::None};          // populated when !ok
  std::string deny_reason;        // Axion reason string when trap == SecurityFault
};

// T81VmBackend — dispatches RFC-0026 AI-native opcodes through the
// deterministic T81 interpreter with an Axion AIHookEngine attached for
// pre-execution governance (RFC-0032 §8.2).
//
// Each dispatch call creates an isolated two-instruction synthetic program
//   [<AI opcode>, Halt]
// loads it into a fresh VM instance with AIHookEngine as the engine, sets
// the operand registers with the caller-supplied handles, and runs to
// completion.  The AIHookEngine fires before opcode execution:
//
//   • ATTN    — tier-gated (requires VM tier >= 2); AIHookEngine emits
//               attn_guard + ai_exec_gate; denies with SecurityFault if tier < 2.
//   • QMATMUL — no tier gate; AIHookEngine emits qmatmul_guard + ai_exec_gate.
//   • EMBED   — no tier gate; AIHookEngine emits ai_exec_gate.
//   • WLOAD   — no tier gate; AIHookEngine emits ai_exec_gate.
//
// NOTE: The VM starts at cognitive TierId::Tier0 by default.  For ATTN to
// proceed past the Axion gate the calling TISC program must first promote
// tier to >= 2 (e.g. via ReflCap / ReflJustify sequences).  A raw
// dispatch_attn() call with default tier returns SecurityFault — this is
// correct and expected; it proves the Axion gate is active.
class T81VmBackend {
 public:
  explicit T81VmBackend(t81::axion::Policy policy = {});

  // ATTN  RD=R0, Q=R1, pack(K=R2, V=R3) — scaled dot-product attention.
  // Tier-gated: VM must be at TierId::Tier2 or higher.
  AiDispatchResult dispatch_attn(std::int64_t q_handle,
                                 std::int64_t k_handle,
                                 std::int64_t v_handle,
                                 const std::string& shape_hint = "");

  // QMATMUL  RD=R0, ACT=R1, pack(WT=R2, SCALE=R3) — quantised matmul.
  AiDispatchResult dispatch_qmatmul(std::int64_t act_handle,
                                    std::int64_t wt_handle,
                                    std::int64_t scale_handle,
                                    const std::string& wt_hash = "");

  // EMBED  RD=R0, TABLE=R1, IDX=R2 — embedding table lookup.
  AiDispatchResult dispatch_embed(std::int64_t table_handle,
                                  std::int64_t index_handle);

  // WLOAD  RD=R0, SRC=R1, POLICY=R2 — weight materialisation with CanonFS.
  AiDispatchResult dispatch_wload(std::int64_t src_handle,
                                  std::int64_t policy_handle);

 private:
  // Build and run a [insn, Halt] program with the given operand registers.
  // r1/r2/r3 are placed in R1/R2/R3; r1_tensor etc. controls ValueTag.
  AiDispatchResult run_ai_opcode(const t81::tisc::Insn& insn,
                                 std::int64_t r1, bool r1_tensor,
                                 std::int64_t r2, bool r2_tensor,
                                 std::int64_t r3, bool r3_tensor,
                                 const std::string& shape_hint);

  t81::axion::Policy policy_;
};

}  // namespace t81::vm::ai_backend
