#pragma once

#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/interchange_ops.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace t81::canonfs {

inline std::optional<t81::axion::Policy> load_interchange_axion_policy(
    const std::filesystem::path& policy_path, std::string& error_message) {
  std::ifstream in(policy_path);
  if (!in) {
    error_message = "could not open policy file: " + policy_path.string();
    return std::nullopt;
  }
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  auto parsed = t81::axion::parse_policy(content);
  if (!parsed) {
    error_message = "policy parse failed: " + parsed.error();
    return std::nullopt;
  }
  return parsed.value();
}

inline InterchangePolicyEvaluator make_axion_interchange_policy_evaluator(
    const std::optional<t81::axion::Policy>& policy, bool fallback_reason_to_path = false) {
  if (!policy) {
    return {};
  }
  return [policy, fallback_reason_to_path](std::string_view canonical_ref, std::string_view operation,
                                           std::string_view path) -> InterchangePolicyDecision {
    t81::axion::PolicyEngine engine(*policy);
    t81::axion::SyscallContext ctx;
    ctx.caller = "t81 canonfs";
    ctx.syscall = std::string(operation);
    ctx.payload = std::string(canonical_ref);
    ctx.next_opcode = t81::tisc::Opcode::TLoadHash;
    ctx.current_tier = 1;
    const auto verdict = engine.evaluate(ctx);
    std::string reason = verdict.reason;
    if (reason.empty()) {
      reason = fallback_reason_to_path ? std::string(path) : "allow";
    }
    return InterchangePolicyDecision{
        .allowed = verdict.kind != t81::axion::VerdictKind::Deny,
        .reason = std::move(reason),
    };
  };
}

}  // namespace t81::canonfs
