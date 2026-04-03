#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace t81::canonfs {

// Canonical Optimization Decision Structure
struct CanonicalOptimizationDecision {
    // Core Identification
    std::string decision_id;                    // Unique identifier (hash-based)
    std::string timestamp;                       // ISO 8601 timestamp
    std::string version;                         // Decision format version
    
    // Performance Context
    std::map<std::string, double> performance_metrics;  // Current system state
    std::string performance_pattern;                          // Detected pattern
    double pattern_confidence;                                // Pattern confidence score
    
    // Decision Content
    std::string optimization_strategy;                           // Chosen strategy
    std::vector<std::string> optimization_sequence;               // Execution sequence
    std::map<std::string, double> strategy_weights;          // Neural network outputs
    double decision_confidence;                                 // Overall confidence
    
    // Policy Compliance
    std::vector<std::string> applicable_policies;               // Relevant policies
    std::map<std::string, bool> policy_compliance;          // Compliance check
    std::string policy_rationale;                                // Policy reasoning
    
    // Execution Parameters
    std::map<std::string, std::string> execution_parameters;      // Configurable parameters
    std::string execution_mode;                                   // sync/async/batch
    std::chrono::milliseconds expected_duration;                     // Estimated duration
    
    // Deterministic Guarantees
    std::string deterministic_hash;                                // Content hash
    std::vector<std::string> preconditions;                       // Required conditions
    std::vector<std::string> expected_outcomes;                   // Predicted results
    std::string rollback_strategy;                                   // Rollback procedure
    
    // Provenance & Audit
    std::string model_version;                                      // AI model version
    std::string training_data_hash;                                 // Training data fingerprint
    std::vector<std::string> feature_importance;                    // Feature contributions
    std::string reasoning_chain;                                     // AI reasoning
    
    // Serialization
    std::string to_canonical_json() const;
    std::string to_canonical_binary() const;
    bool validate_canonical_form() const;
    
    // Replayability
    bool is_replayable() const;
    std::string get_replay_instructions() const;
    CanonicalOptimizationDecision create_rollback_decision() const;
};

// Policy-Bound Decision Engine
class PolicyBoundOptimizationEngine {
public:
    PolicyBoundOptimizationEngine();
    
    // Core Decision Generation
    CanonicalOptimizationDecision generate_canonical_decision(
        const std::map<std::string, double>& current_metrics,
        const std::string& performance_pattern,
        const std::map<std::string, double>& neural_outputs
    );
    
    // Policy Compliance
    bool validate_policy_compliance(const CanonicalOptimizationDecision& decision);
    std::vector<std::string> get_violated_policies(const CanonicalOptimizationDecision& decision);
    
    // Deterministic Guarantees
    std::string compute_deterministic_hash(const CanonicalOptimizationDecision& decision);
    bool verify_deterministic_replay(const CanonicalOptimizationDecision& decision);
    
    // Serialization & Storage
    std::string serialize_decision(const CanonicalOptimizationDecision& decision);
    CanonicalOptimizationDecision deserialize_decision(const std::string& serialized);
    void store_decision(const CanonicalOptimizationDecision& decision);
    std::vector<CanonicalOptimizationDecision> load_decision_history();

private:
    std::string current_model_version_;
    std::map<std::string, std::vector<std::string>> policy_rules_;
    
    // Policy Enforcement
    std::vector<std::string> check_policy_constraints(
        const std::string& strategy,
        const std::map<std::string, double>& metrics
    );
    
    // Deterministic Hashing
    std::string compute_content_hash(const std::string& content);
    std::string compute_decision_hash(const CanonicalOptimizationDecision& decision);
    
    // Validation
    bool validate_decision_structure(const CanonicalOptimizationDecision& decision);
    bool validate_policy_constraints(const CanonicalOptimizationDecision& decision);
};

// Replay Engine for Deterministic Execution
class DeterministicReplayEngine {
public:
    DeterministicReplayEngine();
    
    // Replay Operations
    bool replay_decision(const CanonicalOptimizationDecision& decision);
    std::map<std::string, double> execute_with_verification(
        const CanonicalOptimizationDecision& decision
    );
    
    // State Management
    void capture_initial_state();
    void capture_final_state();
    bool verify_deterministic_outcome(
        const CanonicalOptimizationDecision& decision,
        const std::map<std::string, double>& actual_outcome
    );
    
    // Rollback Support
    bool rollback_decision(const CanonicalOptimizationDecision& decision);
    bool verify_rollback_integrity(const CanonicalOptimizationDecision& rollback);

private:
    std::map<std::string, double> initial_state_;
    std::map<std::string, double> final_state_;
    
    // Execution Tracking
    std::vector<std::map<std::string, double>> execution_snapshots_;
    std::vector<std::string> execution_log_;
    
    // Verification
    bool compare_states(
        const std::map<std::string, double>& expected,
        const std::map<std::string, double>& actual,
        double tolerance = 0.01
    );
};

} // namespace t81::canonfs
