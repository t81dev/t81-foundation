#include "t81/canonfs/canonfs_performance_monitor.hpp"
#include "t81/canonfs/interchange_engine.hpp"
#include <sstream>
#include "t81/canonfs/interchange_ops.hpp"

namespace t81::canonfs {

CanonFSPerformanceMonitor::CanonFSPerformanceMonitor() 
    : collector_(nullptr) {}

CanonFSPerformanceMonitor::~CanonFSPerformanceMonitor() {
    if (collector_) {
        // Record final metrics summary
        collector_->collect_metric("canonfs_final_summary", static_cast<double>(0.0));
    }
}

void CanonFSPerformanceMonitor::initialize_monitoring(CanonFSInterchangeEngine* engine) {
    // Initialize the performance collector
    collector_ = new t81::monitoring::PerformanceCollector();
    
    // Register CanonFS-specific metrics
    register_canonfs_metrics();
    
    // Start resource monitoring
    collector_->start_resource_monitoring();
}

void CanonFSPerformanceMonitor::register_canonfs_metrics() {
    using namespace t81::monitoring;
    
    // Import/Export operation counters
    collector_->register_metric(MetricDefinition{
        "canonfs_import_operations",
        "Total number of CanonFS import operations"
    });
    
    collector_->register_metric(MetricDefinition{
        "canonfs_export_operations", 
        "Total number of CanonFS export operations"
    });
    
    // Policy evaluation metrics
    collector_->register_metric(MetricDefinition{
        "canonfs_policy_evaluations",
        "Total number of policy evaluations performed"
    });
    
    collector_->register_metric(MetricDefinition{
        "canonfs_policy_denials",
        "Number of policy decisions that denied access"
    });
    
    // Performance metrics
    collector_->register_metric(MetricDefinition{
        "canonfs_avg_operation_time_ms",
        "Average CanonFS operation duration in milliseconds"
    });
    
    collector_->register_metric(MetricDefinition{
        "canonfs_evidence_log_size",
        "Current number of entries in evidence log"
    });
}

void CanonFSPerformanceMonitor::record_import_operation(const std::string& path, bool success, 
                                               std::chrono::milliseconds duration) {
    if (!collector_) return;
    
    import_operations_++;
    total_operation_time_ms_ += duration.count();
    operation_count_++;
    
    // Record detailed operation data
    collector_->increment_counter("canonfs_import_operations", 1.0);
    collector_->collect_metric("canonfs_import_duration_ms", static_cast<double>(duration.count()));
    
    // Record operation outcome
    std::string outcome = success ? "success" : "failed";
    collector_->increment_counter("canonfs_import_outcome_" + outcome, 1.0);
}

void CanonFSPerformanceMonitor::record_export_operation(const std::string& hash, bool success,
                                               std::chrono::milliseconds duration) {
    if (!collector_) return;
    
    export_operations_++;
    total_operation_time_ms_ += duration.count();
    operation_count_++;
    
    // Record detailed operation data
    collector_->increment_counter("canonfs_export_operations", 1.0);
    collector_->collect_metric("canonfs_export_duration_ms", static_cast<double>(duration.count()));
    
    // Record operation outcome
    std::string outcome = success ? "success" : "failed";
    collector_->increment_counter("canonfs_export_outcome_" + outcome, 1.0);
}

void CanonFSPerformanceMonitor::record_policy_evaluation(const std::string& operation, bool allowed,
                                                   const std::string& reason) {
    if (!collector_) return;
    
    policy_evaluations_++;
    
    // Record policy evaluation
    collector_->increment_counter("canonfs_policy_evaluations", 1.0);
    
    if (!allowed) {
        policy_denials_++;
        collector_->increment_counter("canonfs_policy_denials", 1.0);
    }
    
    // Record policy decision details
    std::string decision = allowed ? "allow" : "deny";
    collector_->increment_counter("canonfs_policy_decision_" + decision, 1.0);
    collector_->increment_counter("canonfs_policy_reason_" + reason, 1.0);
}

void CanonFSPerformanceMonitor::record_evidence_log_size(size_t entry_count) {
    if (!collector_) return;
    
    // Record evidence log metrics
    collector_->collect_metric("canonfs_evidence_log_size", static_cast<double>(entry_count));
    
    // Record memory usage estimate (rough calculation)
    size_t estimated_memory_kb = entry_count * sizeof(OperationContext) / 1024;
    collector_->collect_metric("canonfs_memory_usage_kb", static_cast<double>(estimated_memory_kb));
}

std::string CanonFSPerformanceMonitor::get_performance_summary() const {
    std::ostringstream summary;
    
    uint64_t imports = import_operations_.load();
    uint64_t exports = export_operations_.load();
    uint64_t policy_evals = policy_evaluations_.load();
    uint64_t denials = policy_denials_.load();
    uint64_t total_time = total_operation_time_ms_.load();
    uint64_t ops_count = operation_count_.load();
    
    summary << "=== CanonFS Performance Summary ===\n";
    summary << "Import Operations: " << imports << "\n";
    summary << "Export Operations: " << exports << "\n";
    summary << "Policy Evaluations: " << policy_evals << "\n";
    summary << "Policy Denials: " << denials << "\n";
    
    if (ops_count > 0) {
        uint64_t avg_time = total_time / ops_count;
        summary << "Average Operation Time: " << avg_time << "ms\n";
    }
    
    if (policy_evals > 0) {
        double denial_rate = (static_cast<double>(denials) / policy_evals) * 100.0;
        summary << "Policy Denial Rate: " << std::fixed << std::setprecision(2) << denial_rate << "%\n";
    }
    
    summary << "================================\n";
    return summary.str();
}

} // namespace t81::canonfs
