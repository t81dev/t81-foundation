#include "t81/axion/policy_engine.hpp"

#include <sstream>
#include "t81/tisc/opcodes.hpp"

namespace t81::axion {

PolicyEngine::PolicyEngine(std::optional<Policy> policy) : policy_(std::move(policy)) {
  if (policy_ && !policy_->loops.empty()) {
    loop_requirements_.reserve(policy_->loops.size());
    for (const auto& hint : policy_->loops) {
      LoopRequirement req;
      req.hint = &hint;
      std::ostringstream expect;
      expect << "loop hint file=" << hint.file << " line=" << hint.line
             << " column=" << hint.column << " bound=";
      if (hint.bound_infinite) {
        expect << "infinite";
      } else if (hint.bound_value) {
        expect << *hint.bound_value;
      } else {
        expect << "unknown";
      }
      req.expected_reason = expect.str();
      loop_requirements_.push_back(std::move(req));
    }
  }
}

Verdict PolicyEngine::evaluate(const SyscallContext& ctx) {
  if (!policy_) {
    return Verdict{VerdictKind::Allow, "Axion policy engine (no policy)"};
  }
  for (const auto& req : loop_requirements_) {
    if (!loop_hint_satisfied(ctx, *req.hint)) {
      std::ostringstream reason;
      reason << "Missing loop hint trace: " << req.expected_reason;
      return Verdict{VerdictKind::Deny, reason.str()};
    }
  }
  const bool final_instruction = ctx.next_opcode == t81::tisc::Opcode::Halt;
  if (final_instruction) {
    for (const auto& req : policy_->match_guards) {
      if (!match_guard_satisfied(ctx, req)) {
        std::ostringstream reason;
        reason << "Missing match guard event: enum=" << req.enum_name
               << " variant=" << req.variant_name;
        if (req.payload) reason << " payload=" << *req.payload;
        reason << " result=" << req.result;
        return Verdict{VerdictKind::Deny, reason.str()};
      }
    }
    for (const auto& req : policy_->segment_requirements) {
      if (!segment_event_satisfied(ctx, req)) {
        std::ostringstream reason;
        reason << "Missing segment event: action=\"" << req.action << "\""
               << " segment=" << req.segment;
        if (req.addr) reason << " addr=" << *req.addr;
        return Verdict{VerdictKind::Deny, reason.str()};
      }
    }
    for (const auto& req : policy_->axion_event_requirements) {
      if (!axion_event_satisfied(ctx, req)) {
        std::ostringstream reason;
        reason << "Missing Axion event reason containing \"" << req.reason << "\"";
        return Verdict{VerdictKind::Deny, reason.str()};
      }
    }
  }
  return Verdict{VerdictKind::Allow, "Axion policy engine (loop hints satisfied)"};
}

bool PolicyEngine::loop_hint_satisfied(const SyscallContext& ctx,
                                       const Policy::LoopHint& hint) const {
  const std::string expected = std::string("loop hint file=") + hint.file +
                               " line=" + std::to_string(hint.line) +
                               " column=" + std::to_string(hint.column) +
                               " bound=" +
                               (hint.bound_infinite
                                    ? "infinite"
                                    : (hint.bound_value ? std::to_string(*hint.bound_value)
                                                       : "unknown"));
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find(expected) != std::string_view::npos) {
      return true;
    }
  }
  return false;
}

bool PolicyEngine::match_guard_satisfied(const SyscallContext& ctx,
                                         const Policy::MatchGuardRequirement& req) const {
  const std::string enum_token = "enum=" + req.enum_name;
  const std::string variant_token = "variant=" + req.variant_name;
  const std::string match_token = "match=" + req.result;
  const std::string payload_token = req.payload ? "payload=" + *req.payload : std::string();
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find("enum guard") == std::string_view::npos) continue;
    if (!req.enum_name.empty() && entry.find(enum_token) == std::string_view::npos) continue;
    if (!req.variant_name.empty() && entry.find(variant_token) == std::string_view::npos) continue;
    if (req.payload && entry.find(payload_token) == std::string_view::npos) continue;
    if (entry.find(match_token) == std::string_view::npos) continue;
    return true;
  }
  return false;
}

bool PolicyEngine::segment_event_satisfied(const SyscallContext& ctx,
                                           const Policy::SegmentEventRequirement& req) const {
  const std::string segment_eq = "segment=" + req.segment;
  const std::string segment_spaced = " " + req.segment + " ";
  const std::string addr_token = req.addr ? ("addr=" + std::to_string(*req.addr)) : std::string();
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find(req.action) == std::string_view::npos) continue;
    bool segment_ok = req.segment.empty();
    if (!segment_ok) {
      segment_ok = entry.find(segment_eq) != std::string_view::npos ||
                   entry.find(segment_spaced) != std::string_view::npos;
    }
    if (!segment_ok) continue;
    if (!addr_token.empty() && entry.find(addr_token) == std::string_view::npos) continue;
    return true;
  }
  return false;
}

bool PolicyEngine::axion_event_satisfied(const SyscallContext& ctx,
                                         const Policy::AxionEventRequirement& req) const {
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find(req.reason) != std::string_view::npos) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<Engine> make_policy_engine(std::optional<Policy> policy) {
  return std::make_unique<PolicyEngine>(std::move(policy));
}

}  // namespace t81::axion
