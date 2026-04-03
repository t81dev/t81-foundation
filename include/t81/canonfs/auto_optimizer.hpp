#pragma once

#include "t81/canonfs/performance_analyzer.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <vector>
#include <functional>

namespace t81::canonfs {

// Optimization strategies
enum class OptimizationStrategy {
    EVIDENCE_LOG_ROTATION,    // Automatic evidence log cleanup
    POLICY_DECISION_CACHING,   // Cache policy decisions
    PARALLEL_PROCESSING,        // Enable concurrent operations
    MEMORY_POOL_MANAGEMENT,     // Optimize memory allocation
    ASYNCHRONOUS_OPERATIONS,   // Non-blocking operations
    ADAPTIVE_THROTTLING,      // Dynamic rate limiting
    PREDICTIVE_CACHING,       // Pre-cache likely operations
    BULK_OPERATIONS           // Batch processing optimization
};

// Optimization action
struct OptimizationAction {
    OptimizationStrategy strategy;
    std::string description;
    std::function<bool()> apply;
    std::function<bool()> rollback;
    double expected_improvement;
    std::string complexity;
    bool is_applied = false;
    std::chrono::steady_clock::time_point applied_time;
};

// Auto-optimization engine
class CanonFSAutoOptimizer {
public:
    CanonFSAutoOptimizer(std::shared_ptr<PerformanceAnalyzer> analyzer);
    ~CanonFSAutoOptimizer();
    
    // Auto-optimization control
    void enable_auto_optimization(bool enable = true);
    void set_optimization_interval(std::chrono::seconds interval);
    void add_custom_strategy(OptimizationAction action);
    
    // Manual optimization
    std::vector<OptimizationAction> get_recommended_optimizations();
    bool apply_optimization(OptimizationStrategy strategy);
    bool rollback_optimization(OptimizationStrategy strategy);
    
    // Auto-optimization status
    std::string get_optimization_status();
    std::vector<OptimizationAction> get_applied_optimizations();
    std::string generate_optimization_report();

private:
    std::shared_ptr<PerformanceAnalyzer> analyzer_;
    std::vector<OptimizationAction> available_strategies_;
    std::vector<OptimizationAction> applied_optimizations_;
    std::chrono::seconds optimization_interval_{300}; // 5 minutes default
    std::chrono::steady_clock::time_point last_optimization_;
    bool auto_optimization_enabled_ = true;
    
    // Optimization strategies
    void initialize_optimization_strategies();
    bool should_apply_optimization(const OptimizationAction& action);
    bool measure_optimization_effectiveness(const OptimizationAction& action);
    void auto_optimization_loop();
    
    // Specific optimization implementations
    bool apply_evidence_log_rotation();
    bool rollback_evidence_log_rotation();
    
    bool apply_policy_decision_caching();
    bool rollback_policy_decision_caching();
    
    bool apply_parallel_processing();
    bool rollback_parallel_processing();
    
    bool apply_memory_pool_management();
    bool rollback_memory_pool_management();
    
    bool apply_asynchronous_operations();
    bool rollback_asynchronous_operations();
    
    bool apply_adaptive_throttling();
    bool rollback_adaptive_throttling();
    
    bool apply_predictive_caching();
    bool rollback_predictive_caching();
    
    bool apply_bulk_operations();
    bool rollback_bulk_operations();
};

// Optimization thresholds
struct OptimizationThresholds {
    double high_latency_ms = 500.0;
    double low_throughput_ops_per_sec = 1.0;
    double high_memory_usage_mb = 100.0;
    size_t large_evidence_log = 5000;
    double high_policy_denial_rate = 0.1; // 10%
};

} // namespace t81::canonfs
