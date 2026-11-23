#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "t81/axion/engine.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/experimental/cog/tier.hpp"
#include "t81/fraction.hpp"
#include "t81/support/expected.hpp"
#include "t81/tensor.hpp"

namespace t81::vm::internal {

enum class TierPromotionError {
  NotEligible,
  AxionDenied,
};

using TierPromotionResult = t81::expected<t81::cog::TierStatus, TierPromotionError>;

bool hash_matches_policy_entry(std::string_view expected, std::string_view actual_checksum);

bool is_exact_ternary_tensor(const t81::T729DynamicTensor& tensor);

TierPromotionResult try_promote_tier(
    const t81::cog::TierStatus& status,
    const std::function<t81::axion::Verdict(const t81::axion::SyscallContext&)>& callback);

std::unique_ptr<t81::axion::Engine> make_deny_with_reason_engine(std::string reason);

t81::T81Fraction fraction_from_double(double x);

}  // namespace t81::vm::internal
