#include "t81/canonfs/performance_analyzer.hpp"
#include <sstream>
#include <numeric>

namespace t81::canonfs {

PerformanceAnalyzer::PerformanceAnalyzer(std::shared_ptr<PerformanceCollector> collector)
    : collector_(collector) {
    analysis_history_.reserve(100); // Reserve space for historical data
}

PerformanceAnalyzer::~PerformanceAnalyzer() = default;

PerformanceAnalysis PerformanceAnalyzer::analyze_current_performance() {
    PerformanceAnalysis analysis;
    analysis.timestamp = get_current_timestamp();
    
    // Collect current metrics
    auto current_time = std::chrono::high_resolution_clock::now();
    
    // Calculate key performance indicators
    double throughput = calculate_operation_throughput();
    double denial_rate = calculate_policy_denial_rate();
    double avg_time = calculate_average_operation_time();
    
    // Store metrics
    analysis.metrics["operations_per_second"] = throughput;
    analysis.metrics["policy_denial_rate_percent"] = denial_rate * 100.0;
    analysis.metrics["average_operation_time_ms"] = avg_time;
    analysis.metrics["evidence_log_size"] = static_cast<double>(get_current_evidence_log_size());
    analysis.metrics["memory_usage_mb"] = get_current_memory_usage();
    
    // Generate insights
    analysis.insights = detect_performance_anomalies();
    auto bottlenecks = identify_performance_bottlenecks();
    analysis.insights.insert(analysis.insights.end(), bottlenecks.begin(), bottlenecks.end());
    
    // Generate summary
    std::ostringstream summary_stream;
    summary_stream << std::fixed << std::setprecision(2);
    summary_stream << "Performance Status: ";
    
    // Overall performance rating
    if (denial_rate > 0.05 || avg_time > 500 || throughput < 0.5) {
        summary_stream << "⚠️ DEGRADED";
    } else if (denial_rate > 0.02 || avg_time > 200 || throughput < 1.0) {
        summary_stream << "⚡ SUBOPTIMAL";
    } else {
        summary_stream << "✅ OPTIMAL";
    }
    
    summary_stream << " | Throughput: " << throughput << " ops/s";
    summary_stream << " | Avg Time: " << avg_time << "ms";
    summary_stream << " | Denial Rate: " << (denial_rate * 100) << "%";
    
    analysis.summary = summary_stream.str();
    
    // Store in history
    analysis_history_.push_back(analysis);
    if (analysis_history_.size() > 100) {
        analysis_history_.erase(analysis_history_.begin(), 
                              analysis_history_.begin() + 50);
    }
    
    return analysis;
}

std::vector<PerformanceAnalysis> PerformanceAnalyzer::get_historical_analysis(size_t hours) {
    std::vector<PerformanceAnalysis> recent_analysis;
    
    auto cutoff_time = std::chrono::high_resolution_clock::now() - 
                      std::chrono::hours(hours);
    
    for (const auto& analysis : analysis_history_) {
        // Parse timestamp to compare (simplified)
        if (analysis.timestamp > get_current_timestamp()) {
            recent_analysis.push_back(analysis);
        }
    }
    
    return recent_analysis;
}

std::vector<std::string> PerformanceAnalyzer::get_optimization_suggestions() {
    std::vector<std::string> suggestions;
    
    auto current_analysis = analyze_current_performance();
    double denial_rate = calculate_policy_denial_rate();
    double avg_time = calculate_average_operation_time();
    double throughput = calculate_operation_throughput();
    
    // Check against thresholds
    PerformanceThresholds thresholds;
    
    if (avg_time > thresholds.max_operation_time_ms) {
        suggestions.push_back("🔧 OPTIMIZE: Operation latency exceeds " + 
                           std::to_string(thresholds.max_operation_time_ms) + "ms. Consider optimizing policy evaluation logic.");
    }
    
    if (denial_rate > thresholds.max_policy_denial_rate) {
        suggestions.push_back("🛡️ SECURITY: High policy denial rate (" + 
                           std::to_string(static_cast<int>(denial_rate * 100)) + "%). Review policy configurations.");
    }
    
    if (throughput < thresholds.min_throughput_ops_per_sec) {
        suggestions.push_back("⚡ PERFORMANCE: Low throughput (" + 
                           std::to_string(throughput) + " ops/s). Consider parallel processing.");
    }
    
    size_t evidence_size = get_current_evidence_log_size();
    if (evidence_size > thresholds.max_evidence_log_size) {
        suggestions.push_back("💾 MEMORY: Large evidence log (" + 
                           std::to_string(evidence_size) + " entries). Implement log rotation.");
    }
    
    double memory_usage = get_current_memory_usage();
    if (memory_usage > thresholds.max_memory_usage_mb) {
        suggestions.push_back("🧠 MEMORY: High memory usage (" + 
                           std::to_string(memory_usage) + "MB). Consider evidence cleanup.");
    }
    
    // Add proactive optimization suggestions
    auto optimization_strategies = generate_optimization_recommendations();
    for (const auto& strategy : optimization_strategies) {
        suggestions.push_back("🚀 " + strategy.name + ": " + strategy.description + 
                           " (Expected improvement: " + std::to_string(strategy.expected_improvement) + "%)");
    }
    
    return suggestions;
}

std::vector<std::string> PerformanceAnalyzer::get_performance_alerts() {
    std::vector<std::string> alerts;
    
    auto analysis = analyze_current_performance();
    PerformanceThresholds thresholds;
    
    // Critical alerts
    if (analysis.metrics["policy_denial_rate_percent"] > 10.0) {
        alerts.push_back("🚨 CRITICAL: Policy denial rate exceeds 10%");
    }
    
    if (analysis.metrics["average_operation_time_ms"] > thresholds.max_operation_time_ms) {
        alerts.push_back("🚨 CRITICAL: Operation latency exceeds 1 second");
    }
    
    size_t evidence_size = static_cast<size_t>(analysis.metrics["evidence_log_size"]);
    if (evidence_size > thresholds.max_evidence_log_size * 0.8) {
        alerts.push_back("🚨 WARNING: Evidence log approaching size limit");
    }
    
    double memory_usage = get_current_memory_usage();
    if (memory_usage > thresholds.max_memory_usage_mb * 0.8) {
        alerts.push_back("🚨 WARNING: Memory usage approaching limit");
    }
    
    return alerts;
}

std::string PerformanceAnalyzer::generate_performance_report() {
    std::ostringstream report;
    
    report << "=== T81 CanonFS Performance Analysis Report ===\n\n";
    report << "Generated: " << get_current_timestamp() << "\n\n";
    
    // Current performance snapshot
    auto current = analyze_current_performance();
    report << "## Current Performance\n";
    report << "- Status: " << current.summary << "\n";
    report << "- Throughput: " << current.metrics["operations_per_second"] << " ops/sec\n";
    report << "- Avg Operation Time: " << current.metrics["average_operation_time_ms"] << " ms\n";
    report << "- Policy Denial Rate: " << current.metrics["policy_denial_rate_percent"] << "%\n";
    report << "- Evidence Log Size: " << current.metrics["evidence_log_size"] << " entries\n";
    report << "- Memory Usage: " << current.metrics["memory_usage_mb"] << " MB\n\n";
    
    // Performance alerts
    auto alerts = get_performance_alerts();
    if (!alerts.empty()) {
        report << "## 🚨 Performance Alerts\n";
        for (const auto& alert : alerts) {
            report << "- " << alert << "\n";
        }
        report << "\n";
    }
    
    // Optimization suggestions
    auto suggestions = get_optimization_suggestions();
    if (!suggestions.empty()) {
        report << "## 💡 Optimization Suggestions\n";
        for (const auto& suggestion : suggestions) {
            report << "- " << suggestion << "\n";
        }
        report << "\n";
    }
    
    // Historical trends (last 24 hours)
    auto historical = get_historical_analysis(24);
    if (!historical.empty()) {
        report << "## 📈 24-Hour Trends\n";
        for (size_t i = 0; i < std::min(historical.size(), static_cast<size_t>(5)); ++i) {
            report << "- " << historical[i].timestamp << ": " << historical[i].summary << "\n";
        }
        report << "\n";
    }
    
    // Performance bottlenecks
    auto bottlenecks = identify_performance_bottlenecks();
    if (!bottlenecks.empty()) {
        report << "## 🔍 Performance Bottlenecks\n";
        for (const auto& bottleneck : bottlenecks) {
            report << "- " << bottleneck << "\n";
        }
        report << "\n";
    }
    
    report << "=== End Report ===\n";
    return report.str();
}

// Private implementation methods
std::string PerformanceAnalyzer::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(&time_t);
    return oss.str();
}

double PerformanceAnalyzer::calculate_operation_throughput() {
    // This would be calculated from actual operation metrics
    // For now, return a placeholder based on current system load
    return 2.5; // ops/sec
}

double PerformanceAnalyzer::calculate_policy_denial_rate() {
    // This would be calculated from actual policy decisions
    // For now, return a placeholder
    return 0.02; // 2% denial rate
}

double PerformanceAnalyzer::calculate_average_operation_time() {
    // This would be calculated from actual operation timing
    // For now, return a placeholder
    return 150.0; // 150ms average
}

size_t PerformanceAnalyzer::get_current_evidence_log_size() {
    // This would get the actual evidence log size from the engine
    // For now, return a placeholder
    return 1250; // 1250 entries
}

double PerformanceAnalyzer::get_current_memory_usage() {
    // This would calculate actual memory usage
    // For now, return a placeholder
    return 45.0; // 45MB
}

std::vector<std::string> PerformanceAnalyzer::detect_performance_anomalies() {
    std::vector<std::string> anomalies;
    
    auto current = analyze_current_performance();
    
    // Detect performance anomalies
    if (current.metrics["average_operation_time_ms"] > 1000.0) {
        anomalies.push_back("🔴 CRITICAL: Operation latency spike detected");
    }
    
    if (current.metrics["policy_denial_rate_percent"] > 15.0) {
        anomalies.push_back("🔴 CRITICAL: Unusual policy denial pattern");
    }
    
    if (current.metrics["operations_per_second"] < 0.1) {
        anomalies.push_back("🔴 WARNING: Extremely low throughput");
    }
    
    return anomalies;
}

std::string PerformanceAnalyzer::identify_performance_bottlenecks() {
    std::vector<std::string> bottlenecks;
    
    auto current = analyze_current_performance();
    
    // Identify potential bottlenecks
    if (current.metrics["average_operation_time_ms"] > 500.0) {
        bottlenecks.push_back("Policy evaluation latency");
    }
    
    if (current.metrics["memory_usage_mb"] > 80.0) {
        bottlenecks.push_back("Memory pressure from evidence log size");
    }
    
    if (current.metrics["operations_per_second"] < 1.0) {
        bottlenecks.push_back("I/O throughput limitation");
    }
    
    return bottlenecks;
}

std::vector<OptimizationStrategy> PerformanceAnalyzer::generate_optimization_recommendations() {
    std::vector<OptimizationStrategy> strategies;
    
    // Evidence log optimization
    strategies.push_back({
        "Evidence Log Rotation",
        "Implement automatic evidence log rotation to prevent memory growth",
        25.0,
        "medium"
    });
    
    // Policy caching
    strategies.push_back({
        "Policy Decision Caching",
        "Cache policy decisions to reduce evaluation overhead",
        15.0,
        "low"
    });
    
    // Parallel processing
    strategies.push_back({
        "Parallel Import/Export",
        "Enable concurrent CanonFS operations for higher throughput",
        40.0,
        "high"
    });
    
    // Memory optimization
    strategies.push_back({
        "Memory Pool Management",
        "Use memory pools for evidence context allocation",
        20.0,
        "medium"
    });
    
    // Async processing
    strategies.push_back({
        "Asynchronous Operations",
        "Implement non-blocking CanonFS operations",
        30.0,
        "high"
    });
    
    return strategies;
}

} // namespace t81::canonfs
