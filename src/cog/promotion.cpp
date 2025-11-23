#include "t81/cog/promotion.hpp"

#include "t81/axion/context.hpp"

namespace t81::cog {
Result<TierStatus> try_promote(const TierStatus& status, t81::axion::Engine& engine) {
  if (status.current == TierId::Tier2) {
    return PromotionError::NotEligible;
  }

  t81::axion::SyscallContext ctx{{}, "system", "promote"};
  auto verdict = engine.evaluate(ctx);
  if (verdict.kind == t81::axion::VerdictKind::Deny) {
    return PromotionError::AxionDenied;
  }

  TierStatus next = status;
  if (status.current == TierId::Tier0) {
    next.current = TierId::Tier1;
    next.label = "Tier1";
  } else if (status.current == TierId::Tier1) {
    next.current = TierId::Tier2;
    next.label = "Tier2";
  }
  return next;
}
}  // namespace t81::cog

