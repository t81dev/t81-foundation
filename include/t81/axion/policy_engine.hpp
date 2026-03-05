#pragma once

#include <optional>
#include <memory>
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

struct PolicyViolation {
    std::string rule;
    std::string reason;
    bool is_violation;
};

class PolicyEngine : public Engine {
public:
  explicit PolicyEngine(std::optional<Policy> policy);
  Verdict evaluate(const SyscallContext& ctx) override;
  static std::optional<PolicyViolation> validate_policy(const std::string& policy_text);
  static std::optional<PolicyViolation> validate_tensor_hash(const std::string& model_hash, 
                                                         const std::string& policy_text);
  static std::string generate_policy_report(const PolicyViolation& violation);

private:
  bool loop_hint_satisfied(const SyscallContext& ctx, size_t requirement_idx) const;
  bool match_guard_satisfied(const SyscallContext& ctx,
                             const Policy::MatchGuardRequirement& req) const;
  bool segment_event_satisfied(const SyscallContext& ctx,
                               const Policy::SegmentEventRequirement& req) const;
  bool axion_event_satisfied(const SyscallContext& ctx,
                             const Policy::AxionEventRequirement& req) const;
  bool alignment_event_satisfied(const SyscallContext& ctx,
                                 const Policy::AlignmentRequirement& req) const;

  Verdict execute_bytecode(const SyscallContext& ctx);
  Verdict evaluate_internal(const SyscallContext& ctx);

  std::optional<Policy> policy_;
  struct InternalLoopReq {
    const Policy::LoopHint* hint;
    std::string expected_reason;
    mutable bool satisfied = false;
  };
  std::vector<InternalLoopReq> loop_reqs_;
};

std::unique_ptr<Engine> make_policy_engine(std::optional<Policy> policy);

}  // namespace t81::axion
