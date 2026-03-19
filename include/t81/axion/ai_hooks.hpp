// include/t81/axion/ai_hooks.hpp
//
// AI event hook engine — RFC-0032 Phase 3 (C-04).
//
// Provides AIHookEngine: an Engine wrapper that emits canonical Axion AI event
// trace strings (RFC-0032 §8.2) and enforces AI-specific pre-execution guards
// before delegating to the inner policy engine.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "t81/axion/engine.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/axion/context.hpp"

namespace t81::axion {

// ── Canonical AI event trace strings (RFC-0032 §8.2) ─────────────────────────
//
// Build the normative verbatim event strings emitted before each AI operation.
// Tests MUST match these exact formats to satisfy the Phase 3 gate criteria.

namespace ai_reasons {

/// "model_load success|failure hash=<hash> reason=<reason>"
inline std::string model_load_event(bool success,
                                    std::string_view hash,
                                    std::string_view reason) {
  std::string s;
  s.reserve(128);
  s += "model_load ";
  s += success ? "success" : "failure";
  s += " hash=";
  s += hash;
  s += " reason=";
  s += reason;
  return s;
}

/// "attn_guard shape=<QxKxV> tier=<n>"
/// shape strings are opaque in this layer; callers supply the serialised form.
inline std::string attn_guard_event(std::string_view shape, int tier) {
  std::string s;
  s.reserve(64);
  s += "attn_guard shape=";
  s += shape;
  s += " tier=";
  s += std::to_string(tier);
  return s;
}

/// "qmatmul_guard policy=allow|deny scale=<int> wt_hash=<hash>"
inline std::string qmatmul_guard_event(bool allow,
                                       int32_t scale,
                                       std::string_view wt_hash) {
  std::string s;
  s.reserve(96);
  s += "qmatmul_guard policy=";
  s += allow ? "allow" : "deny";
  s += " scale=";
  s += std::to_string(scale);
  s += " wt_hash=";
  s += wt_hash;
  return s;
}

/// "ai_exec_gate backend=t81vm policy=allow|deny"
inline std::string ai_exec_gate_event(bool allow) {
  return allow ? "ai_exec_gate backend=t81vm policy=allow"
               : "ai_exec_gate backend=t81vm policy=deny";
}

}  // namespace ai_reasons

// ── AIHookEngine ──────────────────────────────────────────────────────────────

/// Engine wrapper that enforces AI-specific pre-execution guards and emits
/// canonical Axion AI event trace strings before delegating to an inner engine.
///
/// Registration: pass a constructed AIHookEngine as the Engine to AxionContext.
///
/// Side-effect-free with respect to VM state: hooks MAY read ctx fields for
/// guard evaluation but MUST NOT modify tensor pool contents, register values,
/// or program counter (RFC-0032 §6.4).
class AIHookEngine final : public Engine {
public:
  explicit AIHookEngine(std::unique_ptr<Engine> inner);

  /// Evaluate ctx:
  ///  1. Emit pre-execution AI event trace string for recognised AI opcodes.
  ///  2. Apply AI-specific guard checks (e.g. tier ≥ 2 for ATTN).
  ///  3. Emit ai_exec_gate event.
  ///  4. Delegate to inner engine for full policy evaluation.
  ///
  /// On DENY from any guard, returns Deny without calling inner engine.
  Verdict evaluate(const SyscallContext& ctx) override;

  /// Returns the accumulated AI event trace since construction or last clear.
  const std::vector<std::string>& ai_trace() const noexcept { return trace_; }

  /// Clear the accumulated trace.
  void clear_trace() noexcept { trace_.clear(); }

private:
  std::unique_ptr<Engine>  inner_;
  std::vector<std::string> trace_;  ///< AI event trace (RFC-0032 §8.2)

  void push(std::string event) { trace_.push_back(std::move(event)); }
};

/// Factory: wrap any engine with AI hook guards.
/// Returns AIHookEngine owning the supplied inner engine.
std::unique_ptr<Engine> make_ai_hook_engine(std::unique_ptr<Engine> inner);

}  // namespace t81::axion
