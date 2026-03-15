// include/t81/axion/ai_model_loader.hpp
//
// TLOADHASH-gated AI model loader — RFC-0032 Phase 3 (C-03).
// Promoted from experiments/ai/model_provenance/model_manager.cpp.
//
// All ad hoc hash verification has been removed. Model loading is performed
// exclusively via the TLOADHASH TISC instruction path, mediated by the
// Axion policy engine (RFC-0025 §3.3, RFC-0032 §6.3).

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "t81/axion/engine.hpp"

namespace t81::axion {

// ── Load result ───────────────────────────────────────────────────────────────

/// Result of a policy-gated model load attempt.
struct AiLoadResult {
  bool        ok{false};      ///< true if the load was allowed and succeeded
  std::string hash;           ///< the hash that was presented to the engine
  std::string reason;         ///< verbatim Axion verdict reason string
  std::string trace_event;    ///< canonical "model_load ..." trace event emitted
};

// ── load_model_via_tloadhash ──────────────────────────────────────────────────

/// Request a model load via the TLOADHASH Axion gate.
///
/// Constructs a SyscallContext with:
///   next_opcode = TLoadHash
///   payload     = hash
/// and calls engine.evaluate(ctx).
///
/// Emits the canonical Axion trace event (RFC-0032 §8.2):
///   "model_load success|failure hash=<hash> reason=<reason>"
/// This event is embedded in the returned AiLoadResult::trace_event.
///
/// A DENY verdict means the model MUST NOT be materialised in the tensor pool.
/// A ALLOW verdict means the caller may proceed to deserialise the weight data.
///
/// @param engine  Active Axion engine (typically AIHookEngine or PolicyEngine).
/// @param hash    Content hash of the model artifact (sha3-256 hex string per
///                RFC-0025 §3.3.3; the `sha3-256:` prefix is optional here).
/// @param caller  Identifier of the calling subsystem (for audit trace).
[[nodiscard]] AiLoadResult load_model_via_tloadhash(Engine&          engine,
                                                     std::string_view hash,
                                                     std::string_view caller = "ai_model_loader");

}  // namespace t81::axion
