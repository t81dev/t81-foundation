#include "t81/canonfs/canonical_optimization_decision.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/sha.h>

namespace t81::canonfs {

// Implementation of CanonicalOptimizationDecision methods
std::string CanonicalOptimizationDecision::to_canonical_json() const {
    nlohmann::json decision_json;
    
    // Core Identification
    decision_json["decision_id"] = decision_id;
    decision_json["timestamp"] = timestamp;
    decision_json["version"] = version;
    
    // Performance Context
    decision_json["performance_context"]["metrics"] = performance_metrics;
    decision_json["performance_context"]["pattern"] = performance_pattern;
    decision_json["performance_context"]["confidence"] = pattern_confidence;
    
    // Decision Content
    decision_json["decision"]["strategy"] = optimization_strategy;
    decision_json["decision"]["sequence"] = optimization_sequence;
    decision_json["decision"]["weights"] = strategy_weights;
    decision_json["decision"]["confidence"] = decision_confidence;
    
    // Policy Compliance
    decision_json["policy"]["applicable"] = applicable_policies;
    decision_json["policy"]["compliance"] = policy_compliance;
    decision_json["policy"]["rationale"] = policy_rationale;
    
    // Execution Parameters
    decision_json["execution"]["parameters"] = execution_parameters;
    decision_json["execution"]["mode"] = execution_mode;
    decision_json["execution"]["expected_duration_ms"] = expected_duration.count();
    
    // Deterministic Guarantees
    decision_json["deterministic"]["hash"] = deterministic_hash;
    decision_json["deterministic"]["preconditions"] = preconditions;
    decision_json["deterministic"]["expected_outcomes"] = expected_outcomes;
    decision_json["deterministic"]["rollback_strategy"] = rollback_strategy;
    
    // Provenance & Audit
    decision_json["provenance"]["model_version"] = model_version;
    decision_json["provenance"]["training_data_hash"] = training_data_hash;
    decision_json["provenance"]["feature_importance"] = feature_importance;
    decision_json["provenance"]["reasoning_chain"] = reasoning_chain;
    
    return decision_json.dump(2); // Pretty print with 2-space indentation
}

std::string CanonicalOptimizationDecision::to_canonical_binary() const {
    // Create canonical binary representation
    std::ostringstream binary_stream;
    
    // Header
    binary_stream << "CANONFS_OPT_DECISION_v1";
    binary_stream << std::setw(19) << std::setfill('0') << timestamp;
    binary_stream << decision_id;
    
    // Performance metrics (sorted for determinism)
    std::vector<std::pair<std::string, double>> sorted_metrics;
    for (const auto& [key, value] : performance_metrics) {
        sorted_metrics.push_back({key, value});
    }
    std::sort(sorted_metrics.begin(), sorted_metrics.end());
    
    binary_stream << sorted_metrics.size();
    for (const auto& [key, value] : sorted_metrics) {
        binary_stream << key << ":" << std::fixed << std::setprecision(6) << value << ";";
    }
    
    // Decision content
    binary_stream << optimization_strategy;
    binary_stream << optimization_sequence.size();
    for (const auto& step : optimization_sequence) {
        binary_stream << step << "|";
    }
    
    // Policy compliance
    binary_stream << policy_compliance.size();
    for (const auto& [policy, compliant] : policy_compliance) {
        binary_stream << policy << ":" << (compliant ? "1" : "0") << ";";
    }
    
    // Deterministic hash
    binary_stream << deterministic_hash;
    
    std::string binary_content = binary_stream.str();
    
    // Compute SHA-256 hash of the binary content
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(binary_content.c_str()), 
           binary_content.length(), hash);
    
    // Convert hash to hex string
    std::ostringstream hash_stream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hash_stream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return hash_stream.str();
}

bool CanonicalOptimizationDecision::validate_canonical_form() const {
    // Validate required fields
    if (decision_id.empty() || timestamp.empty() || version.empty()) {
        return false;
    }
    
    if (optimization_strategy.empty() || optimization_sequence.empty()) {
        return false;
    }
    
    if (deterministic_hash.empty() || expected_outcomes.empty()) {
        return false;
    }
    
    // Validate policy compliance
    for (const auto& [policy, compliant] : policy_compliance) {
        if (policy.empty()) {
            return false;
        }
    }
    
    // Validate performance metrics
    for (const auto& [metric, value] : performance_metrics) {
        if (metric.empty() || std::isnan(value) || std::isinf(value)) {
            return false;
        }
    }
    
    return true;
}

bool CanonicalOptimizationDecision::is_replayable() const {
    // Check if decision has all required components for replay
    return validate_canonical_form() && 
           !preconditions.empty() && 
           !expected_outcomes.empty() && 
           !rollback_strategy.empty();
}

std::string CanonicalOptimizationDecision::get_replay_instructions() const {
    std::ostringstream instructions;
    
    instructions << "=== CanonFS Deterministic Replay Instructions ===\n\n";
    instructions << "Decision ID: " << decision_id << "\n";
    instructions << "Timestamp: " << timestamp << "\n";
    instructions << "Strategy: " << optimization_strategy << "\n\n";
    
    instructions << "Preconditions:\n";
    for (const auto& precondition : preconditions) {
        instructions << "  - " << precondition << "\n";
    }
    
    instructions << "\nExecution Sequence:\n";
    for (size_t i = 0; i < optimization_sequence.size(); ++i) {
        instructions << "  " << (i + 1) << ". " << optimization_sequence[i] << "\n";
    }
    
    instructions << "\nExpected Outcomes:\n";
    for (const auto& outcome : expected_outcomes) {
        instructions << "  - " << outcome << "\n";
    }
    
    instructions << "\nRollback Strategy:\n";
    instructions << "  " << rollback_strategy << "\n";
    
    instructions << "\nDeterministic Hash: " << deterministic_hash << "\n";
    
    return instructions.str();
}

CanonicalOptimizationDecision CanonicalOptimizationDecision::create_rollback_decision() const {
    CanonicalOptimizationDecision rollback;
    
    // Core Identification
    rollback.decision_id = decision_id + "_rollback";
    rollback.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    rollback.version = version;
    
    // Performance Context (same as original)
    rollback.performance_metrics = performance_metrics;
    rollback.performance_pattern = performance_pattern;
    rollback.pattern_confidence = pattern_confidence;
    
    // Decision Content (rollback strategy)
    rollback.optimization_strategy = "ROLLBACK_" + optimization_strategy;
    rollback.optimization_sequence = {rollback_strategy};
    rollback.strategy_weights = {{"rollback": 1.0}};
    rollback.decision_confidence = decision_confidence;
    
    // Policy Compliance
    rollback.applicable_policies = applicable_policies;
    rollback.policy_compliance = policy_compliance;
    rollback.policy_rationale = "Rollback of decision: " + decision_id;
    
    // Execution Parameters
    rollback.execution_parameters = execution_parameters;
    rollback.execution_mode = "rollback";
    rollback.expected_duration = std::chrono::milliseconds(5000); // 5 seconds rollback
    
    // Deterministic Guarantees
    rollback.deterministic_hash = compute_content_hash("ROLLBACK_" + deterministic_hash);
    rollback.preconditions = {"original_decision_applied: " + decision_id};
    rollback.expected_outcomes = {"system_restored_to_pre_optimization_state"};
    rollback.rollback_strategy = "N/A"; // Rollback of rollback not supported
    
    // Provenance & Audit
    rollback.model_version = model_version;
    rollback.training_data_hash = training_data_hash;
    rollback.feature_importance = feature_importance;
    rollback.reasoning_chain = "Rollback initiated due to: " + reasoning_chain;
    
    return rollback;
}

// Implementation of PolicyBoundOptimizationEngine
PolicyBoundOptimizationEngine::PolicyBoundOptimizationEngine() 
    : current_model_version_("deep_learning_v1.0") {
    
    // Initialize policy rules
    policy_rules_["parallel_processing"] = {
        "max_concurrent_operations <= 10",
        "cpu_utilization <= 0.9",
        "memory_usage_mb <= 512"
    };
    
    policy_rules_["async_operations"] = {
        "max_async_queue_size <= 1000",
        "async_timeout_ms <= 30000",
        "async_completion_rate >= 0.95"
    };
    
    policy_rules_["memory_pool_optimization"] = {
        "pool_size_mb <= 256",
        "allocation_alignment_bytes = 64",
        "pool_fragmentation_rate <= 0.1"
    };
    
    policy_rules_["policy_caching"] = {
        "cache_size_mb <= 128",
        "cache_ttl_seconds <= 3600",
        "cache_hit_rate >= 0.8"
    };
    
    policy_rules_["evidence_log_rotation"] = {
        "max_log_size_mb <= 1024",
        "rotation_interval_hours >= 24",
        "retention_days <= 30"
    };
}

CanonicalOptimizationDecision PolicyBoundOptimizationEngine::generate_canonical_decision(
    const std::map<std::string, double>& current_metrics,
    const std::string& performance_pattern,
    const std::map<std::string, double>& neural_outputs) {
    
    CanonicalOptimizationDecision decision;
    
    // Core Identification
    auto now = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    decision.decision_id = "opt_" + std::to_string(timestamp_ms);
    decision.timestamp = std::to_string(timestamp_ms);
    decision.version = "1.0";
    
    // Performance Context
    decision.performance_metrics = current_metrics;
    decision.performance_pattern = performance_pattern;
    
    // Find best neural network output
    std::string best_strategy;
    double max_confidence = 0.0;
    for (const auto& [strategy, confidence] : neural_outputs) {
        if (confidence > max_confidence) {
            max_confidence = confidence;
            best_strategy = strategy;
        }
    }
    
    decision.pattern_confidence = max_confidence;
    
    // Decision Content
    decision.optimization_strategy = best_strategy;
    decision.strategy_weights = neural_outputs;
    decision.decision_confidence = max_confidence;
    
    // Generate optimization sequence
    if (best_strategy == "parallel_processing") {
        decision.optimization_sequence = {
            "enable_parallel_processing",
            "configure_worker_threads",
            "validate_concurrent_safety"
        };
    } else if (best_strategy == "async_operations") {
        decision.optimization_sequence = {
            "enable_async_mode",
            "configure_async_queue",
            "set_async_timeout"
        };
    } else if (best_strategy == "memory_pool_optimization") {
        decision.optimization_sequence = {
            "initialize_memory_pool",
            "configure_pool_size",
            "enable_pool_monitoring"
        };
    } else if (best_strategy == "policy_caching") {
        decision.optimization_sequence = {
            "enable_policy_cache",
            "configure_cache_size",
            "set_cache_ttl"
        };
    } else {
        decision.optimization_sequence = {
            "rotate_evidence_log",
            "cleanup_old_entries",
            "compact_log_storage"
        };
    }
    
    // Policy Compliance
    decision.applicable_policies = get_violated_policies(decision);
    
    for (const auto& policy : decision.applicable_policies) {
        decision.policy_compliance[policy] = validate_policy_compliance(decision);
    }
    
    decision.policy_rationale = "Strategy " + best_strategy + 
                              " selected based on neural network confidence " + 
                              std::to_string(max_confidence);
    
    // Execution Parameters
    decision.execution_parameters = {
        {"max_duration_ms", "30000"},
        {"retry_count", "3"},
        {"monitoring_interval_ms", "1000"}
    };
    
    decision.execution_mode = "deterministic";
    decision.expected_duration = std::chrono::milliseconds(10000); // 10 seconds
    
    // Deterministic Guarantees
    std::string decision_content = decision.to_canonical_json();
    decision.deterministic_hash = compute_content_hash(decision_content);
    
    decision.preconditions = {
        "system_stable",
        "sufficient_resources",
        "policy_compliance_verified"
    };
    
    decision.expected_outcomes = {
        "optimization_applied",
        "performance_improved",
        "policy_compliance_maintained"
    };
    
    decision.rollback_strategy = "reverse_optimization_sequence";
    
    // Provenance & Audit
    decision.model_version = current_model_version_;
    decision.training_data_hash = compute_content_hash("training_data_v1.0");
    
    decision.feature_importance = {
        "throughput:0.35",
        "latency:0.40",
        "memory:0.15",
        "policy_denial_rate:0.10"
    };
    
    decision.reasoning_chain = "Neural network analysis identified " + 
                              performance_pattern + 
                              " with confidence " + 
                              std::to_string(max_confidence) + 
                              ", recommending " + best_strategy;
    
    return decision;
}

bool PolicyBoundOptimizationEngine::validate_policy_compliance(
    const CanonicalOptimizationDecision& decision) {
    
    const std::string& strategy = decision.optimization_strategy;
    
    if (policy_rules_.find(strategy) == policy_rules_.end()) {
        return false; // Unknown strategy
    }
    
    const auto& rules = policy_rules_.at(strategy);
    const auto& metrics = decision.performance_metrics;
    
    // Check each policy rule
    for (const std::string& rule : rules) {
        // Simple rule evaluation (in production, this would be more sophisticated)
        if (rule.find("cpu_utilization") != std::string::npos) {
            double cpu_usage = metrics.at("cpu_utilization");
            if (cpu_usage > 0.9) return false;
        }
        
        if (rule.find("memory_usage_mb") != std::string::npos) {
            double memory_usage = metrics.at("memory_usage_mb");
            if (memory_usage > 512) return false;
        }
        
        if (rule.find("max_concurrent_operations") != std::string::npos) {
            double throughput = metrics.at("throughput_ops_per_sec");
            if (throughput > 10) return false;
        }
    }
    
    return true;
}

std::vector<std::string> PolicyBoundOptimizationEngine::get_violated_policies(
    const CanonicalOptimizationDecision& decision) {
    
    std::vector<std::string> violated_policies;
    
    // Check all applicable policies
    for (const auto& [strategy, rules] : policy_rules_) {
        bool compliant = true;
        
        for (const std::string& rule : rules) {
            if (!evaluate_policy_rule(rule, decision.performance_metrics)) {
                compliant = false;
                break;
            }
        }
        
        if (!compliant) {
            violated_policies.push_back(strategy);
        }
    }
    
    return violated_policies;
}

bool PolicyBoundOptimizationEngine::evaluate_policy_rule(
    const std::string& rule,
    const std::map<std::string, double>& metrics) {
    
    // Simple rule evaluation (would be more sophisticated in production)
    if (rule.find("cpu_utilization") != std::string::npos) {
        double cpu_usage = metrics.at("cpu_utilization");
        return cpu_usage <= 0.9;
    }
    
    if (rule.find("memory_usage_mb") != std::string::npos) {
        double memory_usage = metrics.at("memory_usage_mb");
        return memory_usage <= 512;
    }
    
    if (rule.find("max_concurrent_operations") != std::string::npos) {
        double throughput = metrics.at("throughput_ops_per_sec");
        return throughput <= 10;
    }
    
    return true; // Default to compliant
}

std::string PolicyBoundOptimizationEngine::compute_deterministic_hash(
    const CanonicalOptimizationDecision& decision) {
    
    std::string decision_content = decision.to_canonical_json();
    return compute_content_hash(decision_content);
}

std::string PolicyBoundOptimizationEngine::compute_content_hash(const std::string& content) {
    // Compute SHA-256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(content.c_str()), 
           content.length(), hash);
    
    // Convert to hex string
    std::ostringstream hash_stream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hash_stream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return hash_stream.str();
}

bool PolicyBoundOptimizationEngine::verify_deterministic_replay(
    const CanonicalOptimizationDecision& decision) {
    
    // Verify decision structure
    if (!decision.validate_canonical_form()) {
        return false;
    }
    
    // Recompute hash and verify
    std::string recomputed_hash = compute_deterministic_hash(decision);
    return recomputed_hash == decision.deterministic_hash;
}

std::string PolicyBoundOptimizationEngine::serialize_decision(
    const CanonicalOptimizationDecision& decision) {
    
    return decision.to_canonical_json();
}

CanonicalOptimizationDecision PolicyBoundOptimizationEngine::deserialize_decision(
    const std::string& serialized) {
    
    try {
        auto json = nlohmann::json::parse(serialized);
        CanonicalOptimizationDecision decision;
        
        // Parse all fields
        decision.decision_id = json["decision_id"];
        decision.timestamp = json["timestamp"];
        decision.version = json["version"];
        
        decision.performance_metrics = json["performance_context"]["metrics"];
        decision.performance_pattern = json["performance_context"]["pattern"];
        decision.pattern_confidence = json["performance_context"]["confidence"];
        
        decision.optimization_strategy = json["decision"]["strategy"];
        decision.optimization_sequence = json["decision"]["sequence"];
        decision.strategy_weights = json["decision"]["weights"];
        decision.decision_confidence = json["decision"]["confidence"];
        
        decision.applicable_policies = json["policy"]["applicable"];
        decision.policy_compliance = json["policy"]["compliance"];
        decision.policy_rationale = json["policy"]["rationale"];
        
        decision.execution_parameters = json["execution"]["parameters"];
        decision.execution_mode = json["execution"]["mode"];
        decision.expected_duration = std::chrono::milliseconds(
            json["execution"]["expected_duration_ms"]);
        
        decision.deterministic_hash = json["deterministic"]["hash"];
        decision.preconditions = json["deterministic"]["preconditions"];
        decision.expected_outcomes = json["deterministic"]["expected_outcomes"];
        decision.rollback_strategy = json["deterministic"]["rollback_strategy"];
        
        decision.model_version = json["provenance"]["model_version"];
        decision.training_data_hash = json["provenance"]["training_data_hash"];
        decision.feature_importance = json["provenance"]["feature_importance"];
        decision.reasoning_chain = json["provenance"]["reasoning_chain"];
        
        return decision;
        
    } catch (const std::exception& e) {
        // Return empty decision on parse error
        return CanonicalOptimizationDecision{};
    }
}

} // namespace t81::canonfs
