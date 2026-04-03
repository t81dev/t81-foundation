#include "t81/canonfs/auto_optimizer.hpp"
#include <thread>
#include <algorithm>
#include <sstream>

namespace t81::canonfs {

CanonFSAutoOptimizer::CanonFSAutoOptimizer(std::shared_ptr<PerformanceAnalyzer> analyzer)
    : analyzer_(analyzer) {
    initialize_optimization_strategies();
}

CanonFSAutoOptimizer::~CanonFSAutoOptimizer() = default;

void CanonFSAutoOptimizer::enable_auto_optimization(bool enable) {
    auto_optimization_enabled_ = enable;
    if (enable) {
        // Start auto-optimization loop in background
        std::thread([this]() { auto_optimization_loop(); }).detach();
    }
}

void CanonFSAutoOptimizer::set_optimization_interval(std::chrono::seconds interval) {
    optimization_interval_ = interval;
}

void CanonFSAutoOptimizer::add_custom_strategy(OptimizationAction action) {
    available_strategies_.push_back(action);
}

std::vector<OptimizationAction> CanonFSAutoOptimizer::get_recommended_optimizations() {
    auto current_metrics = analyzer_->analyze_current_performance();
    std::vector<OptimizationAction> recommendations;
    
    // Analyze current performance and recommend optimizations
    for (const auto& strategy : available_strategies_) {
        if (should_apply_optimization(strategy)) {
            recommendations.push_back(strategy);
        }
    }
    
    // Sort by expected improvement (highest first)
    std::sort(recommendations.begin(), recommendations.end(),
              [](const OptimizationAction& a, const OptimizationAction& b) {
                  return a.expected_improvement > b.expected_improvement;
              });
    
    return recommendations;
}

bool CanonFSAutoOptimizer::apply_optimization(OptimizationStrategy strategy) {
    auto it = std::find_if(available_strategies_.begin(), available_strategies_.end(),
                           [strategy](const OptimizationAction& action) {
                               return action.strategy == strategy;
                           });
    
    if (it != available_strategies_.end()) {
        std::cout << "🔧 Applying optimization: " << it->description << "\n";
        
        if (it->apply()) {
            it->is_applied = true;
            it->applied_time = std::chrono::steady_clock::now();
            applied_optimizations_.push_back(*it);
            
            std::cout << "✅ Optimization applied successfully\n";
            return true;
        } else {
            std::cout << "❌ Failed to apply optimization\n";
            return false;
        }
    }
    
    std::cout << "❌ Optimization strategy not found\n";
    return false;
}

bool CanonFSAutoOptimizer::rollback_optimization(OptimizationStrategy strategy) {
    auto it = std::find_if(applied_optimizations_.begin(), applied_optimizations_.end(),
                           [strategy](const OptimizationAction& action) {
                               return action.strategy == strategy;
                           });
    
    if (it != applied_optimizations_.end()) {
        std::cout << "🔄 Rolling back optimization: " << it->description << "\n";
        
        if (it->rollback()) {
            it->is_applied = false;
            std::cout << "✅ Optimization rolled back successfully\n";
            return true;
        } else {
            std::cout << "❌ Failed to rollback optimization\n";
            return false;
        }
    }
    
    std::cout << "❌ Applied optimization not found for rollback\n";
    return false;
}

std::string CanonFSAutoOptimizer::get_optimization_status() {
    std::ostringstream status;
    
    status << "🚀 CanonFS Auto-Optimization Status\n";
    status << "=====================================\n\n";
    
    status << "Auto-optimization: " << (auto_optimization_enabled_ ? "✅ ENABLED" : "❌ DISABLED") << "\n";
    status << "Check interval: " << optimization_interval_.count() << " seconds\n";
    status << "Last optimization: " << std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - last_optimization_).count() << " seconds ago\n\n";
    
    status << "Applied Optimizations (" << applied_optimizations_.size() << "):\n";
    for (size_t i = 0; i < applied_optimizations_.size(); ++i) {
        const auto& opt = applied_optimizations_[i];
        status << "  " << (i + 1) << ". " << opt.description;
        status << " [" << (opt.is_applied ? "✅ ACTIVE" : "❌ INACTIVE") << "]\n";
    }
    
    return status.str();
}

std::vector<OptimizationAction> CanonFSAutoOptimizer::get_applied_optimizations() {
    return applied_optimizations_;
}

std::string CanonFSAutoOptimizer::generate_optimization_report() {
    std::ostringstream report;
    
    report << "=== CanonFS Auto-Optimization Report ===\n\n";
    
    auto current_metrics = analyzer_->analyze_current_performance();
    report << "Current Performance Status:\n";
    report << "- Throughput: " << current_metrics.metrics.at("operations_per_second") << " ops/sec\n";
    report << "- Avg Latency: " << current_metrics.metrics.at("average_operation_time_ms") << " ms\n";
    report << "- Memory Usage: " << current_metrics.metrics.at("memory_usage_mb") << " MB\n";
    report << "- Policy Denial Rate: " << current_metrics.metrics.at("policy_denial_rate_percent") << "%\n\n";
    
    report << "Optimization History:\n";
    for (const auto& opt : applied_optimizations_) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - opt.applied_time);
        
        report << "- " << opt.description << "\n";
        report << "  Applied: " << std::chrono::duration_cast<std::chrono::minutes>(duration).count() << " minutes ago\n";
        report << "  Expected Improvement: " << opt.expected_improvement << "%\n";
        report << "  Status: " << (opt.is_applied ? "✅ Active" : "❌ Inactive") << "\n\n";
    }
    
    auto recommendations = get_recommended_optimizations();
    if (!recommendations.empty()) {
        report << "Recommended Optimizations:\n";
        for (const auto& rec : recommendations) {
            report << "- " << rec.description << "\n";
            report << "  Expected Improvement: " << rec.expected_improvement << "%\n";
            report << "  Complexity: " << rec.complexity << "\n\n";
        }
    }
    
    return report.str();
}

void CanonFSAutoOptimizer::initialize_optimization_strategies() {
    // Evidence Log Rotation
    available_strategies_.push_back({
        OptimizationStrategy::EVIDENCE_LOG_ROTATION,
        "Evidence Log Rotation - Automatic cleanup of old evidence entries",
        [this]() { return apply_evidence_log_rotation(); },
        [this]() { return rollback_evidence_log_rotation(); },
        25.0,
        "low"
    });
    
    // Policy Decision Caching
    available_strategies_.push_back({
        OptimizationStrategy::POLICY_DECISION_CACHING,
        "Policy Decision Caching - Cache frequently used policy decisions",
        [this]() { return apply_policy_decision_caching(); },
        [this]() { return rollback_policy_decision_caching(); },
        15.0,
        "medium"
    });
    
    // Parallel Processing
    available_strategies_.push_back({
        OptimizationStrategy::PARALLEL_PROCESSING,
        "Parallel Processing - Enable concurrent CanonFS operations",
        [this]() { return apply_parallel_processing(); },
        [this]() { return rollback_parallel_processing(); },
        40.0,
        "high"
    });
    
    // Memory Pool Management
    available_strategies_.push_back({
        OptimizationStrategy::MEMORY_POOL_MANAGEMENT,
        "Memory Pool Management - Optimize memory allocation patterns",
        [this]() { return apply_memory_pool_management(); },
        [this]() { return rollback_memory_pool_management(); },
        20.0,
        "medium"
    });
    
    // Asynchronous Operations
    available_strategies_.push_back({
        OptimizationStrategy::ASYNCHRONOUS_OPERATIONS,
        "Asynchronous Operations - Enable non-blocking operations",
        [this]() { return apply_asynchronous_operations(); },
        [this]() { return rollback_asynchronous_operations(); },
        30.0,
        "high"
    });
    
    // Adaptive Throttling
    available_strategies_.push_back({
        OptimizationStrategy::ADAPTIVE_THROTTLING,
        "Adaptive Throttling - Dynamic rate limiting based on load",
        [this]() { return apply_adaptive_throttling(); },
        [this]() { return rollback_adaptive_throttling(); },
        35.0,
        "medium"
    });
    
    // Predictive Caching
    available_strategies_.push_back({
        OptimizationStrategy::PREDICTIVE_CACHING,
        "Predictive Caching - Pre-cache likely operations",
        [this]() { return apply_predictive_caching(); },
        [this]() { return rollback_predictive_caching(); },
        45.0,
        "high"
    });
    
    // Bulk Operations
    available_strategies_.push_back({
        OptimizationStrategy::BULK_OPERATIONS,
        "Bulk Operations - Batch processing for efficiency",
        [this]() { return apply_bulk_operations(); },
        [this]() { return rollback_bulk_operations(); },
        50.0,
        "high"
    });
}

bool CanonFSAutoOptimizer::should_apply_optimization(const OptimizationAction& action) {
    auto current_metrics = analyzer_->analyze_current_performance();
    
    // Check if optimization should be applied based on current metrics
    switch (action.strategy) {
        case OptimizationStrategy::EVIDENCE_LOG_ROTATION:
            return current_metrics.metrics.at("evidence_log_size") > 3000;
            
        case OptimizationStrategy::POLICY_DECISION_CACHING:
            return current_metrics.metrics.at("policy_denial_rate_percent") > 5.0;
            
        case OptimizationStrategy::PARALLEL_PROCESSING:
            return current_metrics.metrics.at("operations_per_second") < 2.0;
            
        case OptimizationStrategy::MEMORY_POOL_MANAGEMENT:
            return current_metrics.metrics.at("memory_usage_mb") > 80.0;
            
        case OptimizationStrategy::ASYNCHRONOUS_OPERATIONS:
            return current_metrics.metrics.at("average_operation_time_ms") > 300.0;
            
        case OptimizationStrategy::ADAPTIVE_THROTTLING:
            return current_metrics.metrics.at("operations_per_second") > 5.0;
            
        case OptimizationStrategy::PREDICTIVE_CACHING:
            return current_metrics.metrics.at("policy_denial_rate_percent") > 3.0;
            
        case OptimizationStrategy::BULK_OPERATIONS:
            return current_metrics.metrics.at("operations_per_second") > 1.0;
    }
    
    return false;
}

void CanonFSAutoOptimizer::auto_optimization_loop() {
    while (auto_optimization_enabled_) {
        std::this_thread::sleep_for(optimization_interval_);
        
        auto recommendations = get_recommended_optimizations();
        
        for (const auto& recommendation : recommendations) {
            if (should_apply_optimization(recommendation)) {
                std::cout << "🤖 Auto-applying optimization: " << recommendation.description << "\n";
                apply_optimization(recommendation.strategy);
                
                // Wait a bit to measure effect
                std::this_thread::sleep_for(std::chrono::seconds(30));
                
                if (measure_optimization_effectiveness(recommendation)) {
                    std::cout << "✅ Auto-optimization effective\n";
                } else {
                    std::cout << "⚠️ Auto-optimization ineffective, rolling back\n";
                    rollback_optimization(recommendation.strategy);
                }
                break;
            }
        }
        
        last_optimization_ = std::chrono::steady_clock::now();
    }
}

bool CanonFSAutoOptimizer::measure_optimization_effectiveness(const OptimizationAction& action) {
    // Measure performance before and after optimization
    auto before_metrics = analyzer_->analyze_current_performance();
    
    // Wait a bit to collect new metrics
    std::this_thread::sleep_for(std::chrono::seconds(10));
    auto after_metrics = analyzer_->analyze_current_performance();
    
    // Simple effectiveness check
    switch (action.strategy) {
        case OptimizationStrategy::EVIDENCE_LOG_ROTATION:
            return after_metrics.metrics.at("memory_usage_mb") < before_metrics.metrics.at("memory_usage_mb");
            
        case OptimizationStrategy::POLICY_DECISION_CACHING:
            return after_metrics.metrics.at("average_operation_time_ms") < before_metrics.metrics.at("average_operation_time_ms");
            
        case OptimizationStrategy::PARALLEL_PROCESSING:
            return after_metrics.metrics.at("operations_per_second") > before_metrics.metrics.at("operations_per_second");
            
        case OptimizationStrategy::MEMORY_POOL_MANAGEMENT:
            return after_metrics.metrics.at("memory_usage_mb") < before_metrics.metrics.at("memory_usage_mb");
            
        case OptimizationStrategy::ASYNCHRONOUS_OPERATIONS:
            return after_metrics.metrics.at("average_operation_time_ms") < before_metrics.metrics.at("average_operation_time_ms");
            
        default:
            return true; // Assume effective for other optimizations
    }
}

// Optimization implementation stubs (these would integrate with actual CanonFS engine)
bool CanonFSAutoOptimizer::apply_evidence_log_rotation() {
    std::cout << "🔄 Implementing evidence log rotation...\n";
    // This would integrate with actual CanonFS evidence log cleanup
    return true;
}

bool CanonFSAutoOptimizer::rollback_evidence_log_rotation() {
    std::cout << "🔄 Rolling back evidence log rotation...\n";
    return true;
}

bool CanonFSAutoOptimizer::apply_policy_decision_caching() {
    std::cout << "🧠 Implementing policy decision caching...\n";
    // This would integrate with actual CanonFS policy caching
    return true;
}

bool CanonFSAutoOptimizer::rollback_policy_decision_caching() {
    std::cout << "🧠 Rolling back policy decision caching...\n";
    return true;
}

bool CanonFSAutoOptimizer::apply_parallel_processing() {
    std::cout << "⚡ Enabling parallel processing...\n";
    // This would integrate with actual CanonFS parallel operations
    return true;
}

bool CanonFSAutoOptimizer::rollback_parallel_processing() {
    std::cout << "⚡ Rolling back parallel processing...\n";
    return true;
}

bool CanonFSAutoOptimizer::apply_memory_pool_management() {
    std::cout << "💾 Implementing memory pool management...\n";
    // This would integrate with actual CanonFS memory optimization
    return true;
}

bool CanonFSAutoOptimizer::rollback_memory_pool_management() {
    std::cout << "💾 Rolling back memory pool management...\n";
    return true;
}

bool CanonFSAutoOptimizer::apply_asynchronous_operations() {
    std::cout << "🔄 Enabling asynchronous operations...\n";
    // This would integrate with actual CanonFS async operations
    return true;
}

bool CanonFSAutoOptimizer::rollback_asynchronous_operations() {
    std::cout << "🔄 Rolling back asynchronous operations...\n";
    return true;
}

bool CanonFSAutoOptimizer::apply_adaptive_throttling() {
    std::cout << "🎛 Implementing adaptive throttling...\n";
    // This would integrate with actual CanonFS rate limiting
    return true;
}

bool CanonFSAutoOptimizer::rollback_adaptive_throttling() {
    std::cout << "🎛 Rolling back adaptive throttling...\n";
    return true;
}

bool CanonFSAutoOptimizer::apply_predictive_caching() {
    std::cout << "🔮 Implementing predictive caching...\n";
    // This would integrate with actual CanonFS predictive caching
    return true;
}

bool CanonFSAutoOptimizer::rollback_predictive_caching() {
    std::cout << "🔮 Rolling back predictive caching...\n";
    return true;
}

bool CanonFSAutoOptimizer::apply_bulk_operations() {
    std::cout << "📦 Enabling bulk operations...\n";
    // This would integrate with actual CanonFS batch processing
    return true;
}

bool CanonFSAutoOptimizer::rollback_bulk_operations() {
    std::cout << "📦 Rolling back bulk operations...\n";
    return true;
}

} // namespace t81::canonfs
