#include "t81/experimental/cog/promotion.hpp"

#include "t81/axion/context.hpp"

namespace t81::cog {
Result<TierStatus> try_promote(const TierStatus& status, AxionCallback callback) {
  if (status.current == TierId::Tier6) {
    // RFC-0000 §6: Tier6 (T6561) is the highest defined tier; no further promotion.
    return Result<TierStatus>(t81::unexpect, PromotionError::NotEligible);
  }

  t81::axion::SyscallContext ctx{{},      "system", "promote", "",
                                 nullptr, {},       0,         t81::tisc::Opcode::Nop};
  auto verdict = callback(ctx);
  if (verdict.kind == t81::axion::VerdictKind::Deny) {
    return Result<TierStatus>(t81::unexpect, PromotionError::AxionDenied);
  }

  TierStatus next = status;
  switch (status.current) {
    case TierId::Tier0:
      next.current = TierId::Tier1;
      next.label = "Tier1";
      break;
    case TierId::Tier1:
      next.current = TierId::Tier2;
      next.label = "Tier2";
      break;
    case TierId::Tier2:
      next.current = TierId::Tier3;
      next.label = "Tier3";
      break;
    case TierId::Tier3:
      next.current = TierId::Tier4;
      next.label = "Tier4";
      break;
    case TierId::Tier4:
      next.current = TierId::Tier5;
      next.label = "Tier5";
      break;
    case TierId::Tier5:
      // RFC-0000 §6: Promote to T6561 — distributed recursive monads (Θ₇).
      next.current = TierId::Tier6;
      next.label = "Tier6";
      break;
    default:
      return Result<TierStatus>(t81::unexpect, PromotionError::NotEligible);
  }
  return next;
}

bool should_promote_to_tier4(const v1::ReflectionTrace& trace) {
  // Heuristic: promote if confidence is low.
  if (trace.confidence < 0.81f) {
    return true;
  }

  // Heuristic: promote if we have three or more observations without successful refinement
  // (implied by recalibrate goal persisting in snapshot).
  int observations = 0;
  for (const auto& event : trace.history_snapshot) {
    if (event.find("Observation:") != std::string::npos) {
      observations++;
    }
  }

  if (observations >= 3 && trace.goal == "recalibrate") {
    return true;
  }

  // Tier 4 specifically handles self-referential paradoxes or inconsistencies
  // recorded in the trace reason.
  if (trace.reason.find("inconsistency") != std::string::npos ||
      trace.reason.find("paradox") != std::string::npos) {
    return true;
  }

  return false;
}

}  // namespace t81::cog
