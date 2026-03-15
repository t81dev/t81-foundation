// kernel/axion/ai_hooks.cpp
//
// AI event hook engine — RFC-0032 Phase 3 (C-04).
// Promoted from experiments/ai/policy_hooks/axion_hooks.cpp.
//
// Implements AIHookEngine: emits canonical Axion AI event trace strings
// (RFC-0032 §8.2) and enforces AI-specific pre-execution guards.

#include "t81/axion/ai_hooks.hpp"

#include "t81/axion/verdict.hpp"
#include "t81/axion/context.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/ai_native_opcodes.hpp"

#include <sstream>

namespace t81::axion {

// ── AIHookEngine ──────────────────────────────────────────────────────────────

AIHookEngine::AIHookEngine(std::unique_ptr<Engine> inner)
    : inner_(std::move(inner)) {}

Verdict AIHookEngine::evaluate(const SyscallContext& ctx) {
  using Opcode = t81::tisc::Opcode;

  // ── TLoadHash / model_load event ────────────────────────────────────────
  if (ctx.next_opcode == Opcode::TLoadHash) {
    // The inner engine (PolicyEngine) performs the actual whitelist check.
    // We emit the trace event here, resolving allow/deny by peeking at the
    // inner engine's verdict before returning.
    Verdict inner = inner_->evaluate(ctx);
    const bool allowed = (inner.kind == VerdictKind::Allow);
    push(ai_reasons::model_load_event(allowed, ctx.payload,
                                       allowed ? "allow" : inner.reason));
    return inner;
  }

  // ── AI-native inference opcodes ─────────────────────────────────────────
  if (!t81::isa::is_phase1_ai_opcode(ctx.next_opcode)) {
    // Non-AI opcode: delegate directly.
    return inner_->evaluate(ctx);
  }

  // Emit ai_exec_gate BEFORE the per-opcode guard (RFC-0032 §8.2 ordering:
  // events MUST be emitted before the associated operation executes).
  // We determine allow/deny after all guards pass, so we defer final emission
  // but run guards first.

  // ── Per-opcode guards ────────────────────────────────────────────────────
  switch (ctx.next_opcode) {
    case Opcode::ATTN: {
      // attn_guard: require tier ≥ 2 (RFC-0032 §8.1).
      // Shape is opaque at this layer; emit a placeholder unless ctx.payload
      // carries a shape descriptor from the VM.
      const std::string shape = ctx.payload.empty() ? "?" : ctx.payload;
      push(ai_reasons::attn_guard_event(shape, ctx.current_tier));

      if (ctx.current_tier < 2) {
        push(ai_reasons::ai_exec_gate_event(false));
        return Verdict{VerdictKind::Deny,
                       "attn_guard: tier < 2 (current_tier=" +
                           std::to_string(ctx.current_tier) + ")"};
      }
      push(ai_reasons::ai_exec_gate_event(true));
      break;
    }

    case Opcode::QMATMUL: {
      // qmatmul_guard: emit guard event; inner engine validates tensor hash.
      // scale and wt_hash are carried in ctx.payload as "scale=N wt_hash=H"
      // or left empty if not yet wired by the VM (Phase 4).
      int32_t scale = 0;
      std::string wt_hash;
      if (!ctx.payload.empty()) {
        // Best-effort parse of "scale=<N> wt_hash=<H>".
        std::istringstream iss(ctx.payload);
        std::string token;
        while (iss >> token) {
          if (token.substr(0, 6) == "scale=") scale = std::stoi(token.substr(6));
          if (token.substr(0, 8) == "wt_hash=") wt_hash = token.substr(8);
        }
      }
      push(ai_reasons::qmatmul_guard_event(true, scale, wt_hash.empty() ? "?" : wt_hash));
      push(ai_reasons::ai_exec_gate_event(true));
      break;
    }

    case Opcode::EMBED:
    case Opcode::WLOAD:
    default:
      // No additional guards in Phase 3 for EMBED/WLOAD; emit exec gate.
      push(ai_reasons::ai_exec_gate_event(true));
      break;
  }

  // Delegate to inner engine for policy evaluation.
  return inner_->evaluate(ctx);
}

// ── Factory ───────────────────────────────────────────────────────────────────

std::unique_ptr<Engine> make_ai_hook_engine(std::unique_ptr<Engine> inner) {
  return std::make_unique<AIHookEngine>(std::move(inner));
}

}  // namespace t81::axion
