#pragma once

#include <optional>
#include <string>
#include "t81/axion/policy.hpp"

namespace t81::axion {

struct PolicyViolation {
    std::string rule;
    std::string reason;
    bool is_violation;
};

class PolicyValidator {
public:
    static std::optional<PolicyViolation> validate_policy(const std::string& policy_text);
    static std::optional<PolicyViolation> validate_tensor_hash(const std::string& model_hash, 
                                                         const std::string& policy_text);
    static std::string generate_policy_report(const PolicyViolation& violation);
};

} // namespace t81::axion
