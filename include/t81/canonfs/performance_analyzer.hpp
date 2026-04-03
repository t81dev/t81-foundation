#pragma once

#include "t81/monitoring/performance_monitor.hpp"
#include "t81/canonfs/interchange_engine.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

namespace t81::canonfs {

// Performance analysis results
struct PerformanceAnalysis {
    std::string timestamp;
    std::map<std::string, double> metrics;
    std::vector<std::string> insights;
    std::string summary;
};

class PerformanceAnalyzer {
public:
    PerformanceAnalyzer(std::shared_ptr<t81::monitoring::PerformanceCollector> collector);
    ~PerformanceAnalyzer();
    
    // Real-time analysis
    PerformanceAnalysis analyze_current_performance();
    std::vector<PerformanceAnalysis> get_historical_analysis(size_t hours = 24);
    
    // Performance optimization suggestions
    std::vector<std::string> get_optimization_suggestions();
    
    // Performance alerts
    std::vector<std::string> get_performance_alerts();
    
    // Generate comprehensive report
    std::string generate_performance_report();

private:
    std::shared_ptr<t81::monitoring::PerformanceCollector> collector_;
    std::vector<PerformanceAnalysis> analysis_history_;
    
    // Analysis methods
    double calculate_operation_throughput();
    double calculate_policy_denial_rate();
    double calculate_average_operation_time();
    std::vector<std::string> detect_performance_anomalies();
    std::string identify_performance_bottlenecks();
    std::vector<std::string> generate_optimization_recommendations();
};

// Performance alert thresholds
struct PerformanceThresholds {
    double max_operation_time_ms = 1000.0;      // 1 second
    double max_policy_denial_rate = 0.1;          // 10%
    double min_throughput_ops_per_sec = 1.0;       // 1 op/sec
    size_t max_evidence_log_size = 5000;              // 5000 entries
    double max_memory_usage_mb = 100.0;              // 100MB
};

// Performance optimization strategies
struct OptimizationStrategy {
    std::string name;
    std::string description;
    double expected_improvement;  // percentage
    std::string implementation_complexity;  // low, medium, high
};

} // namespace t81::canonfs
