#pragma once

#include <string>

namespace t81::cog {
enum class TierId {
  Tier0,
  Tier1,
  Tier2,
  Tier3,
  Tier4,
  Tier5,
  Tier6,  // RFC-0000 §6: T6561 (3^8) — Universal Cognition Tier (Θ₇), distributed recursive monads.
};

struct TierStatus {
  TierId current{TierId::Tier0};
  std::string label;
};
}  // namespace t81::cog
