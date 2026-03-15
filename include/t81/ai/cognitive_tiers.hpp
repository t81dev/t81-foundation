#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace t81::ai::cognitive {

// Cognitive Tier enumeration for graduated capability levels
enum class CognitiveTier {
    TIER0_GROUND = 0,        // Load & validation state
    TIER1_SYMBOLIC = 1,       // Basic symbolic reasoning
    TIER2_REFLECTIVE = 2,      // Self-monitoring and adaptation
    TIER3_RECURSIVE = 3,      // Self-improvement and meta-learning
    TIER4_COLLABORATIVE = 4,   // Multi-agent coordination
    TIER5_INFINITE = 5         // Unbounded capability growth
};

// Tier capability definitions
struct TierCapabilities {
    CognitiveTier tier;
    std::string name;
    std::string description;
    
    // Computational limits
    size_t max_cpu_time_ms;
    size_t max_memory_mb;
    size_t max_storage_gb;
    bool network_access;
    
    // Learning constraints
    double max_learning_rate;
    size_t max_parameters;
    std::string data_access_level;
    size_t max_model_size_gb;
    
    // Governance requirements
    std::string access_control;
    std::string monitoring_level;
    std::string audit_frequency;
    std::string oversight_level;
    
    // Safety constraints
    std::vector<std::string> safety_protocols;
    bool human_oversight_required;
    bool emergency_shutdown_enabled;
};

// Operation context for tier enforcement
struct OperationContext {
    std::string operation_id;
    CognitiveTier current_tier;
    std::string user_id;
    std::string project_id;
    std::map<std::string, std::string> metadata;
    
    // Runtime constraints
    size_t execution_time_ms;
    size_t memory_used_mb;
    size_t network_calls_made;
    std::vector<std::string> operations_executed;
};

// Security and governance evaluation
struct SecurityEvaluation {
    bool access_granted;
    std::string reason;
    std::vector<std::string> violated_constraints;
    std::vector<std::string> required_approvals;
    
    // Monitoring requirements
    bool requires_real_time_monitoring;
    bool requires_human_oversight;
    bool requires_executive_approval;
};

// Tier promotion request
struct PromotionRequest {
    CognitiveTier from_tier;
    CognitiveTier to_tier;
    std::string requester_id;
    std::string project_id;
    
    // Justification and evidence
    std::string justification;
    std::vector<std::string> performance_evidence;
    std::vector<std::string> safety_assessments;
    std::vector<std::string> governance_reviews;
    
    // Timeline and milestones
    std::string request_date;
    std::string target_date;
    std::vector<std::string> milestones;
};

// Main cognitive tier engine interface
class CognitiveTierEngine {
public:
    virtual ~CognitiveTierEngine() = default;
    
    // Core tier management
    virtual bool can_execute_operation(
        const OperationContext& context,
        const std::string& operation_type) = 0;
    
    virtual SecurityEvaluation evaluate_security(
        const OperationContext& context,
        const std::string& operation_type) = 0;
    
    virtual bool verify_tier_constraints(
        const OperationContext& context,
        const CognitiveTier required_tier) = 0;
    
    // Tier promotion management
    virtual bool submit_promotion_request(
        const PromotionRequest& request) = 0;
    
    virtual std::vector<PromotionRequest> get_pending_promotions() = 0;
    
    virtual bool approve_promotion_request(
        const std::string& request_id,
        const std::string& approver_id,
        const std::string& justification) = 0;
    
    // Capability management
    virtual TierCapabilities get_tier_capabilities(
        CognitiveTier tier) = 0;
    
    virtual std::vector<CognitiveTier> get_available_tiers() = 0;
    
    // Monitoring and enforcement
    virtual void log_tier_operation(
        const OperationContext& context,
        const std::string& operation_type,
        const std::string& result) = 0;
    
    virtual bool check_tier_violation(
        const OperationContext& context,
        const std::string& violation_type) = 0;
    
    virtual void enforce_tier_limits(
        const OperationContext& context) = 0;
};

// Tier enforcement implementation
class TierEnforcement {
public:
    // Static enforcement methods
    static SecurityEvaluation check_tier_access(
        const std::string& user_id,
        const std::string& operation_type,
        CognitiveTier current_tier,
        CognitiveTier required_tier);
    
    static bool verify_operation_safety(
        const OperationContext& context,
        const std::string& operation_result);
    
    static void log_tier_violation(
        const OperationContext& context,
        const std::string& violation_type,
        const std::string& reason);
    
    static bool requires_human_oversight(
        CognitiveTier tier,
        const std::string& operation_type);
    
    static std::vector<std::string> get_required_approvals(
        CognitiveTier tier,
        const std::string& operation_type);
};

// Tier capability definitions
namespace tier_capabilities {
    static const TierCapabilities TIER1_CAPABILITIES = {
        CognitiveTier::TIER0_GROUND,
        "Ground State",
        "Load and validation state - no computation",
        0, 0, 0, false,  // No computational limits
        0.0, 0, "none", 0,  // No learning
        "validation", "basic", "none", "system",  // Basic governance
        {}, false, false  // Minimal safety
    };
    
    static const TierCapabilities TIER1_CAPABILITIES = {
        CognitiveTier::TIER1_SYMBOLIC,
        "Symbolic",
        "Basic symbolic reasoning and logic",
        static_cast<size_t>(1000), static_cast<size_t>(100), static_cast<size_t>(1), false,  // 1s, 100MB, 1GB, no network
        0.001, static_cast<size_t>(1000000), "pre-approved", static_cast<size_t>(0.1),  // Fixed learning, 1M params, 100MB
        "role-based", "basic", "annual", "development",  // Role-based access
        {"fixed_computation", "determinism_verification"}, false, false  // Basic safety
    };
    
    static const TierCapabilities TIER2_CAPABILITIES = {
        CognitiveTier::TIER2_REFLECTIVE,
        "Reflective",
        "Self-monitoring and dynamic adaptation",
        static_cast<size_t>(10000), static_cast<size_t>(1024), static_cast<size_t>(10), false,  // 10s, 1GB, 10GB, read-only network
        0.01, static_cast<size_t>(10000000), "curated", static_cast<size_t>(1.0),  // Adaptive learning, 10M params, 1GB
        "capability-based", "performance_metrics", "quarterly", "senior_developers",  // Enhanced governance
        {"bounded_self_modification", "learning_rate_limits"}, false, false  // Enhanced safety
    };
    
    static const TierCapabilities TIER3_CAPABILITIES = {
        CognitiveTier::TIER3_RECURSIVE,
        "Recursive",
        "Self-improvement and meta-learning",
        static_cast<size_t>(60000), static_cast<size_t>(2048), static_cast<size_t>(100), true,  // 1min, 2GB, 100GB, limited network
        0.01, static_cast<size_t>(100000000), "public_datasets", static_cast<size_t>(10.0),  // Meta-learning, 100M params, 10GB
        "project-based", "full_trace_logging", "monthly", "governance_council",  // Council oversight
        {"restricted_learning", "recursion_depth_limits", "human_oversight"}, true, false  // Enhanced safety
    };
    
    static const TierCapabilities TIER4_CAPABILITIES = {
        CognitiveTier::TIER4_COLLABORATIVE,
        "Collaborative",
        "Multi-agent coordination and communication",
        static_cast<size_t>(600000), static_cast<size_t>(4096), static_cast<size_t>(1024), true,  // 10min, 4GB, 1TB, monitored network
        0.01, static_cast<size_t>(1000000000), "internet_access", static_cast<size_t>(100.0),  // Collaborative learning, 1B params, 100GB
        "organization-based", "comprehensive_monitoring", "bi_weekly", "human_oversight_committee",  // Committee oversight
        {"human_in_the_loop", "communication_rate_limiting", "fail_safe_mechanisms"}, true, true  // Advanced safety
    };
    
    static const TierCapabilities TIER5_CAPABILITIES = {
        CognitiveTier::TIER5_INFINITE,
        "Infinite",
        "Unbounded capability growth and autonomous research",
        static_cast<size_t>(3600000), static_cast<size_t>(8192), static_cast<size_t>(10240), true,  // 1hr, 8GB, 10TB, full network
        0.01, static_cast<size_t>(0), "full_internet", static_cast<size_t>(1000.0),  // Autonomous learning, unlimited params, 1TB
        "executive_approval", "real_time_monitoring", "weekly", "executive_board",  // Executive oversight
        {"comprehensive_safety_protocols", "emergency_shutdown", "international_cooperation"}, true, true  // Full safety
    };
}

// Utility functions
std::string tier_to_string(CognitiveTier tier);
CognitiveTier string_to_tier(const std::string& tier_str);
bool is_tier_valid(CognitiveTier tier);
std::vector<CognitiveTier> get_promotion_path(CognitiveTier from_tier, CognitiveTier to_tier);

// Factory function
std::unique_ptr<CognitiveTierEngine> create_cognitive_tier_engine();

} // namespace t81::ai::cognitive
