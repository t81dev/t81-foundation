// T81 Axion Policy Engine Implementation
// Real policy validation and enforcement for T81-governed LLM inference

#include "t81/axion/policy.hpp"
#include "t81/support/expected_minimal.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace t81::axion {

struct PolicyViolation {
    std::string rule;
    std::string reason;
    bool is_violation;
};

class PolicyEngine {
public:
    static std::optional<PolicyViolation> validate_policy(const std::string& policy_text) {
        PolicyViolation violation;
        
        // Check if policy is empty
        if (policy_text.empty()) {
            violation.rule = "policy_empty";
            violation.reason = "Policy text cannot be empty";
            violation.is_violation = true;
            return violation;
        }
        
        // Check if policy starts with (policy
        if (policy_text.find("(policy") != 0) {
            violation.rule = "policy_format";
            violation.reason = "Policy must start with '(policy'";
            violation.is_violation = true;
            return violation;
        }
        
        // Check for required policy components
        bool has_tier = policy_text.find("(tier") != std::string::npos;
        bool has_max_instructions = policy_text.find("(max-instructions") != std::string::npos;
        bool has_max_tensors = policy_text.find("(max-tensors") != std::string::npos;
        bool has_allowed_hashes = policy_text.find("(allowed-tensor-hashes") != std::string::npos;
        
        if (!has_tier) {
            violation.rule = "missing_tier";
            violation.reason = "Policy must specify tier level";
            violation.is_violation = true;
            return violation;
        }
        
        if (!has_max_instructions) {
            violation.rule = "missing_max_instructions";
            violation.reason = "Policy must specify max-instructions";
            violation.is_violation = true;
            return violation;
        }
        
        if (!has_allowed_hashes) {
            violation.rule = "missing_allowed_hashes";
            violation.reason = "Policy must specify allowed-tensor-hashes";
            violation.is_violation = true;
            return violation;
        }
        
        // Parse policy to extract values
        auto parsed_policy = parse_policy(policy_text);
        if (!parsed_policy.has_value()) {
            violation.rule = "parse_failed";
            violation.reason = "Failed to parse policy structure";
            violation.is_violation = true;
            return violation;
        }
        
        // All checks passed
        violation.rule = "valid";
        violation.reason = "Policy is valid and properly structured";
        violation.is_violation = false;
        return violation;
    }
    
    static std::optional<PolicyViolation> validate_tensor_hash(const std::string& model_hash, 
                                                         const std::string& policy_text) {
        PolicyViolation violation;
        
        // Extract allowed hashes from policy
        auto parsed_policy = parse_policy(policy_text);
        if (!parsed_policy.has_value()) {
            violation.rule = "policy_parse_failed";
            violation.reason = "Cannot parse policy to extract allowed hashes";
            violation.is_violation = true;
            return violation;
        }
        
        // Check if model hash is in allowed list
        const auto& policy = parsed_policy.value();
        bool hash_allowed = false;
        
        for (const auto& hash : policy.allowed_tensor_hashes) {
            if (model_hash == hash) {
                hash_allowed = true;
                break;
            }
        }
        
        if (!hash_allowed) {
            violation.rule = "hash_not_allowed";
            violation.reason = "Model hash '" + model_hash + "' is not in allowed-tensor-hashes list";
            violation.is_violation = true;
            return violation;
        }
        
        // Hash validation passed
        violation.rule = "hash_valid";
        violation.reason = "Model hash is allowed by policy";
        violation.is_violation = false;
        return violation;
    }
    
    static std::string generate_policy_report(const PolicyViolation& violation) {
        std::stringstream report;
        report << "🔍 T81 Policy Validation Report\n";
        report << "Rule: " << violation.rule << "\n";
        report << "Reason: " << violation.reason << "\n";
        report << "Status: " << (violation.is_violation ? "❌ VIOLATION" : "✅ VALID") << "\n";
        return report.str();
    }
};

} // namespace t81::axion
