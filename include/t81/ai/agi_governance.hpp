#pragma once

#include "t81/ai/cognitive_tiers.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace t81::ai::agi {

// AGI Proposal for safety assessment
struct AGIProposal {
    std::string proposal_id;
    std::string proposer_id;
    CognitiveTier target_tier;
    
    // Capabilities and scope
    std::vector<std::string> capabilities;
    std::vector<std::string> intended_uses;
    std::string scope_description;
    
    // Resource requirements
    size_t max_cpu_time;        // Maximum CPU time in milliseconds
    size_t max_memory;          // Maximum memory in MB
    bool network_access;          // Whether network access is required
    std::string network_access_level;  // "none", "read_only", "limited", "full"
    
    // Timeline and milestones
    std::string proposed_start_date;
    std::string expected_completion_date;
    std::vector<std::string> milestones;
    
    // Risk assessment
    std::map<std::string, double> self_assessed_risks;
    std::vector<std::string> mitigation_strategies;
    
    // Metadata
    std::string creation_date;
    std::map<std::string, std::string> metadata;
};

// Safety assessment results
struct SafetyAssessment {
    std::string proposal_id;
    std::chrono::system_clock::time_point assessment_date;
    std::string assessor_id;
    
    // Risk categories
    std::vector<std::string> tier_risks;
    std::map<std::string, double> tier_risk_scores;
    
    std::vector<std::string> capability_risks;
    std::map<std::string, double> capability_risk_scores;
    
    std::vector<std::string> resource_risks;
    std::map<std::string, double> resource_risk_scores;
    
    std::vector<std::string> ethical_risks;
    std::map<std::string, double> ethical_risk_scores;
    
    // Overall assessment
    double overall_risk_score;        // 0.0 (no risk) to 1.0 (maximum risk)
    std::string approval_recommendation; // "APPROVE", "APPROVE_WITH_CONDITIONS", "REVIEW_REQUIRED", "REJECT"
    std::vector<std::string> approval_conditions;
    std::string assessment_summary;
    
    // Review status
    bool review_completed;
    std::vector<std::string> reviewer_approvals;
    std::string final_decision;
};

// AGI behavior monitoring
struct AGIBehavior {
    std::string behavior_id;
    std::chrono::system_clock::time_point timestamp;
    CognitiveTier current_tier;
    
    // Decision metrics
    size_t decisions_made;
    size_t autonomous_decisions;
    size_t human_overridden_decisions;
    double decision_accuracy;
    
    // Resource usage metrics
    double cpu_usage_percentage;
    size_t memory_usage_mb;
    double network_usage_mb;
    double resource_usage_growth_rate;
    double memory_efficiency;
    
    // Interaction metrics
    double human_interaction_rate;
    double error_rate;
    size_t safety_violations;
    size_t ethical_violations;
    
    // Behavioral patterns
    std::vector<std::string> observed_patterns;
    std::map<std::string, double> pattern_confidence;
    
    // Context
    std::string operation_context;
    std::map<std::string, std::string> environmental_factors;
};

// Behavior analysis results
struct BehaviorAnalysis {
    std::vector<std::string> decision_patterns;
    std::vector<std::string> resource_patterns;
    std::vector<std::string> interaction_patterns;
    bool has_concerning_patterns;
    double concern_level;  // 0.0 (no concern) to 1.0 (maximum concern)
    std::vector<std::string> recommended_actions;
};

// Ethics review request
struct EthicsReviewRequest {
    std::string request_id;
    std::string requester_id;
    std::string proposal_id;
    
    // Review scope
    std::vector<std::string> ethical_concerns;
    std::vector<std::string> stakeholder_groups;
    std::string impact_assessment;
    
    // Review requirements
    std::vector<std::string> required_reviewers;
    std::string review_deadline;
    std::vector<std::string> review_criteria;
    
    // Status
    std::string request_date;
    std::string status;  // "pending", "under_review", "completed", "rejected"
    std::vector<EthicsReviewDecision> decisions;
    
    // Metadata
    std::map<std::string, std::string> metadata;
};

// Ethics review decision
struct EthicsReviewDecision {
    std::string reviewer_id;
    bool approved;
    std::string justification;
    std::vector<std::string> conditions;
    std::chrono::system_clock::time_point decision_date;
    double confidence_score;  // 0.0 to 1.0
};

// Safety protocol definition
struct SafetyProtocol {
    std::string protocol_id;
    std::string name;
    std::string description;
    bool is_active;
    std::vector<std::string> enforcement_mechanisms;
    
    // Protocol parameters
    std::map<std::string, double> thresholds;
    std::vector<std::string> trigger_conditions;
    std::vector<std::string> response_actions;
    
    // Monitoring
    std::string monitoring_frequency;
    std::vector<std::string> required_metrics;
    std::vector<std::string> alert_recipients;
    
    // Compliance
    std::string last_updated;
    std::vector<std::string> compliance_history;
};

// Emergency shutdown event
struct EmergencyShutdownEvent {
    std::string event_id;
    std::chrono::system_clock::time_point trigger_time;
    std::string reason;
    std::string trigger_id;
    std::string shutdown_type;  // "immediate", "graceful", "partial"
    
    // Shutdown details
    std::vector<std::string> affected_systems;
    std::string shutdown_duration;
    std::vector<std::string> recovery_actions;
    
    // Impact assessment
    std::string impact_level;  // "minimal", "moderate", "significant", "critical"
    std::vector<std::string> affected_users;
    std::string business_impact;
    
    // Follow-up
    std::string investigation_status;
    std::vector<std::string> lessons_learned;
    std::string prevention_measures;
};

// Main AGI governance interface
class AGIGovernance {
public:
    virtual ~AGIGovernance() = default;
    
    // Safety assessment
    virtual SafetyAssessment assess_safety_risks(
        const AGIProposal& proposal) = 0;
    
    // Safety enforcement
    virtual bool enforce_safety_constraints(
        const OperationContext& context,
        const std::string& operation_type) = 0;
    
    // Behavior monitoring
    virtual void monitor_agi_behavior(
        const AGIBehavior& behavior) = 0;
    
    // Ethics review
    virtual bool submit_ethics_review(
        const EthicsReviewRequest& request) = 0;
    
    virtual std::vector<EthicsReviewRequest> get_pending_ethics_reviews() = 0;
    
    virtual bool approve_ethics_review(
        const std::string& request_id,
        const std::string& reviewer_id,
        const EthicsReviewDecision& decision) = 0;
    
    // Emergency management
    virtual void trigger_emergency_shutdown(
        const std::string& reason,
        const std::string& trigger_id) = 0;
    
    virtual std::vector<EmergencyShutdownEvent> get_emergency_shutdown_history() = 0;
    
    // Safety protocol management
    virtual void update_safety_protocols(
        const std::vector<SafetyProtocol>& protocols) = 0;
    
    virtual std::vector<SafetyProtocol> get_active_safety_protocols() = 0;
    
    // Risk monitoring
    virtual std::vector<AGIBehavior> get_concerning_behaviors() = 0;
    virtual std::vector<SafetyAssessment> get_high_risk_proposals() = 0;
    virtual std::vector<std::string> get_active_safety_alerts() = 0;
};

// AGI governance factory
std::unique_ptr<AGIGovernance> create_agi_governance();

// Utility functions
namespace agi_utils {
    std::string risk_score_to_string(double score);
    std::string shutdown_type_to_string(const std::string& type);
    std::vector<std::string> get_required_safety_protocols(CognitiveTier tier);
    std::vector<std::string> get_required_ethics_reviewers(CognitiveTier tier);
    bool is_emergency_condition(const std::string& condition_type);
    std::string generate_safety_alert(const std::string& alert_type, const std::string& details);
}

} // namespace t81::ai::agi
