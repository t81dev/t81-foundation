// kernel/axion/ai_model_loader.cpp
//
// TLOADHASH-gated AI model loader — RFC-0032 Phase 3 (C-03).
//
// Ad hoc hash verification has been replaced with Axion policy evaluation.
// Model loading is exclusively gated by the TLOADHASH path in PolicyEngine
// (kernel/axion/policy_engine.cpp §evaluate_internal).

#include "t81/axion/ai_model_loader.hpp"

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/context.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/isa/opcodes.hpp"

#include <sstream>

namespace t81::axion {

AiLoadResult load_model_via_tloadhash(Engine&          engine,
                                       std::string_view hash,
                                       std::string_view caller) {
  // Build the SyscallContext for a TLOADHASH pre-execution check.
  // The payload carries the content hash presented to the policy whitelist.
  SyscallContext ctx;
  ctx.next_opcode = t81::tisc::Opcode::TLoadHash;
  ctx.payload     = std::string{hash};
  ctx.caller      = std::string{caller};

  const Verdict verdict = engine.evaluate(ctx);
  const bool    allowed = (verdict.kind == VerdictKind::Allow);

  AiLoadResult result;
  result.ok          = allowed;
  result.hash        = std::string{hash};
  result.reason      = verdict.reason;
  result.trace_event = ai_reasons::model_load_event(allowed, hash,
                           allowed ? "allow" : verdict.reason);

  // Note: if !result.ok the caller MUST NOT materialise the model in the
  // tensor pool (RFC-0032 §6.3, RFC-0025 §3.3.3). No further action here;
  // the caller owns the SecurityFault responsibility.
  return result;
}

}  // namespace t81::axion
