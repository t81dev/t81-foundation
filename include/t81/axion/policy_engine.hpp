#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "t81/axion/context.hpp"
#include "t81/axion/engine.hpp"
#include "t81/axion/policy.hpp"

namespace t81::axion {

struct LoopRequirement {
  const Policy::LoopHint* hint{nullptr};
  std::string_view expected_reason;
};

class PolicyEngine : public Engine {
 public:
  explicit PolicyEngine(std::optional<Policy> policy);
  Verdict evaluate(const SyscallContext& ctx) override;

 private:
  bool loop_hint_satisfied(const SyscallContext& ctx,
                           const Policy::LoopHint& hint) const;
  bool match_guard_satisfied(const SyscallContext& ctx,
                             const Policy::MatchGuardRequirement& req) const;
  bool segment_event_satisfied(const SyscallContext& ctx,
                               const Policy::SegmentEventRequirement& req) const;

  std::optional<Policy> policy_;
  std::vector<LoopRequirement> loop_requirements_;
};

std::unique_ptr<Engine> make_policy_engine(std::optional<Policy> policy);

}  // namespace t81::axion
