#include "t81/axion/policy_validator.hpp"

#include <sstream>

namespace t81::axion {

std::optional<PolicyViolation> PolicyValidator::validate_policy(const std::string& policy_text) {
  PolicyViolation violation{};
  if (policy_text.empty()) {
    violation.rule = "policy_empty";
    violation.reason = "Policy text cannot be empty";
    violation.is_violation = true;
    return violation;
  }

  const auto parsed = parse_policy(policy_text);
  if (!parsed.has_value()) {
    violation.rule = "policy_parse_failed";
    violation.reason = parsed.error();
    violation.is_violation = true;
    return violation;
  }

  const Policy& policy = parsed.value();
  if (policy.allowed_tensor_hashes.empty()) {
    violation.rule = "missing_allowed_tensor_hashes";
    violation.reason = "Policy must include at least one allowed tensor hash";
    violation.is_violation = true;
    return violation;
  }

  violation.rule = "valid";
  violation.reason = "Policy parse/structure checks passed";
  violation.is_violation = false;
  return violation;
}

std::optional<PolicyViolation> PolicyValidator::validate_tensor_hash(
    const std::string& model_hash,
    const std::string& policy_text) {
  PolicyViolation violation{};
  const auto parsed = parse_policy(policy_text);
  if (!parsed.has_value()) {
    violation.rule = "policy_parse_failed";
    violation.reason = parsed.error();
    violation.is_violation = true;
    return violation;
  }

  const auto& allowed = parsed.value().allowed_tensor_hashes;
  for (const auto& hash : allowed) {
    if (hash == model_hash) {
      violation.rule = "hash_valid";
      violation.reason = "Model hash is allowed by policy";
      violation.is_violation = false;
      return violation;
    }
  }

  violation.rule = "hash_not_allowed";
  violation.reason = "Model hash is not listed in allowed-tensor-hashes";
  violation.is_violation = true;
  return violation;
}

std::string PolicyValidator::generate_policy_report(const PolicyViolation& violation) {
  std::ostringstream os;
  os << "Rule: " << violation.rule << "\n";
  os << "Reason: " << violation.reason << "\n";
  os << "Status: " << (violation.is_violation ? "VIOLATION" : "VALID");
  return os.str();
}

}  // namespace t81::axion
