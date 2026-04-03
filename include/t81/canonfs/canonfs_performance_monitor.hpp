#pragma once

#include "t81/monitoring/performance_monitor.hpp"
#include <chrono>

namespace t81::canonfs {

// CanonFS-specific performance metrics
enum class CanonFSMetricType {
    IMPORT_OPERATIONS,      // Number of import operations
    EXPORT_OPERATIONS,      // Number of export operations  
    POLICY_EVALUATIONS,   // Number of policy evaluations
    EVIDENCE_ENTRIES,      // Number of evidence log entries
    POLICY_DENIALS,       // Number of denied policy decisions
    AVERAGE_OPERATION_TIME, // Average operation duration
    MEMORY_USAGE           // Current memory usage of evidence log
};

// CanonFS performance monitoring integration
class CanonFSPerformanceMonitor {
public:
    CanonFSPerformanceMonitor();
    ~CanonFSPerformanceMonitor();
    
    // Initialize monitoring for a CanonFS engine
    void initialize_monitoring(CanonFSInterchangeEngine* engine);
    
    // Record specific CanonFS metrics
    void record_import_operation(const std::string& path, bool success, 
                           std::chrono::milliseconds duration);
    void record_export_operation(const std::string& hash, bool success,
                           std::chrono::milliseconds duration);
    void record_policy_evaluation(const std::string& operation, bool allowed,
                             const std::string& reason);
    void record_evidence_log_size(size_t entry_count);
    
    // Get performance summary
    std::string get_performance_summary() const;
    
private:
    t81::monitoring::PerformanceCollector* collector_;
    std::atomic<uint64_t> import_operations_{0};
    std::atomic<uint64_t> export_operations_{0};
    std::atomic<uint64_t> policy_evaluations_{0};
    std::atomic<uint64_t> policy_denials_{0};
    std::atomic<uint64_t> total_operation_time_ms_{0};
    std::atomic<uint64_t> operation_count_{0};
    
    void register_canonfs_metrics();
};

} // namespace t81::canonfs
