#pragma once

#include <t81/support/expected.hpp>
#include "t81/axion/engine.hpp"
#include "t81/cog/tier.hpp"

namespace t81::cog {
enum class PromotionError {
  NotEligible,
  AxionDenied,
};

template <typename T>
using Result = std::expected<T, PromotionError>;

Result<TierStatus> try_promote(const TierStatus& status, t81::axion::Engine& engine);
}  // namespace t81::cog

